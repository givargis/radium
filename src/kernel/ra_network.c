/**
 * Copyright (c) Tony Givargis, 2020-2025
 *
 * ra_network.c
 */

#define _POSIX_C_SOURCE 200809L

#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/uio.h>
#include <unistd.h>
#include <netdb.h>

#include "ra_thread.h"
#include "ra_error.h"
#include "ra_network.h"

#define CLOSE(fd)					\
	do {						\
		if (0 < (fd)) {				\
			shutdown((fd), SHUT_RDWR);	\
			close((fd));			\
			(fd) = 0;			\
		}					\
	} while (0)

struct ra__network {
	int fd;
	struct server {
		int fd;
		ra__thread_t thread;
		struct server *link;
		struct client {
			int fd;
			ra__thread_t thread;
			struct client *link;
			/*-*/
			void *ctx;
			ra__network_fnc_t fnc;
		} *clients;
		/*-*/
		void *ctx;
		ra__network_fnc_t fnc;
	} *servers;
};

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
		CLOSE(fd);
		return 0;
	}
	return fd;
}

static void
_client_(void *ctx)
{
	struct ra__network network;
	struct client *client;

	client = (struct client *)ctx;
	memset(&network, 0, sizeof (struct ra__network));
	network.fd = client->fd;
	client->fnc(client->ctx, &network);
	CLOSE(network.fd);
	client->fd = 0;
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
		while (client && ra__thread_good(client->thread)) {
			client = client->link;
		}
		if (!client) {
			if (!(client = ra__malloc(sizeof (struct client)))) {
				CLOSE(fd);
				RA__ERROR_TRACE(0);
				// ignore
			}
			memset(client, 0, sizeof (struct client));
			client->link = server->clients;
			server->clients = client;
		}
		client->fd = fd;
		client->ctx = server->ctx;
		client->fnc = server->fnc;
		if (!(client->thread = ra__thread_open(_client_, client))) {
			CLOSE(fd);
			RA__ERROR_TRACE(0);
			// ignore
		}
	}
}

ra__network_t
ra__network_listen(const char *hostname,
		   const char *servname,
		   ra__network_fnc_t fnc,
		   void *ctx)
{
	struct addrinfo hints, *res, *p;
	struct ra__network *network;
	struct server *server;
	int fd;

	assert( ra__strlen(hostname) );
	assert( ra__strlen(servname) );
	assert( fnc );

	// initialize

	if (!(network = ra__malloc(sizeof (struct ra__network)))) {
		RA__ERROR_TRACE(0);
		return NULL;
	}
	memset(network, 0, sizeof (struct ra__network));

	// address

	memset(&hints, 0, sizeof (struct addrinfo));
	hints.ai_family = PF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	if (getaddrinfo(hostname, servname, &hints, &res)) {
		ra__network_close(network);
		RA__ERROR_TRACE(RA__ERROR_NETWORK_ADDRESS);
		return NULL;
	}

	// listen

	fd = 0;
	p = res;
	while (p) {
		if (!(fd = osocket(p->ai_family,
				   p->ai_socktype,
				   p->ai_protocol)) ||
		    (0 > bind(fd, p->ai_addr, p->ai_addrlen)) ||
		    (0 > listen(fd, SOMAXCONN))) {
			CLOSE(fd);
			p = p->ai_next;
			continue;
		}
		if (!(server = ra__malloc(sizeof (struct server)))) {
			CLOSE(fd);
			ra__network_close(network);
			RA__ERROR_TRACE(0);
			return NULL;
		}
		memset(server, 0, sizeof (struct server));
		server->fd = fd;
		server->ctx = ctx;
		server->fnc = fnc;
		if (!(server->thread = ra__thread_open(_server_, server))) {
			CLOSE(fd);
			ra__network_close(network);
			RA__FREE(server);
			RA__ERROR_TRACE(0);
			return NULL;
		}
		server->link = network->servers;
		network->servers = server;
		ra__log("info: listening on '%s:%s'",
			inet_ntoa(((struct sockaddr_in *)
				   p->ai_addr)->sin_addr),
			servname);
		p = p->ai_next;
	}
	freeaddrinfo(res);
	p = res = NULL;

	// listening?

	if (!network->servers) {
		ra__network_close(network);
		RA__ERROR_TRACE(RA__ERROR_NETWORK_INTERFACE);
		return NULL;
	}
	return network;
}

ra__network_t
ra__network_connect(const char *hostname, const char *servname)
{
	struct addrinfo hints, *res, *p;
	struct ra__network *network;
	int fd;

	assert( ra__strlen(hostname) );
	assert( ra__strlen(servname) );

	// initialize

	if (!(network = ra__malloc(sizeof (struct ra__network)))) {
		RA__ERROR_TRACE(0);
		return NULL;
	}
	memset(network, 0, sizeof (struct ra__network));

	// address

	memset(&hints, 0, sizeof (struct addrinfo));
	hints.ai_family = PF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	if (getaddrinfo(hostname, servname, &hints, &res)) {
		ra__network_close(network);
		RA__ERROR_TRACE(RA__ERROR_NETWORK_ADDRESS);
		return NULL;
	}

	// open

	fd = 0;
	p = res;
	while (p) {
		if (!(fd = osocket(p->ai_family,
				   p->ai_socktype,
				   p->ai_protocol)) ||
		    (0 > connect(fd, p->ai_addr, p->ai_addrlen))) {
			CLOSE(fd);
			p = p->ai_next;
			continue;
		}
		break;
	}
	freeaddrinfo(res);
	p = res = NULL;

	// connected?

	if (!fd) {
		ra__network_close(network);
		RA__ERROR_TRACE(RA__ERROR_NETWORK_CONNECT);
		return NULL;
	}
	network->fd = fd;
	return network;
}

void
ra__network_close(ra__network_t network)
{
	struct server *server, *server_;
	struct client *client, *client_;

	if (network) {
		CLOSE(network->fd);
		server = network->servers;
		while (server) {
			server_ = server;
			server = server->link;
			CLOSE(server_->fd);
			ra__thread_close(server_->thread);
			client = server_->clients;
			while (client) {
				client_ = client;
				client = client->link;
				ra__thread_close(client_->thread);
				memset(client_, 0, sizeof (struct client));
				RA__FREE(client_);
			}
			memset(server_, 0, sizeof (struct server));
			RA__FREE(server_);
		}
		memset(network, 0, sizeof (struct ra__network));
	}
	RA__FREE(network);
}

int
ra__network_read(ra__network_t network, void *buf_, uint64_t len)
{
	char *buf;
	ssize_t n;

	assert( network );
	assert( !len || buf_ );

	buf = (char *)buf_;
	while (len) {
		if (0 >= (n = read(network->fd, buf, len))) {
			RA__ERROR_TRACE(RA__ERROR_NETWORK_READ);
			return -1;
		}
		buf += (uint64_t)n;
		len -= (uint64_t)n;
	}
	return 0;
}

int
ra__network_write(ra__network_t network, const void *buf, uint64_t len)
{
	assert( network );
	assert( !len || buf );

	if (len != (uint64_t)write(network->fd, buf, len)) {
		RA__ERROR_TRACE(RA__ERROR_NETWORK_WRITE);
		return -1;
	}
	return 0;
}

int
ra__network_writev(ra__network_t network, int n, ...)
{
	struct iovec iovec[RA__NETWORK_WRITEV_MAX_N];
	uint64_t len;
	va_list va;

	assert( network );
	assert( RA__NETWORK_WRITEV_MAX_N >= n );

	len = 0;
	va_start(va, n);
	for (int i=0; i<n; ++i) {
		iovec[i].iov_base = (void *)va_arg(va, const void *);
		iovec[i].iov_len = (size_t)va_arg(va, uint64_t);
		len += (uint64_t)iovec[i].iov_len;
	}
	va_end(va);
	if (len != (uint64_t)writev(network->fd, iovec, n)) {
		RA__ERROR_TRACE(RA__ERROR_NETWORK_WRITE);
		return -1;
	}
	return 0;
}
