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
#ifndef TASSE_TOPOLOGY_H
#define TASSE_TOPOLOGY_H

typedef union {
	char	string[4];
	uint32	integer;
} name_t;

static_assert( sizeof(name_t) == 4, "sizeof(name_t) must be 4" );

#define AF_PROTEIN		BIT( 0 )	// atom belongs to aminoacid
#define AF_NUCLEIC		BIT( 1 )	// atom belongs to nucleotide
#define AF_SOLVENT		BIT( 2 )	// atom belongs to solvent
#define AF_AMINONT		BIT( 3 )	// atom belongs n-terminal aminoacid
#define AF_AMINOCT		BIT( 4 )	// atom belongs c-terminal aminoacid
#define AF_HYDROGEN		BIT( 5 )	// atom is a hydrogen
#define AF_SKIP			BIT( 6 )	// atom is skipped when writing output PDB

typedef struct atom_s {
	uint32	serial;					// serial number of the atom
	uint32	resnum;					// residue number of the atom
	uint32	rfirst;					// first atom index of the residue
	uint32	rcount;					// number of atoms in the residue
	uint8	chainnum;				// chain number of the atom
	uint8	chainid;				// chain identificator ('A', 'B', etc.)
	uint16	flags;					// atomic flags (bit combination of AF_xxx)
	name_t	title;					// original name of the atom
	name_t	residue;				// original name of the residue
	name_t	restopo;				// topological name of the residue
	name_t	xtitle;					// fixed atom's title (trimmed and whitespace-expanded)
	name_t	xresidue;				// fixed residue's title (trimmed and whitespace-expanded)
	real	radius;					// VdW radius
	real	charge;					// partial charge in AMBER units
} atom_t;

static_assert( sizeof(atom_t) == ( 40 + 2*sizeof(real) ), "invalid sizeof(atom_t)" );

typedef struct coord3_s {
	real	x;
	real	y;
	real	z;
} coord3_t;

typedef struct coord4_s {
	real	x;
	real	y;
	real	z;
	real	r;
} coord4_t;

static_assert( sizeof(coord3_t) == ( 3 * sizeof(real) ), "sizeof(coord3_t) must be 3 * sizeof(real)" );
static_assert( sizeof(coord4_t) == ( 4 * sizeof(real) ), "sizeof(coord4_t) must be 4 * sizeof(real)" );

interface ITopology
{
	typedef void (*TrajectoryCallback_t)( uint32, uint32, const coord3_t* );
	virtual void Initialize() = 0;
	virtual void Clear() = 0;
	virtual bool Load( const char *topFile, int topNature, const char *crdFile, int crdNature ) = 0;
	virtual bool Save( const char *outFile ) = 0;
	virtual uint32 ProcessTrajectory( const char *trajFile, int trajNature, size_t firstSnap, TrajectoryCallback_t func, bool pacifier ) = 0;
	virtual size_t GetAtomCount() const = 0;
	virtual size_t GetSolventSize() const = 0;
	virtual atom_t *GetAtomArray() const = 0;
	virtual coord3_t *GetBaseCoords() const = 0;
};

extern ITopology *topology;

#endif //TASSE_TOPOLOGY_H
