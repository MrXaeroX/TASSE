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
#include <tasse.h>

class CUtils : public IUtils
{
public:
	virtual void *Alloc( size_t numbytes );
	virtual void Free( void *ptr );
	virtual char *StrDup( const char *s );
	virtual void Warning( const char *fmt, ... );
	virtual void Fatal( const char *fmt, ... );
	virtual double FloatMilliseconds();
	virtual char **GetFileList( const char *directory, const char *extension, int *numfiles );
	virtual void FreeFileList( char **list );
	virtual int Atoi( const char *str, size_t len = 0 );
	virtual real Atof( const char *str, size_t len = 0 );
	virtual void Trim( char *dst, size_t size, const char *src );
	virtual int ExtractFileName( char *dest, size_t size, const char *path );
	virtual int ExtractFilePath( char *dst, size_t size, const char *src );
	virtual	int ExtractFileExtension( char *dest, size_t size, const char *path );
	virtual int GetMainDirectory( char *out, size_t outSize );
private:
	static const size_t c_MaxFoundFiles = 8192;
};

static CUtils utilsLocal;
IUtils *utils = &utilsLocal;

//////////////////////////////////////////////////////////////////////////

void *CUtils :: Alloc( size_t numbytes )
{
	assert( numbytes != 0 );
	void *result = calloc( numbytes, 1 );
	if ( !result )
		utils->Fatal( "memory allocation failed on %u bytes!\n", (uint32)numbytes );
	return result;
}

void CUtils :: Free( void *ptr )
{
	if ( ptr )
		free( ptr );
}

char *CUtils :: StrDup( const char *s )
{
	if ( !s ) return nullptr;
	size_t l = strlen( s );
	char *ss = reinterpret_cast<char*>( this->Alloc( l + 1 ) );
	if ( !ss ) return nullptr;
	memcpy( ss, s, l );
	ss[l] = '\0';
	return ss;
}

void CUtils :: Warning( const char *fmt, ... )
{
	assert( fmt != nullptr );
	char output[MAX_CONSOLE_OUTPUT_STRING];

	va_list argptr;
	va_start( argptr, fmt );
	_vsnprintf_s( output, sizeof(output), sizeof(output)-1, fmt, argptr );
	va_end( argptr );
#if defined(_QTASSE)
	console->Print( CC_YELLOW "<b>WARNING:</b> " CC_DEFAULT "%s", output );
#else
	console->Print( CC_YELLOW "WARNING: " CC_DEFAULT "%s", output );
#endif
}

void CUtils :: Fatal( const char *fmt, ... )
{
	assert( fmt != nullptr );
	char output[MAX_CONSOLE_OUTPUT_STRING];

	ThreadInterrupt();

	va_list argptr;
	va_start( argptr, fmt );
	_vsnprintf_s( output, sizeof(output), sizeof(output)-1, fmt, argptr );
	va_end( argptr );
#if defined(_QTASSE)
	console->Print( CC_RED "<b>ERROR:</b> " CC_DEFAULT "%s", output );
	throw std::runtime_error( output );
#else
	console->Print( CC_RED "ERROR: " CC_DEFAULT "%s", output );
	longjmp( gpGlobals->abort_marker, -1 );
#endif
}

double CUtils :: FloatMilliseconds()
{
#if defined(_WIN32)
	FILETIME ftime;
	GetSystemTimeAsFileTime( &ftime );
	double rval = ftime.dwLowDateTime;
	rval += ( static_cast<int64>( ftime.dwHighDateTime) ) << 32;
	return ( rval / 10000.0 );
#else //!_WIN32
	struct timeval tp;
	struct timezone tzp;
	static int secbase = 0;
	gettimeofday( &tp, &tzp );
	if ( !secbase ) {
		secbase = tp.tv_sec;
		return tp.tv_usec / 1000.0;
	}
	return ( tp.tv_sec - secbase ) + tp.tv_usec / 1000.0;
#endif //_WIN32
}

char **CUtils :: GetFileList( const char *directory, const char *extension, int *numfiles )
{
	char *list[c_MaxFoundFiles];
	char search[MAX_OSPATH];
	int flag = 1;
	int nfiles = 0;

	if ( !extension )
		extension = "";

	if ( extension[0] == '/' && extension[1] == 0 ) {
		extension = "";
		flag = 0;
	} 

#if defined(_WIN32)
	if ( flag ) flag = _A_SUBDIR;

	sprintf_s( search, sizeof(search), "%s/*%s", directory, extension );

	struct _finddata_t findinfo;
	intptr_t findhandle = _findfirst( search, &findinfo );
	if ( findhandle == -1 ) {
		*numfiles = 0;
		return nullptr;
	}

	do {
		if ( flag ^ ( findinfo.attrib & _A_SUBDIR ) ) {
			if ( nfiles == c_MaxFoundFiles - 1 )
				break;
			const size_t sizFilename = strlen( findinfo.name ) + 1;
			list[nfiles] = reinterpret_cast<char*>( this->Alloc( sizFilename ) );
			memcpy( list[nfiles], findinfo.name, sizFilename );
			++nfiles;
		}
	} while ( _findnext( findhandle, &findinfo ) != -1 );
	_findclose( findhandle );
#else
	DIR *dirhandle;
	if ( ( dirhandle = opendir( directory ) ) == nullptr ) {
		*numfiles = 0;
		return nullptr;
	}

	struct dirent *d;
	struct stat st;

	while ( ( d = readdir( dirhandle ) ) != nullptr ) {
		sprintf_s( search, sizeof(search), "%s/%s", directory, d->d_name );
		if ( stat( search, &st ) == -1 ) {
			continue;
		}
		if ( !( flag ^ ( S_ISDIR( st.st_mode ) ) ) ) {
			continue;
		}
		if ( *extension ) {
			if ( strlen( d->d_name ) < strlen( extension ) || _stricmp( d->d_name + strlen( d->d_name ) - strlen( extension ), extension ) ) {
				continue;
			}
		}
		if ( nfiles == c_MaxFoundFiles - 1 )
			break;
		const size_t sizFilename = strlen( d->d_name ) + 1;
		list[nfiles] = reinterpret_cast<char*>( this->Alloc( sizFilename ) );
		memcpy( list[nfiles], d->d_name, sizFilename );
		++nfiles;
	}
	closedir( dirhandle );
#endif

	// return a copy of the list
	if ( numfiles ) *numfiles = nfiles;
	if ( !nfiles )
		return nullptr;

	char **listCopy = reinterpret_cast<char**>( this->Alloc( ( nfiles + 1 )*sizeof(*list) ) );
	memcpy( listCopy, list, nfiles * sizeof(*list) );
	listCopy[nfiles] = nullptr;

	return listCopy;
}

void CUtils :: FreeFileList( char **list )
{
	if ( list ) {
		for ( char **item = list; *item; ++item )
			this->Free( *item );
		this->Free( list );
	}
}

int CUtils :: Atoi( const char *str, size_t len )
{
	assert( str != nullptr );
	if ( !str ) 
		return 0;

	const uint8 *src = reinterpret_cast<const uint8*>( str );
	const uint8 *end = &src[len];
	if ( src == end ) while ( *end ) ++end;

#if defined(_DEBUG)
	// sanity check
	char buf[64];
	memset( buf, 0, sizeof(buf) );
	strncpy_s( buf, str, end - src );
	const int check = atoi( buf );
#endif

	// check for empty characters in string
	while ( src != end && (*src) <= 32 ) 
		++src;

	const int sgn = ( *src == '-' ) ? -1 : 1;
	if ( *src == '-' || *src == '+' ) ++src;

	int val = 0;

	// check for hex
	if ( src[0] == '0' && ( src[1] == 'x' || src[1] == 'X' ) ) {
		src += 2;
		while ( src != end ) {
			const int c = *src++;
			if ( c >= '0' && c <= '9' )
				val = ( val << 4 ) + c - '0';
			else if ( c >= 'a' && c <= 'f' )
				val = ( val << 4 ) + c - 'a' + 10;
			else if ( c >= 'A' && c <= 'F' )
				val = ( val << 4 ) + c - 'A' + 10;
			else
				break;
		}
		val *= sgn;
		assert( val == check );
		return val;
	}

	// check for character
	if ( src[0] == '\'' ) {
		val = sgn * src[1];
		assert( val == check );
		return val;
	}

	// assume decimal
	while ( src != end ) {
		const int c = *src++;
		if ( c < '0' || c > '9' ) 
			break;
		val = val * 10 + c - '0';
	}

	val *= sgn;
	assert( val == check );
	return val;
}

real CUtils :: Atof( const char *str, size_t len )
{
	assert( str != nullptr );
	if ( !str ) 
		return 0;

	const uint8 *src = reinterpret_cast<const uint8*>( str );
	const uint8 *end = &src[len];
	if ( src == end ) while ( *end ) ++end;

#if defined(_DEBUG)
	// sanity check
	char buf[64];
	memset( buf, 0, sizeof(buf) );
	strncpy_s( buf, str, end - src );
	const real check = static_cast<real>( atof( buf ) );
#endif

	// check for empty characters in string
	while ( src != end && (*src) <= 32 ) 
		++src;

	const real sgn = ( *src == '-' ) ? -1 : 1;
	if ( *src == '-' || *src == '+' ) ++src;

	real val = 0;

	// check for hex
	if ( src[0] == '0' && ( src[1] == 'x' || src[1] == 'X' ) ) {
		src += 2;
		while ( src != end ) {
			const int c = *src++;
			if ( c >= '0' && c <= '9' )
				val = ( val * 16 ) + c - '0';
			else if ( c >= 'a' && c <= 'f' )
				val = ( val * 16 ) + c - 'a' + 10;
			else if ( c >= 'A' && c <= 'F' )
				val = ( val * 16 ) + c - 'A' + 10;
			else
				break;
		}
		val *= sgn;
		assert( fabs( val - check ) < 0.00001 );
		return val;
	}

	// check for character
	if ( src[0] == '\'' ) {
		val = sgn * src[1];
		assert( fabs( val - check ) < 0.00001 );
		return val;
	}

	// assume decimal
	int decimal = -1;
	int total = 0;
	int exponent = 0;
	int expsgn = 0;

	while ( src != end ) {
		int c = *src++;
		if ( c == '.' ) {
			decimal = total;
			continue;
		} else if ( c == 'e' || c == 'E' ) {
			c = *src++;
			if ( c == '-' ) {
				expsgn = -1;
				continue;
			} else {
				expsgn = 1;
				if ( c == '+' )
					continue;
			}
		} 
		if ( c < '0' || c > '9' )
			break;
		if ( expsgn )
			exponent = exponent * 10 + c - '0';
		else {
			val = val * 10 + c - '0';
			++total;
		}
	}

	if ( expsgn > 0 ) {
		while ( exponent-- )
			val *= 10;
	} else if ( expsgn < 0 ) {
		while ( exponent-- )
			val /= 10;
	}

	if ( decimal == -1 ) {
		val *= sgn;
		assert( val == check );
		return val;
	}

	while ( total --> decimal )
		val /= 10;

	val *= sgn;

	assert( fabs( val - check ) < 0.00001 );
	return val;
}

void CUtils :: Trim( char *dst, size_t size, const char *src )
{
	memset( dst, 0, size );
	const size_t srclen = strlen( src );
	if ( srclen ) {
		const uint8 *ubeg = reinterpret_cast<const uint8*>( src );
		for ( ; *ubeg && *ubeg <= 32; ++ubeg ) {}
		const uint8 *uend = reinterpret_cast<const uint8*>( src + srclen - 1 );
		for ( ; uend > ubeg && *uend <= 32; --uend ) {}
		uint8 *udst = reinterpret_cast<uint8*>( dst );
		for ( size_t i = 0; ubeg <= uend && i < size - 1; ++i )
			*udst++ = *ubeg++;
	}
}

int CUtils :: ExtractFileName( char *dest, size_t size, const char *path )
{
	const char *s2;
	const char *s;
	size_t slen;
	int maxsize;

	slen = strlen( path );
	s = path + slen;

	for ( s2 = s ; s2 != path && *s2 != '/' && *s2 != '\\' ; --s2 );

	if ( s - s2 < 2 ) {
		dest[0] = 0;
		maxsize = 0;
	} else {
		if ( s2[0] == '\\' || s2[0] == '/' ) ++s2;
		maxsize = min( (int)(s-s2), (int)size-1 );
		strncpy_s( dest, size, s2, maxsize );
		dest[s-s2] = 0;
	}

	return maxsize;
}


int CUtils :: ExtractFilePath( char *dest, size_t size, const char *src )
{
	size_t slen = strlen( src );
	if ( !slen ) {
		*dest = '\0';
		return 0;
	}

	// Returns the path up to, but not including the last slash
	const char *s = src + slen - 1;

	while ( s != src && *s != '/' && *s != '\\' )
		--s;

	int maxsize = min( (int)(s-src), (int)size-1 );

	if ( maxsize > 0 )
		strncpy_s( dest, size, src, maxsize );

	dest[maxsize] = 0;
	return maxsize;
}

int CUtils :: ExtractFileExtension( char *dest, size_t size, const char *path )
{
	size_t slen = strlen( path );
	if ( !slen ) {
		*dest = '\0';
		return 0;
	}

	const char *s = path + slen - 1;

	while ( s != path && *s != '.' )
		--s;

	if ( s == path ) {
		*dest = 0;	// no extension
		return 0;
	}

	size_t maxsize = min( slen - ( s - path ), size - 1 );

	if ( maxsize > 0 )
		strncpy_s( dest, size, s, maxsize );

	dest[maxsize] = 0;
	return (int)maxsize;
}

int CUtils :: GetMainDirectory( char *out, size_t outSize )
{
	// return main program directory
	char buffer[MAX_OSPATH];
	memset( buffer, 0, sizeof(buffer) );
#if defined(_WIN32)
	GetModuleFileName( (HMODULE)0, buffer, (DWORD)sizeof(buffer) );
#else
	ssize_t result = readlink( "/proc/self/exe", out, outSize );
	if ( result == -1 ) {
		fprintf( stderr, "GetMainDirectory(): %s\n", strerror( errno ) );
		return 0;
	}
#endif
	ExtractFilePath( out, outSize, buffer );
	return 1;
}
