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
#include <hbonds.h>
#include <topology.h>

static GlobalVars_t gGlobals;
GlobalVars_t *gpGlobals = &gGlobals;

static void banner()
{
	console->Print( "----------------------------------------------------\n" );
	console->Print( CC_WHITE " Tightly Associated Solvent Shell Extractor (TASSE)\n" );
	console->Print( " Version %s %s build %s (%s)\n", PROGRAM_VERSION, PROGRAM_EXE_TYPE, PROGRAM_ARCH, __DATE__ );
	console->Print( "----------------------------------------------------\n" );
}

static void usage()
{
	console->Print( CC_WHITE "Usage:\n"
					" " PROGRAM_EXE_NAME " [arguments]\n"
					CC_WHITE "Arguments:\n"
					" -tf  : source topology filename\n"
					" -tn  : source topology nature (0 = autodetect)\n"
					" -cf  : source coordinate filename\n"
					" -cn  : source coordinate nature (0 = autodetect)\n"
					" -trf : coordinate trajectory filename\n"
					" -trn : coordinate trajectory nature (0 = autodetect)\n"
					" -n   : starting snapshot of the trajectory (skip N previous snapshots)\n"
					" -o   : output PDB filename\n"
					" -i   : output information filename\n"
					" -e   : relative dielectric permittivity of the medium (default 80)\n"
					" -r   : electrostatic cut-off radius, in angstroms (default 15.0)\n"
					" -ce  : electrostatic scale coefficient (default 0.25)\n"
					" -d   : h-bond maximum length, in angstroms (default 4.0)\n"
					" -h   : h-bond cut-off absolute energy, in kcal/mol (default 1)\n"
					" -ch  : h-bond scale coefficient (default 0.75)\n"
					" -p   : probability (trajectory occurence) cut-off (default 0.9)\n"
					" -ng  : don't group similar donor/acceptor atoms\n"
					" -q   : read atomic charges from source (not applicable to PDB nature)\n"
					" -s   : solvent residue title (default HOH)\n"
					" -t   : number of threads (default is autodetect)\n"
					" -low : low thread priority (yield resources to other programs)\n"
					" -est : show progress pacifier (estimate completion time)\n"
					" -v   : verbose mode (print log messages to the console)\n"
					" -x   : don't process trajectory, only convert source to output PDB\n"
					" -?   : print help for arguments (this message)\n"
					"\n" );
}

static const char *bool_to_string( bool value )
{
	return value ? "true" : "false";
}

static void print_globals()
{
	console->Print( CC_WHITE "Settings:\n" );
	console->Print( " %-20s : %s\n", "topology file", gGlobals.input_topology );
	console->Print( " %-20s : %s\n", "topology nature", NatureHelper( gGlobals.input_topology_nature ).toString() );
	console->Print( " %-20s : %s\n", "coordinate file", gGlobals.input_coordinate );
	console->Print( " %-20s : %s\n", "coordinate nature", NatureHelper( gGlobals.input_coordinate_nature ).toString() );
	console->Print( " %-20s : %s\n", "trajectory file", gGlobals.input_trajectory );
	console->Print( " %-20s : %s\n", "trajectory nature", NatureHelper( gGlobals.input_trajectory_nature ).toString() );
	console->Print( " %-20s : %u\n", "start from snapshot", static_cast<uint32>( gGlobals.first_snap ) );
	console->Print( " %-20s : %s\n", "output PDB file", gGlobals.output_pdbname );
	console->Print( " %-20s : %s\n", "output info file", gGlobals.output_tuples );
	console->Print( " %-20s : %s\n", "solvent residue name", gGlobals.solvent_title );
	console->Print( " %-20s : %g\n", "dielectric constant", gGlobals.dielectric_const );
	console->Print( " %-20s : %g\n", "electrostatic radius", gGlobals.electrostatic_radius );
	console->Print( " %-20s : %g\n", "electrostatic coeff", gGlobals.electrostatic_coeff );
	console->Print( " %-20s : %g\n", "h-bond max length", gGlobals.hbond_max_length );
	console->Print( " %-20s : %g\n", "h-bond coeff", gGlobals.hbond_126_coeff );
	console->Print( " %-20s : %g\n", "h-bond cutoff energy", gGlobals.hbond_cutoff_energy );
	console->Print( " %-20s : %g%%\n", "occurence cutoff", gGlobals.occurence_cutoff * 100.0 );
	console->Print( " %-20s : %g%%\n", "VdW tolerance", gGlobals.vdw_tolerance * 100.0 );
	console->Print( " %-20s : %s\n", "h-bond grouping", bool_to_string( gGlobals.group_bonds ) );
	console->Print( " %-20s : %s\n", "read charges", bool_to_string( gGlobals.read_charges ) );
	if ( gGlobals.thread_count > 0 )
		console->Print( " %-20s : %i\n", "threads", gGlobals.thread_count );
	else
		console->Print( " %-20s : %s\n", "threads", "Autodetect" );
	console->Print( " %-20s : %s\n", "priority", gGlobals.low_prio ? "Low" : "Normal" );
	console->Print( " %-20s : %s\n", "estimate", bool_to_string ( gGlobals.pacifier ) );
	console->Print( " %-20s : %s\n", "convert only", bool_to_string( gGlobals.convert_only ) );
	console->Print( " %-20s : %s\n", "verbose mode", bool_to_string( gGlobals.verbose ) );
	console->Print( "\n" );
}

static bool parse_cmdline( int argc, char *argv[] )
{
	memset( gGlobals.input_topology, 0, sizeof(gGlobals.input_topology) );
	memset( gGlobals.input_coordinate, 0, sizeof(gGlobals.input_coordinate) );
	memset( gGlobals.input_trajectory, 0, sizeof(gGlobals.input_trajectory) );
	memset( gGlobals.output_pdbname, 0, sizeof(gGlobals.output_pdbname) );
	memset( gGlobals.output_tuples, 0, sizeof(gGlobals.output_tuples) );
	memset( gGlobals.solvent_title, 0, sizeof(gGlobals.solvent_title) );

	// init defaults
	gGlobals.thread_count = -1;
	gGlobals.low_prio = false;
	gGlobals.pacifier = false;
	gGlobals.verbose = false;
	gGlobals.convert_only = false;
	gGlobals.read_charges = false;
	gGlobals.group_bonds = true;
	gGlobals.first_snap = 0;
	gGlobals.dielectric_const = real( 80 );
	gGlobals.electrostatic_radius = real( 15 );
	gGlobals.hbond_cutoff_energy = real( 1.0 );
	gGlobals.hbond_max_length = real( 4.5 );
	gGlobals.electrostatic_coeff = real( 0.25 );
	gGlobals.hbond_126_coeff = real( 0.75 );
	gGlobals.occurence_cutoff = real( 0.9 );
	gGlobals.vdw_tolerance = real( 0.25 );
	gGlobals.input_topology_nature = TYP_AUTO;
	gGlobals.input_coordinate_nature = TYP_AUTO;
	gGlobals.input_trajectory_nature = TYP_AUTO;
	strcat_s( gGlobals.input_topology, "input.pdb" );
	strcat_s( gGlobals.input_coordinate, "input.pdb" );
	strcat_s( gGlobals.input_trajectory, "input.lst" );
	strcat_s( gGlobals.output_pdbname, "output.pdb" );
	strcat_s( gGlobals.output_tuples, "tuples.txt" );
	strcat_s( gGlobals.solvent_title, "HOH" );

	bool args_valid = true;
	for ( int i = 1; i < argc; ++i ) {
		if ( argv[i][0] == '-' ) {
			if ( !strcmp( &argv[i][1], "v" ) ) {
				gGlobals.verbose = true;
			} else if ( !strcmp( &argv[i][1], "low" ) ) {
				gGlobals.low_prio = true;
			} else if ( !strcmp( &argv[i][1], "est" ) ) {
				gGlobals.pacifier = true;
			} else if ( !strcmp( &argv[i][1], "t" ) ) {
				if ( i < argc - 1 && argv[i+1][0] != '-' ) {
					gGlobals.thread_count = utils->Atoi( argv[i+1] );
					++i;
				} else {
					utils->Warning( "missing value for argument \"%s\"!\n" ,argv[i] );
					args_valid = false;
				}
			} else if ( !strcmp( &argv[i][1], "x" ) ) {
				gGlobals.convert_only = true;
			} else if ( !strcmp( &argv[i][1], "q" ) ) {
				gGlobals.read_charges = true;
			} else if ( !strcmp( &argv[i][1], "ng" ) ) {
				gGlobals.group_bonds = false;
			} else if ( !strcmp( &argv[i][1], "?" ) ) {
				usage();
			} else if ( !strcmp( &argv[i][1], "s" ) ) {
				if ( i < argc - 1 && argv[i+1][0] != '-' ) {
					memset( gGlobals.solvent_title, 0, sizeof(gGlobals.solvent_title) );
					strncat_s( gGlobals.solvent_title, argv[i+1], 4 );
					++i;
				} else {
					utils->Warning( "missing value for argument \"%s\"!\n" ,argv[i] );
					args_valid = false;
				}
			} else if ( !strcmp( &argv[i][1], "o" ) ) {
				if ( i < argc - 1 && argv[i+1][0] != '-' ) {
					memset( gGlobals.output_pdbname, 0, sizeof(gGlobals.output_pdbname) );
					strncat_s( gGlobals.output_pdbname, argv[i+1], sizeof(gGlobals.output_pdbname)-1 );
					++i;
				} else {
					utils->Warning( "missing value for argument \"%s\"!\n" ,argv[i] );
					args_valid = false;
				}
			} else if ( !strcmp( &argv[i][1], "i" ) ) {
				if ( i < argc - 1 && argv[i+1][0] != '-' ) {
					memset( gGlobals.output_tuples, 0, sizeof(gGlobals.output_tuples) );
					strncat_s( gGlobals.output_tuples, argv[i+1], sizeof(gGlobals.output_tuples)-1 );
					++i;
				} else {
					utils->Warning( "missing value for argument \"%s\"!\n" ,argv[i] );
					args_valid = false;
				}
			} else if ( !strcmp( &argv[i][1], "tf" ) ) {
				if ( i < argc - 1 && argv[i+1][0] != '-' ) {
					memset( gGlobals.input_topology, 0, sizeof(gGlobals.input_topology) );
					strncat_s( gGlobals.input_topology, argv[i+1], sizeof(gGlobals.input_topology)-1 );
					++i;
				} else {
					utils->Warning( "missing value for argument \"%s\"!\n" ,argv[i] );
					args_valid = false;
				}
			} else if ( !strcmp( &argv[i][1], "cf" ) ) {
				if ( i < argc - 1 && argv[i+1][0] != '-' ) {
					memset( gGlobals.input_coordinate, 0, sizeof(gGlobals.input_coordinate) );
					strncat_s( gGlobals.input_coordinate, argv[i+1], sizeof(gGlobals.input_coordinate)-1 );
					++i;
				} else {
					utils->Warning( "missing value for argument \"%s\"!\n" ,argv[i] );
					args_valid = false;
				}
			} else if ( !strcmp( &argv[i][1], "trf" ) ) {
				if ( i < argc - 1 && argv[i+1][0] != '-' ) {
					memset( gGlobals.input_trajectory, 0, sizeof(gGlobals.input_trajectory) );
					strncat_s( gGlobals.input_trajectory, argv[i+1], sizeof(gGlobals.input_trajectory)-1 );
					++i;
				} else {
					utils->Warning( "missing value for argument \"%s\"!\n" ,argv[i] );
					args_valid = false;
				}
			} else if ( !strcmp( &argv[i][1], "tn" ) ) {
				if ( i < argc - 1 && argv[i+1][0] != '-' ) {
					gGlobals.input_topology_nature = utils->Atoi( argv[i+1] );
					++i;
				} else {
					utils->Warning( "missing value for argument \"%s\"!\n" ,argv[i] );
					args_valid = false;
				}
			} else if ( !strcmp( &argv[i][1], "cn" ) ) {
				if ( i < argc - 1 && argv[i+1][0] != '-' ) {
					gGlobals.input_coordinate_nature = utils->Atoi( argv[i+1] );
					++i;
				} else {
					utils->Warning( "missing value for argument \"%s\"!\n" ,argv[i] );
					args_valid = false;
				}
			} else if ( !strcmp( &argv[i][1], "trn" ) ) {
				if ( i < argc - 1 && argv[i+1][0] != '-' ) {
					gGlobals.input_trajectory_nature = utils->Atoi( argv[i+1] );
					++i;
				} else {
					utils->Warning( "missing value for argument \"%s\"!\n" ,argv[i] );
					args_valid = false;
				}
			} else if ( !strcmp( &argv[i][1], "n" ) ) {
				if ( i < argc - 1 && argv[i+1][0] != '-' ) {
					gGlobals.first_snap = static_cast<size_t>( utils->Atoi( argv[i+1] ) );
					++i;
				} else {
					utils->Warning( "missing value for argument \"%s\"!\n" ,argv[i] );
					args_valid = false;
				}
			} else if ( !strcmp( &argv[i][1], "e" ) ) {
				if ( i < argc - 1 && argv[i+1][0] != '-' ) {
					gGlobals.dielectric_const = utils->Atof( argv[i+1] );
					if ( gGlobals.dielectric_const < 0 )
						gGlobals.dielectric_const = 0;
					++i;
				} else {
					utils->Warning( "missing value for argument \"%s\"!\n" ,argv[i] );
					args_valid = false;
				}
			} else if ( !strcmp( &argv[i][1], "r" ) ) {
				if ( i < argc - 1 && argv[i+1][0] != '-' ) {
					gGlobals.electrostatic_radius = utils->Atof( argv[i+1] );
					if ( gGlobals.electrostatic_radius < 2 )
						gGlobals.electrostatic_radius = 2;
					++i;
				} else {
					utils->Warning( "missing value for argument \"%s\"!\n" ,argv[i] );
					args_valid = false;
				}
			} else if ( !strcmp( &argv[i][1], "d" ) ) {
				if ( i < argc - 1 && argv[i+1][0] != '-' ) {
					gGlobals.hbond_max_length = utils->Atof( argv[i+1] );
					if ( gGlobals.hbond_max_length < 2 )
						gGlobals.hbond_max_length = 2;
					++i;
				} else {
					utils->Warning( "missing value for argument \"%s\"!\n" ,argv[i] );
					args_valid = false;
				}
			} else if ( !strcmp( &argv[i][1], "h" ) ) {
				if ( i < argc - 1 && argv[i+1][0] != '-' ) {
					gGlobals.hbond_cutoff_energy = utils->Atof( argv[i+1] );
					if ( gGlobals.hbond_cutoff_energy < 0 )
						gGlobals.hbond_cutoff_energy = 0;
					++i;
				} else {
					utils->Warning( "missing value for argument \"%s\"!\n" ,argv[i] );
					args_valid = false;
				}
			} else if ( !strcmp( &argv[i][1], "p" ) ) {
				if ( i < argc - 1 && argv[i+1][0] != '-' ) {
					gGlobals.occurence_cutoff = utils->Atof( argv[i+1] );
					if ( gGlobals.occurence_cutoff < 0 )
						gGlobals.occurence_cutoff = 0;
					else if ( gGlobals.occurence_cutoff > 1 )
						gGlobals.occurence_cutoff = 1;
					++i;
				} else {
					utils->Warning( "missing value for argument \"%s\"!\n" ,argv[i] );
					args_valid = false;
				}
			} else if ( !strcmp( &argv[i][1], "vdw" ) ) {
				if ( i < argc - 1 && argv[i+1][0] != '-' ) {
					gGlobals.vdw_tolerance = utils->Atof( argv[i+1] );
					if ( gGlobals.vdw_tolerance < 0 )
						gGlobals.vdw_tolerance = 0;
					else if ( gGlobals.vdw_tolerance > 1 )
						gGlobals.vdw_tolerance = 1;
					++i;
				} else {
					utils->Warning( "missing value for argument \"%s\"!\n" ,argv[i] );
					args_valid = false;
				}
			} else if ( !strcmp( &argv[i][1], "ce" ) ) {
				if ( i < argc - 1 && argv[i+1][0] != '-' ) {
					gGlobals.electrostatic_coeff = utils->Atof( argv[i+1] );
					if ( gGlobals.electrostatic_coeff < 0 )
						gGlobals.electrostatic_coeff = 0;
					++i;
				} else {
					utils->Warning( "missing value for argument \"%s\"!\n" ,argv[i] );
					args_valid = false;
				}
			} else if ( !strcmp( &argv[i][1], "ch" ) ) {
				if ( i < argc - 1 && argv[i+1][0] != '-' ) {
					gGlobals.hbond_126_coeff = utils->Atof( argv[i+1] );
					if ( gGlobals.hbond_126_coeff < 0 )
						gGlobals.hbond_126_coeff = 0;
					++i;
				} else {
					utils->Warning( "missing value for argument \"%s\"!\n" ,argv[i] );
					args_valid = false;
				}
			} else {
				utils->Warning( "unknown argument \"%s\"!\n" ,argv[i] );
				args_valid = false;
			}
		} else {
			utils->Warning( "unexpected token \"%s\"!\n" ,argv[i] );
			args_valid = false;
			continue;
		}
	}

	if ( gGlobals.input_topology_nature < TYP_AUTO || gGlobals.input_topology_nature >= TYP_MAX_ ) {
		utils->Warning( "invalid input topology nature (%i), using default \"%s\" (%i)\n", gGlobals.input_topology_nature, NatureHelper( DEFAULT_TOPOLOGY_NATURE ).toString(), DEFAULT_TOPOLOGY_NATURE );
		gGlobals.input_topology_nature = DEFAULT_TOPOLOGY_NATURE;
		args_valid = false;
	}
	if ( gGlobals.input_coordinate_nature < TYP_AUTO || gGlobals.input_coordinate_nature >= TYP_MAX_ ) {
		utils->Warning( "invalid input coordinate nature (%i), using default \"%s\" (%i)\n", gGlobals.input_coordinate_nature, NatureHelper( DEFAULT_COORDINATE_NATURE ).toString(), DEFAULT_COORDINATE_NATURE );
		gGlobals.input_coordinate_nature = DEFAULT_COORDINATE_NATURE;
		args_valid = false;
	}
	if ( gGlobals.input_trajectory_nature < TYP_AUTO || gGlobals.input_trajectory_nature >= TYP_MAX_ ) {
		utils->Warning( "invalid input trajectory nature (%i), using default \"%s\" (%i)\n", gGlobals.input_trajectory_nature, NatureHelper( DEFAULT_TRAJECTORY_NATURE ).toString(), DEFAULT_TRAJECTORY_NATURE );
		gGlobals.input_trajectory_nature = DEFAULT_TRAJECTORY_NATURE;
		args_valid = false;
	}

	return args_valid;
}

static void check_natures()
{
	if ( gGlobals.input_topology_nature == TYP_AUTO ) {
		gGlobals.input_topology_nature = NatureHelper()[gGlobals.input_topology]();
		if ( gGlobals.input_topology_nature == TYP_AUTO ) {
			utils->Warning( "failed to autodetect input topology nature, using default \"%s\" (%i)\n", NatureHelper( DEFAULT_TOPOLOGY_NATURE ).toString(), DEFAULT_TOPOLOGY_NATURE );
			gGlobals.input_topology_nature = DEFAULT_TOPOLOGY_NATURE;
		} else {
			console->Print( "Auto-detected input topology nature: %s\n", NatureHelper( gGlobals.input_topology_nature ).toString() );
		}
	}
	if ( gGlobals.input_coordinate_nature == TYP_AUTO ) {
		gGlobals.input_coordinate_nature = NatureHelper()[gGlobals.input_coordinate]();
		if ( gGlobals.input_coordinate_nature == TYP_AUTO ) {
			utils->Warning( "failed to autodetect input coordinate nature, using default \"%s\" (%i)\n", NatureHelper( DEFAULT_COORDINATE_NATURE ).toString(), DEFAULT_COORDINATE_NATURE );
			gGlobals.input_coordinate_nature = DEFAULT_COORDINATE_NATURE;
		} else {
			console->Print( "Auto-detected input coordinate nature: %s\n", NatureHelper( gGlobals.input_coordinate_nature ).toString() );
		}
	}
	if ( gGlobals.input_trajectory_nature == TYP_AUTO ) {
		gGlobals.input_trajectory_nature = NatureHelper()[gGlobals.input_trajectory]();
		if ( gGlobals.input_trajectory_nature == TYP_AUTO ) {
			utils->Warning( "failed to autodetect input trajectory nature, using default \"%s\" (%i)\n", NatureHelper( DEFAULT_TRAJECTORY_NATURE ).toString(), DEFAULT_TRAJECTORY_NATURE );
			gGlobals.input_trajectory_nature = DEFAULT_TRAJECTORY_NATURE;
		} else {
			console->Print( "Auto-detected input trajectory nature: %s\n", NatureHelper( gGlobals.input_trajectory_nature ).toString() );
		}
	}
}

static void process_trajectory_snapshot( uint32 threadNum, uint32 snapshotNum, const coord3_t *coords )
{
	// this function can be called concurrently and therefore must be reenterant!
	hbonds->CalcMicrosets( threadNum, snapshotNum, coords );
}

static void cleanup()
{
	// clear H-bond information
	hbonds->Clear();

	// clear topology
	topology->Clear();

	// unload config files
	configs->Unload();

	// cleanup threads
	ThreadCleanup();
}

int main( int argc, char *argv[] )
{
	setlocale( LC_ALL, "C" );

	// print program name and version info
	banner();

	// parse and verify command line
	if ( !parse_cmdline( argc, argv ) )
		usage();

	// print global settings
	print_globals();

	// autodetect natures if needed
	check_natures();

	// handle fatal program termination
	if ( setjmp( gGlobals.abort_marker ) ) {
		cleanup();
		return 1;
	}

	// init threads
	ThreadSetDefault( gGlobals.thread_count, gGlobals.low_prio );

	// remember start time
	double startTime = utils->FloatMilliseconds();

	// scan and load config files
	configs->Load();

	// initialize topology libraries
	topology->Initialize();

	// initialize H-bond information
	hbonds->Initialize();

	// load input topology and coordinates
	if ( topology->Load( gGlobals.input_topology, gGlobals.input_topology_nature,
						 gGlobals.input_coordinate, gGlobals.input_coordinate_nature ) ) {
		// setup H-bond detection
		hbonds->Setup( ThreadCount() );
		// topology and coordinates were successfully loaded
		// start processing the trajectory
		if ( gGlobals.convert_only ) {
			// just save the PDB
			topology->Save( gGlobals.output_pdbname );
		} else {
			// build lists of donors and acceptors
			hbonds->BuildAtomLists();
			// process the trajectory
			uint32 total = topology->ProcessTrajectory( gGlobals.input_trajectory, gGlobals.input_trajectory_nature, 
														gGlobals.first_snap, process_trajectory_snapshot, gGlobals.pacifier );
			if ( total ) {
				// build final solvent info
				hbonds->BuildFinalSolvent( total );
				// save final PDB
				topology->Save( gGlobals.output_pdbname );
				// write output tables
				if ( *gGlobals.output_tuples )
					hbonds->PrintFinalTuples( total, gGlobals.output_tuples );
				// write performance counters
				hbonds->PrintPerformanceCounters();
			}
		}
	}

	// elapsed time
	logfile->LogTimeElapsed( utils->FloatMilliseconds() - startTime );

	cleanup();
	return 0;
}
