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
#include <nature.h>

typedef struct {
	int			index;
	const char *string;
	const char *fileext1;
	const char *fileext2;
} natureDesc_t;

static const natureDesc_t natureDesc[] = {
{ TYP_AUTO,		"Autodetect",	nullptr,	nullptr },
{ TYP_LIST,		"PDB list",		".lst",		nullptr },
{ TYP_PDB,		"PDB file",		".pdb",		nullptr },
{ TYP_PRMTOP,	"AMBER prmtop", ".prmtop",	nullptr },
{ TYP_INPCRD,	"AMBER coords", ".inpcrd",	".rst" },
{ TYP_MDCRD,	"AMBER mdcrd",	".mdcrd",	nullptr }
};

const char *NatureHelper :: toString() const
{
	assert( value_ >= TYP_AUTO && value_ < TYP_MAX_ );
	const size_t numDesc = sizeof(natureDesc) / sizeof(natureDesc[0]);
	for ( size_t i = 0; i < numDesc; ++i ) {
		if ( natureDesc[i].index == value_ )
			return natureDesc[i].string;
	}
	return "???";
}

NatureHelper &NatureHelper :: fromString( const char *string )
{
	assert( string != nullptr );
	value_ = TYP_AUTO;
	const size_t numDesc = sizeof(natureDesc) / sizeof(natureDesc[0]);
	for ( size_t i = 0; i < numDesc; ++i ) {
		if ( natureDesc[i].string && !strcmp( natureDesc[i].string, string ) ) {
			value_ = natureDesc[i].index;
			break;
		}
	}
	return *this;
}

NatureHelper &NatureHelper :: operator[]( const char *filename )
{
	assert( filename != nullptr );
	value_ = TYP_AUTO;
	char extbuf[32];
	if ( utils->ExtractFileExtension( extbuf, sizeof(extbuf), filename ) ) {
		const size_t numDesc = sizeof(natureDesc) / sizeof(natureDesc[0]);
		for ( size_t i = 0; i < numDesc; ++i ) {
			if ( natureDesc[i].fileext1 && !_stricmp( natureDesc[i].fileext1, extbuf ) ) {
				value_ = natureDesc[i].index;
				break;
			}
			if ( natureDesc[i].fileext2 && !_stricmp( natureDesc[i].fileext2, extbuf ) ) {
				value_ = natureDesc[i].index;
				break;
			}
		}
	}
	return *this;
}