//
// Copyright (c) Tony Givargis, 2024-2025
// givargis@uci.edu
// ra_logo.c
//

#include "ra_logger.h"

void
ra_logo(void)
{

	ra_logger(RA_COLOR_CYAN, "\t               ___\n");
	ra_logger(RA_COLOR_CYAN, "\t  _______ ____/ (_)_ ____ _\n");
	ra_logger(RA_COLOR_CYAN, "\t / __/ _ `/ _  / / // /  ' \\\n");
	ra_logger(RA_COLOR_CYAN, "\t/_/  \\_,_/\\_,_/_/\\_,_/_/_/_/\n");
	ra_logger(RA_COLOR_CYAN, "\t----------------------------\n");
	ra_logger(RA_COLOR_GRAY, "\tCopyright (c) Tony Givargis, 2024-2025\n");
	ra_logger(RA_COLOR_GRAY, "\tgivargis@uci.edu\n\n");
}
