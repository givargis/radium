/* Copyright (c) Tony Givargis, 2024-2026 */

#include "core/ra_core.h"
#include "ra_version.h"

#define MKS(x) #x
#define STR(x) MKS(x)

const char *RA_VERSION =
	STR(RA_VERSION_MAJOR) "."
	STR(RA_VERSION_MINOR) "."
	STR(RA_VERSION_PATCH);

const char *RA_COPYRIGHT =
	"Copyright (c) Tony Givargis, 2024-2026";

const char *RA_GRAPHICS =
	"                   ___\n"
	"   _________ _____/ (_)_  ______ ___\n"
	"  / ___/ __ `/ __  / / / / / __ `__ \\\n"
	" / /  / /_/ / /_/ / / /_/ / / / / / /\n"
	"/_/   \\__,_/\\__,_/_/\\__,_/_/ /_/ /_/\n";
