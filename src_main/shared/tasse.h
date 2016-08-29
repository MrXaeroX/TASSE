/***************************************************************************
* Copyright (C) 2015-2016 Alexander V. Popov.
* 
* This file is part of Tightly Associated Solvent Shell Extractor (TASSE) 
* source code.
* 
* TASSE is free software; you can redistribute it and/or modify it under 
* the terms of the GNU General Public License as published by the Free 
* Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
* 
* TASSE is distributed in the hope that it will be useful, but WITHOUT 
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or 
* FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
* for more details.
* 
* You should have received a copy of the GNU General Public License along
* with this program; if not, write to the Free Software Foundation, Inc.,
* 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
***************************************************************************/
#ifndef TASSE_H
#define TASSE_H

// common defines
#define PROGRAM_LARGE_NAME		"TASSE"
#define PROGRAM_SHORT_NAME		"TASSE"
#define PROGRAM_VERSION			"1.0"

#if defined(_WIN32)
#define PROGRAM_EXE_NAME		"tasse.exe"
#else
#define PROGRAM_EXE_NAME		"tasse"
#endif

#if defined(_WIN32)
#define PROGRAM_EXE_TYPE		"EXE"
#elif defined(_LINUX)
#define PROGRAM_EXE_TYPE		"ELF"
#else
#define PROGRAM_EXE_TYPE		"Generic"
#endif

#if defined(_M_X64)
#define PROGRAM_ARCH			"x64"
#elif defined(__arm__)
#define PROGRAM_ARCH			"arm"
#elif defined(__mips__)
#define PROGRAM_ARCH			"mips"
#else
#define PROGRAM_ARCH			"x86"
#endif

#define PROGRAM_CONFIG_PATH		"conf"
#define PROGRAM_HELP_FILENAME	"tasse.pdf"

// load common includes
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <malloc.h>
#include <errno.h>
#include <setjmp.h>
#include <time.h>

// system-specific includes
#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <direct.h>
#include <io.h>
#else
#include <unistd.h>
#include <dirent.h>
#include <pthread.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/stat.h>
#endif

// load STL includes
#include <string>
#include <vector>
#include <list>
#include <set>
#include <map>
#include <algorithm>

#if !defined(min)
#define min		std::min
#endif
#if !defined(max)
#define max		std::max
#endif

#if defined(_LINUX)
#define _stricmp	strcasecmp
#define _strnicmp	strncasecmp
#define _strnicmp	strncasecmp 
#define _vsnprintf	vsnprintf
#define _snprintf	snprintf
#define _unlink		unlink
#endif

// load traits
#include "traits/bitutils.h"
#include "traits/datatypes.h"
#include "traits/fileutils.h"
#include "traits/interface.h"

// program includes
#include "secure_crt.h"
#include "utils.h"
#include "threads.h"
#include "globals.h"
#include "logfile.h"
#include "cfgfile.h"
#include "console.h"

#endif //TASSE_H
