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
#ifndef TASSE_NATURE_H
#define TASSE_NATURE_H

enum {
	TYP_AUTO = 0,
	TYP_LIST,
	TYP_PDB,
	TYP_PRMTOP,
	TYP_INPCRD,
	TYP_MDCRD,
	TYP_MAX_
};

#define DEFAULT_TOPOLOGY_NATURE		TYP_PDB
#define DEFAULT_COORDINATE_NATURE	TYP_PDB
#define DEFAULT_TRAJECTORY_NATURE	TYP_LIST

class NatureHelper {
public:
	explicit NatureHelper( int value = TYP_AUTO ) : value_( value ) {}
	int operator()() const { return value_; }
	const char *toString() const;
	NatureHelper &fromString( const char *string );
	NatureHelper &operator[]( const char *filename );
private:
	int value_;
};

#endif //TASSE_NATURE_H
