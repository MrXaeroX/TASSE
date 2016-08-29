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
#ifndef TASSE_CFGFILE_H
#define TASSE_CFGFILE_H

// TASSE cfg file interface

typedef enum {
	CONFIG_TYPE_UNKNOWN = 0,
	CONFIG_TYPE_DONTCARE,
	CONFIG_TYPE_HBPARMS,
	CONFIG_TYPE_HBRES,
	CONFIG_TYPE_QRES,
	CONFIG_TYPE_SOLVRES,
	CONFIG_TYPE_VDWRES
} eConfigType;

interface ICfgFile
{
	virtual const char *GetToken() const = 0;
	virtual bool ParseToken( bool crossline ) = 0;
	virtual void UnparseToken() = 0;
	virtual bool MatchToken( const char *match ) = 0;
	virtual bool TokenAvailable() = 0;
	virtual void SkipRestOfLine() = 0;
	virtual void Rewind() = 0;
};

interface ICfgList
{
	typedef void (*ConfigCallback_t)( ICfgFile* );
	virtual void Load() = 0;
	virtual void Unload() = 0;
	virtual void ForEach( eConfigType type, ConfigCallback_t func ) = 0;
};

extern ICfgList *configs;

#endif //TASSE_CFGFILE_H
