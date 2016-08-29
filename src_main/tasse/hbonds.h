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
#ifndef TASSE_HBONDS_H
#define TASSE_HBONDS_H

// TASSE H-bond computation interface

interface IHBonds
{
	virtual	void Initialize() = 0;
	virtual	void Setup( uint32 numthreads ) = 0;
	virtual void Clear() = 0;
	virtual void BuildAtomLists() = 0;
	virtual void CalcMicrosets( const uint32 threadNum, const uint32 snapshotNum, const struct coord3_s *coords ) = 0;
	virtual void BuildFinalSolvent( uint32 totalFrames ) = 0;
	virtual	bool PrintFinalTuples( uint32 totalFrames, const char *tupleFile ) const = 0;
	virtual void PrintPerformanceCounters() = 0;
};

extern IHBonds *hbonds;

#endif //TASSE_HBONDS_H
