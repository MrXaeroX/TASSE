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
#ifndef TASSE_SECURE_CRT_H
#define TASSE_SECURE_CRT_H
#if ( _MSC_VER < 1400 )

// Secure CRT wrappers for Linux

#define sprintf_s(buffer, buffer_size, stringbuffer, ...)		sprintf(buffer, stringbuffer, __VA_ARGS__)
#define _snprintf_s(buffer, buffer_size, cnt, format, ...)		snprintf(buffer, cnt, format, __VA_ARGS__)
#define _vsnprintf_s(buffer, buffer_size, cnt, format, argptr)	vsnprintf(buffer, cnt, format, argptr)
#define vsprintf_s(buffer, format, argptr)						vsprintf(buffer, format, argptr)
#define sscanf_s(buffer, format, ...)							sscanf(buffer, format, __VA_ARGS__)
#define fscanf_s(file, format, ...)								fscanf(file, format, __VA_ARGS__)
#define fprintf_s(stream, format, ...)							fprintf(stream, format, __VA_ARGS__)
#define strtok_s(strToken, strDelimit, context)					strtok(strToken, strDelimit)

inline int fopen_s( FILE **f, const char *name, const char *mode ) 
{
	int ret = 0;
	*f = fopen(name, mode);
	if (!*f) ret = errno;
	return ret;
}

inline int strerror_s( char *buf, size_t size, int errnumber )
{
	memset( buf, 0, size );
	strncpy( buf, strerror( errnumber ), size - 1 );
	return errnumber;
}

inline int strcpy_s( char *strDestination, size_t, const char *strSource )
{
	strcpy( strDestination, strSource );
	return 0;
}

inline int strcpy_s( char *strDestination, const char *strSource )
{
	strcpy( strDestination, strSource );
	return 0;
}

inline int strncpy_s( char *strDestination, size_t, const char *strSource, size_t count )
{
	strncpy( strDestination, strSource, count );
	return 0;
}

inline int strncpy_s( char *strDestination, const char *strSource, size_t count )
{
	strncpy( strDestination, strSource, count );
	return 0;
}

inline int strcat_s( char *strDestination, size_t, const char *strSource )
{
	strcat( strDestination, strSource );
	return 0;
}

inline int strcat_s( char *strDestination, const char *strSource )
{
	strcat( strDestination, strSource );
	return 0;
}

inline int strncat_s( char *strDestination, size_t, const char *strSource, size_t count )
{
	strncat( strDestination, strSource, count );
	return 0;
}

inline int strncat_s( char *strDestination, const char *strSource, size_t count )
{
	strncat( strDestination, strSource, count );
	return 0;
}

#endif //_MSC_VER
#endif //TASSE_SECURE_CRT_H
