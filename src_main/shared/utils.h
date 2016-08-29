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
#ifndef TASSE_UTILS_H
#define TASSE_UTILS_H

// TASSE utility functions

#define MAX_OSPATH		4096

interface IUtils
{
	virtual void *Alloc( size_t numbytes ) = 0;
	virtual void Free( void *ptr ) = 0;
	virtual char *StrDup( const char *s ) = 0;
	virtual void Warning( const char *fmt, ... ) = 0;
	virtual void Fatal( const char *fmt, ... ) = 0;
	virtual double FloatMilliseconds() = 0;
	virtual char **GetFileList( const char *directory, const char *extension, int *numfiles ) = 0;
	virtual void FreeFileList( char **list ) = 0;
	virtual int Atoi( const char *str, size_t len = 0 ) = 0;
	virtual real Atof( const char *str, size_t len = 0 ) = 0;
	virtual void Trim( char *dst, size_t size, const char *src ) = 0;
	virtual int ExtractFileName( char *dest, size_t size, const char *path ) = 0;
	virtual int ExtractFilePath( char *dst, size_t size, const char *src ) = 0;
	virtual	int ExtractFileExtension( char *dest, size_t size, const char *path ) = 0;
	virtual int GetMainDirectory( char *out, size_t outSize ) = 0;
};

extern IUtils *utils;

#endif //TASSE_UTILS_H
