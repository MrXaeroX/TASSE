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
#include "window.h"

#include <QtCore/QLocale>
#include <QtCore/QTextCodec>
#include <QtGui/QApplication>

QApplication *g_pApp = nullptr;
QTextCodec *g_pDefaultCodec = nullptr;

static GlobalVars_t gGlobals;
GlobalVars_t *gpGlobals = &gGlobals;

static void parse_cmdline( int argc, char *argv[] )
{
	memset( gGlobals.input_topology, 0, sizeof(gGlobals.input_topology) );
	memset( gGlobals.input_coordinate, 0, sizeof(gGlobals.input_coordinate) );
	memset( gGlobals.input_trajectory, 0, sizeof(gGlobals.input_trajectory) );
	memset( gGlobals.output_pdbname, 0, sizeof(gGlobals.output_pdbname) );
	memset( gGlobals.output_tuples, 0, sizeof(gGlobals.output_tuples) );
	memset( gGlobals.solvent_title, 0, sizeof(gGlobals.solvent_title) );

	// init defaults
	gGlobals.thread_count = -1;
	gGlobals.low_prio = true;
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
				}
			} else if ( !strcmp( &argv[i][1], "x" ) ) {
				gGlobals.convert_only = true;
			} else if ( !strcmp( &argv[i][1], "q" ) ) {
				gGlobals.read_charges = true;
			} else if ( !strcmp( &argv[i][1], "ng" ) ) {
				gGlobals.group_bonds = false;
			} else if ( !strcmp( &argv[i][1], "?" ) ) {
				//ignore
			} else if ( !strcmp( &argv[i][1], "s" ) ) {
				if ( i < argc - 1 && argv[i+1][0] != '-' ) {
					memset( gGlobals.solvent_title, 0, sizeof(gGlobals.solvent_title) );
					strncat_s( gGlobals.solvent_title, argv[i+1], 4 );
					++i;
				}
			} else if ( !strcmp( &argv[i][1], "o" ) ) {
				if ( i < argc - 1 && argv[i+1][0] != '-' ) {
					memset( gGlobals.output_pdbname, 0, sizeof(gGlobals.output_pdbname) );
					strncat_s( gGlobals.output_pdbname, argv[i+1], sizeof(gGlobals.output_pdbname)-1 );
					++i;
				}
			} else if ( !strcmp( &argv[i][1], "i" ) ) {
				if ( i < argc - 1 && argv[i+1][0] != '-' ) {
					memset( gGlobals.output_tuples, 0, sizeof(gGlobals.output_tuples) );
					strncat_s( gGlobals.output_tuples, argv[i+1], sizeof(gGlobals.output_tuples)-1 );
					++i;
				}
			} else if ( !strcmp( &argv[i][1], "tf" ) ) {
				if ( i < argc - 1 && argv[i+1][0] != '-' ) {
					memset( gGlobals.input_topology, 0, sizeof(gGlobals.input_topology) );
					strncat_s( gGlobals.input_topology, argv[i+1], sizeof(gGlobals.input_topology)-1 );
					++i;
				}
			} else if ( !strcmp( &argv[i][1], "cf" ) ) {
				if ( i < argc - 1 && argv[i+1][0] != '-' ) {
					memset( gGlobals.input_coordinate, 0, sizeof(gGlobals.input_coordinate) );
					strncat_s( gGlobals.input_coordinate, argv[i+1], sizeof(gGlobals.input_coordinate)-1 );
					++i;
				}
			} else if ( !strcmp( &argv[i][1], "trf" ) ) {
				if ( i < argc - 1 && argv[i+1][0] != '-' ) {
					memset( gGlobals.input_trajectory, 0, sizeof(gGlobals.input_trajectory) );
					strncat_s( gGlobals.input_trajectory, argv[i+1], sizeof(gGlobals.input_trajectory)-1 );
					++i;
				}
			} else if ( !strcmp( &argv[i][1], "tn" ) ) {
				if ( i < argc - 1 && argv[i+1][0] != '-' ) {
					gGlobals.input_topology_nature = utils->Atoi( argv[i+1] );
					++i;
				}
			} else if ( !strcmp( &argv[i][1], "cn" ) ) {
				if ( i < argc - 1 && argv[i+1][0] != '-' ) {
					gGlobals.input_coordinate_nature = utils->Atoi( argv[i+1] );
					++i;
				}
			} else if ( !strcmp( &argv[i][1], "trn" ) ) {
				if ( i < argc - 1 && argv[i+1][0] != '-' ) {
					gGlobals.input_trajectory_nature = utils->Atoi( argv[i+1] );
					++i;
				}
			} else if ( !strcmp( &argv[i][1], "n" ) ) {
				if ( i < argc - 1 && argv[i+1][0] != '-' ) {
					gGlobals.first_snap = static_cast<size_t>( utils->Atoi( argv[i+1] ) );
					++i;
				}
			} else if ( !strcmp( &argv[i][1], "e" ) ) {
				if ( i < argc - 1 && argv[i+1][0] != '-' ) {
					gGlobals.dielectric_const = utils->Atof( argv[i+1] );
					if ( gGlobals.dielectric_const < 0 )
						gGlobals.dielectric_const = 0;
					++i;
				}
			} else if ( !strcmp( &argv[i][1], "r" ) ) {
				if ( i < argc - 1 && argv[i+1][0] != '-' ) {
					gGlobals.electrostatic_radius = utils->Atof( argv[i+1] );
					if ( gGlobals.electrostatic_radius < 2 )
						gGlobals.electrostatic_radius = 2;
					++i;
				}
			} else if ( !strcmp( &argv[i][1], "d" ) ) {
				if ( i < argc - 1 && argv[i+1][0] != '-' ) {
					gGlobals.hbond_max_length = utils->Atof( argv[i+1] );
					if ( gGlobals.hbond_max_length < 2 )
						gGlobals.hbond_max_length = 2;
					++i;
				}
			} else if ( !strcmp( &argv[i][1], "h" ) ) {
				if ( i < argc - 1 && argv[i+1][0] != '-' ) {
					gGlobals.hbond_cutoff_energy = utils->Atof( argv[i+1] );
					if ( gGlobals.hbond_cutoff_energy < 0 )
						gGlobals.hbond_cutoff_energy = 0;
					++i;
				}
			} else if ( !strcmp( &argv[i][1], "p" ) ) {
				if ( i < argc - 1 && argv[i+1][0] != '-' ) {
					gGlobals.occurence_cutoff = utils->Atof( argv[i+1] );
					if ( gGlobals.occurence_cutoff < 0 )
						gGlobals.occurence_cutoff = 0;
					else if ( gGlobals.occurence_cutoff > 1 )
						gGlobals.occurence_cutoff = 1;
					++i;
				}
			} else if ( !strcmp( &argv[i][1], "vdw" ) ) {
				if ( i < argc - 1 && argv[i+1][0] != '-' ) {
					gGlobals.vdw_tolerance = utils->Atof( argv[i+1] );
					if ( gGlobals.vdw_tolerance < 0 )
						gGlobals.vdw_tolerance = 0;
					else if ( gGlobals.vdw_tolerance > 1 )
						gGlobals.vdw_tolerance = 1;
					++i;
				}
			} else if ( !strcmp( &argv[i][1], "ce" ) ) {
				if ( i < argc - 1 && argv[i+1][0] != '-' ) {
					gGlobals.electrostatic_coeff = utils->Atof( argv[i+1] );
					if ( gGlobals.electrostatic_coeff < 0 )
						gGlobals.electrostatic_coeff = 0;
					++i;
				}
			} else if ( !strcmp( &argv[i][1], "ch" ) ) {
				if ( i < argc - 1 && argv[i+1][0] != '-' ) {
					gGlobals.hbond_126_coeff = utils->Atof( argv[i+1] );
					if ( gGlobals.hbond_126_coeff < 0 )
						gGlobals.hbond_126_coeff = 0;
					++i;
				}
			}
		}
	}

	if ( gGlobals.input_topology_nature < TYP_AUTO || gGlobals.input_topology_nature >= TYP_MAX_ )
		gGlobals.input_topology_nature = DEFAULT_TOPOLOGY_NATURE;

	if ( gGlobals.input_coordinate_nature < TYP_AUTO || gGlobals.input_coordinate_nature >= TYP_MAX_ )
		gGlobals.input_coordinate_nature = DEFAULT_COORDINATE_NATURE;

	if ( gGlobals.input_trajectory_nature < TYP_AUTO || gGlobals.input_trajectory_nature >= TYP_MAX_ )
		gGlobals.input_trajectory_nature = DEFAULT_TRAJECTORY_NATURE;
}

static int QTASSE_Main( int argc, char *argv[] )
{
	// setup locale and text codec
	setlocale( LC_ALL, "C" );
	QLocale usLocale( QLocale::English, QLocale::UnitedStates );
	QLocale::setDefault( usLocale );
	g_pDefaultCodec = QTextCodec::codecForName( "Windows-1251" );
	QTextCodec::setCodecForLocale( g_pDefaultCodec );
	QTextCodec::setCodecForCStrings( g_pDefaultCodec );

	// parse settings
	parse_cmdline( argc, argv );

	// init threads
	ThreadSetDefault( gGlobals.thread_count, gGlobals.low_prio );
	gGlobals.thread_count = ThreadCount();

	// start the app
	g_pApp = new QApplication( argc, argv );
	g_pApp->setApplicationName( PROGRAM_LARGE_NAME );
	g_pApp->setApplicationVersion( PROGRAM_VERSION );

	// create the main window
	CMainWindow::Instance().initialize();

	return g_pApp->exec();
}

#if defined(_WIN32)

#define MAX_ARGV	32

int APIENTRY WinMain( HINSTANCE, HINSTANCE, LPSTR lpCmdLine, int )
{
	int argc;
	char *argv[MAX_ARGV];
	char filename[MAX_OSPATH];

	// Extract argc and argv
	argc = 1;
	memset(argv, 0, sizeof(argv));

	bool in_quote = false;

	while ( *lpCmdLine && ( argc < MAX_ARGV ) ) {
		while ( *lpCmdLine && ( (uint8)(*lpCmdLine) <= 32 ) )
			++lpCmdLine;

		if ( *lpCmdLine ) {
			if ( *lpCmdLine == '"' ) {
				in_quote = !in_quote;
				++lpCmdLine;
			}

			argv[argc] = lpCmdLine;
			argc++;

			while ( *lpCmdLine && ( ( (uint8)(*lpCmdLine) > 32 ) || in_quote ) ) {
				if ( in_quote && ( *lpCmdLine == '"' ) ) {
					in_quote = !in_quote;
					break;
				}
				++lpCmdLine;
			}

			if ( *lpCmdLine ) {
				*lpCmdLine = 0;
				++lpCmdLine;
			}
		}
	}

	GetModuleFileName( nullptr, filename, sizeof(filename)-1 );
	argv[0] = filename;

	return QTASSE_Main( argc, argv );
}

#else /*!_WIN32*/

int main( int argc, char *argv[] )
{
	setlocale( LC_MESSAGES, "C" );
	return QTASSE_Main( argc, argv );
}

#endif /*_WIN32*/