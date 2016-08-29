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
#ifndef TASSE_GLOBALS_H
#define TASSE_GLOBALS_H

// TASSE global settings
// set from command line

typedef struct {
	int			thread_count;
	bool		low_prio;
	bool		pacifier;
	bool		verbose;
	bool		convert_only;
	bool		read_charges;
	bool		group_bonds;
	size_t		first_snap;
	real		dielectric_const;
	real		electrostatic_radius;
	real		electrostatic_coeff;
	real		hbond_max_length;
	real		hbond_cutoff_energy;
	real		hbond_126_coeff;
	real		occurence_cutoff;
	real		vdw_tolerance;
	int			input_topology_nature;
	int			input_coordinate_nature;
	int			input_trajectory_nature;
	char		input_topology[MAX_OSPATH];
	char		input_coordinate[MAX_OSPATH];
	char		input_trajectory[MAX_OSPATH];
	char		output_pdbname[MAX_OSPATH];
	char		output_tuples[MAX_OSPATH];
	char		solvent_title[8];
#if !defined(_QTASSE)
	jmp_buf		abort_marker;
#endif
} GlobalVars_t;

extern GlobalVars_t *gpGlobals;

#endif //TASSE_GLOBALS_H
