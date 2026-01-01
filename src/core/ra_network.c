/* Copyright (c) Tony Givargis, 2024-2026 */

#define _POSIX_C_SOURCE 200809L

#include <netinet/tcp.h>
#include <sys/uio.h>
#include <unistd.h>
#include <netdb.h>

#include "ra_thread.h"
#include "ra_network.h"

#define WRITEV_MAX_N 4

struct ra_network {
	int fd;
	struct server {
		int fd;
		ra_thread_t thread;
		struct server *link;
		struct client {
			int fd;
			ra_thread_t thread;
			struct client *link;
			/*-*/
			void *ctx;
			ra_network_fnc_t fnc;
		} *clients;
		/*-*/
		void *ctx;
		ra_network_fnc_t fnc;
	} *servers;
};

static int _flag_;

static void
sigint(int signum)
{
	if (SIGINT == signum) {
		_flag_ = 1;
	}
}

static void
sclose(int fd)
{
	if (0 < fd) {
		shutdown(fd, SHUT_RDWR);
		close(fd);
	}
}

static int
osocket(int domain, int type, int protocol)
{
	const int NODELAY = 1;
	const int OPTVAL = 1;
	int fd;

	if ((0 >= (fd = socket(domain, type, protocol))) ||
	    (0 > setsockopt(fd,
			    IPPROTO_TCP,
			    TCP_NODELAY,
			    (const void *)&NODELAY,
			    sizeof (NODELAY))) ||
	    (0 > setsockopt(fd,
			    SOL_SOCKET,
			    SO_REUSEADDR,
			    (const void *)&OPTVAL,
			    sizeof (OPTVAL)))) {
		sclose(fd);
		return 0;
	}
	return fd;
}

static void
_client_(void *ctx)
{
	struct ra_network network;
	struct client *client;

	client = (struct client *)ctx;
	memset(&network, 0, sizeof (struct ra_network));
	network.fd = client->fd;
	client->fnc(client->ctx, &network);
	sclose(network.fd);
	client->fd = 0;
	client->ctx = NULL;
	client->fnc = NULL;
}

static void
_server_(void *ctx)
{
	struct server *server;
	struct client *client;
	int fd;

	server = (struct server *)ctx;
	while (__sync_fetch_and_add(&server->fd, 0)) {
		if (0 >= (fd = accept(server->fd, NULL, NULL))) {
			continue;
		}
		client = server->clients;
		while (client && ra_thread_good(client->thread)) {
			client = client->link;
		}
		if (!client) {
			if (!(client = malloc(sizeof (struct client)))) {
				sclose(fd);
				RA_TRACE("out of memory (ignored)");
				continue;
			}
			memset(client, 0, sizeof (struct client));
			client->link = server->clients;
			server->clients = client;
		}
		client->fd = fd;
		client->ctx = server->ctx;
		client->fnc = server->fnc;
		if (!(client->thread = ra_thread_open(_client_, client))) {
			sclose(fd);
			client->fd = 0;
			client->ctx = NULL;
			client->fnc = NULL;
			RA_TRACE("^ (ignored)");
		}
	}
}

int
ra_network_listen(const char *hostname,
		  const char *servname,
		  ra_network_fnc_t fnc,
		  void *ctx)
{
	struct addrinfo hints, *res, *p;
	struct ra_network *network;
	struct server *server;
	int fd;

	assert( hostname && strlen(hostname) );
	assert( hostname && strlen(servname) );
	assert( fnc );

	/* initialize */

	if (!(network = malloc(sizeof (struct ra_network)))) {
		RA_TRACE("out of memory");
		return -1;
	}
	memset(network, 0, sizeof (struct ra_network));

	/* address */

	memset(&hints, 0, sizeof (struct addrinfo));
	hints.ai_family = PF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	if (getaddrinfo(hostname, servname, &hints, &res)) {
		ra_network_close(network);
		RA_TRACE("invalid network address");
		return -1;
	}

	/* listen */

	fd = 0;
	p = res;
	while (p) {
		if (!(fd = osocket(p->ai_family,
				   p->ai_socktype,
				   p->ai_protocol)) ||
		    (0 > bind(fd, p->ai_addr, p->ai_addrlen)) ||
		    (0 > listen(fd, SOMAXCONN))) {
			sclose(fd);
			p = p->ai_next;
			continue;
		}
		if (!(server = malloc(sizeof (struct server)))) {
			sclose(fd);
			ra_network_close(network);
			RA_TRACE("out of memory");
			return -1;
		}
		memset(server, 0, sizeof (struct server));
		server->fd = fd;
		server->ctx = ctx;
		server->fnc = fnc;
		if (!(server->thread = ra_thread_open(_server_, server))) {
			sclose(fd);
			RA_FREE(server);
			ra_network_close(network);
			RA_TRACE("^");
			return -1;
		}
		server->link = network->servers;
		network->servers = server;
		p = p->ai_next;
	}
	freeaddrinfo(res);
	p = res = NULL;

	/* listening? */

	if (!network->servers) {
		ra_network_close(network);
		RA_TRACE("no network interface found");
		return -1;
	}

	/* wait */

	_flag_ = 0;
	if ((SIG_ERR == signal(SIGHUP, sigint)) ||
	    (SIG_ERR == signal(SIGPIPE, sigint)) ||
	    (SIG_ERR == signal(SIGINT, sigint))) {
		ra_network_close(network);
		RA_TRACE("system failure detected");
		return -1;
	}
	while (!__sync_fetch_and_add(&_flag_, 0)) {
		sleep(1);
	}
	(void)signal(SIGHUP, SIG_DFL);
	(void)signal(SIGPIPE, SIG_DFL);
	(void)signal(SIGINT, SIG_DFL);
	ra_network_close(network);
	return 0;
}

ra_network_t
ra_network_connect(const char *hostname, const char *servname)
{
	struct addrinfo hints, *res, *p;
	struct ra_network *network;
	int fd;

	assert( hostname && strlen(hostname) );
	assert( hostname && strlen(servname) );

	/* initialize */

	if (!(network = malloc(sizeof (struct ra_network)))) {
		RA_TRACE("out of memory");
		return NULL;
	}
	memset(network, 0, sizeof (struct ra_network));

	/* address */

	memset(&hints, 0, sizeof (struct addrinfo));
	hints.ai_family = PF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	if (getaddrinfo(hostname, servname, &hints, &res)) {
		ra_network_close(network);
		RA_TRACE("invalid network address");
		return NULL;
	}

	/* open */

	fd = 0;
	p = res;
	while (p) {
		if (!(fd = osocket(p->ai_family,
				   p->ai_socktype,
				   p->ai_protocol)) ||
		    (0 > connect(fd, p->ai_addr, p->ai_addrlen))) {
			sclose(fd);
			p = p->ai_next;
			continue;
		}
		break;
	}
	freeaddrinfo(res);
	p = res = NULL;

	/* connected? */

	if (!fd) {
		ra_network_close(network);
		RA_TRACE("unable to connect to network");
		return NULL;
	}
	network->fd = fd;
	return network;
}

void
ra_network_close(ra_network_t network)
{
	struct server *server, *server_;
	struct client *client, *client_;

	if (network) {
		sclose(network->fd);
		server = network->servers;
		while (server) {
			server_ = server;
			server = server->link;
			sclose(server_->fd);
			ra_thread_close(server_->thread);
			client = server_->clients;
			while (client) {
				client_ = client;
				client = client->link;
				ra_thread_close(client_->thread);
				memset(client_, 0, sizeof (struct client));
				RA_FREE(client_);
			}
			memset(server_, 0, sizeof (struct server));
			RA_FREE(server_);
		}
		memset(network, 0, sizeof (struct ra_network));
		RA_FREE(network);
	}
}

int
ra_network_read(ra_network_t network, void *buf_, size_t len)
{
	char *buf = (char *)buf_;
	ssize_t n;

	assert( network );
	assert( !len || buf );

	while (len) {
		if (0 >= (n = read(network->fd, buf, len))) {
			RA_TRACE("network read failed");
			return -1;
		}
		buf += (size_t)n;
		len -= (size_t)n;
	}
	return 0;
}

int
ra_network_write(ra_network_t network, const void *buf_, size_t len)
{
	const char *buf = (const char *)buf_;
	ssize_t n;

	assert( network );
	assert( !len || buf );

	while (len) {
		if (0 >= (n = write(network->fd, buf, len))) {
			RA_TRACE("network write failed");
			return -1;
		}
		buf += (size_t)n;
		len -= (size_t)n;
	}
	return 0;
}
