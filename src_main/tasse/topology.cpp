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
#include <topology.h>

// Uncomment if you want to use original solvent residue numbers for tests
//#define DEBUG_SOLVENT_RESNUM

class CTopology : public ITopology
{
	typedef struct {
		char *pdbfile;
	} trajItemPDB_t;
public:
	CTopology();
	virtual ~CTopology();
	virtual void Initialize();
	virtual void Clear();
	virtual bool Load( const char *topFile, int topNature, const char *crdFile, int crdNature );
	virtual bool Save( const char *outFile );
	virtual uint32 ProcessTrajectory( const char *trajFile, int trajNature, size_t firstSnap, TrajectoryCallback_t func, bool pacifier );
	virtual size_t GetAtomCount() const { return atcount_; }
	virtual size_t GetSolventSize() const { return solvsize_; }
	virtual atom_t *GetAtomArray() const { return atoms_; }
	virtual coord3_t *GetBaseCoords() const { return coords_base_; }

	void ProcessTrajectoryThread_PDB( uint32 threadnum, uint32 num );
	void ProcessTrajectoryThread_AMBER( uint32 threadnum, uint32 num );

private:
	CTopology( const CTopology &other );
	CTopology& operator = ( const CTopology& other );
	bool LoadTopology( const char *topFile, int topNature, bool attemptLoadCoords );
	bool LoadCoordinates( const char *crdFile, int crdNature );
	void PostProcess();
	void Print() const;

	void CopyTrimmed( name_t *dst, const name_t *src ) const;
	void ParseTopology_PDBLine( const char *line, atom_t *top ) const;
	void ParseCoordinates_PDBLine( const char *line, coord3_t *crd ) const;
	bool LoadTopology_PDB( const char *topFile, bool loadCoords );
	bool LoadTopology_AMBER( const char *topFile );
	bool LoadCoordinates_PDB( const char *crdFile, coord3_t *out_coords );
	bool LoadCoordinates_AMBER( const char *crdFile, coord3_t *out_coords );
	uint32 ProcessTrajectory_PDB( const char *trajFile, size_t firstSnap, TrajectoryCallback_t func, bool pacifier );
	uint32 ProcessTrajectory_AMBER( const char *trajFile, size_t firstSnap, TrajectoryCallback_t func, bool pacifier );

public:
	typedef std::map<uint64,real> ChargeMap;
	typedef std::map<uint64,real> RadiusMap;
	typedef std::map<uint32,uint32> SolventMap;

	ChargeMap				chargeMap_;
	RadiusMap				radiusMap_;
	SolventMap				solventMap_;

	static const real c_AmberChargeScale;
	
private:
	size_t					atcount_;
	atom_t					*atoms_;
	coord3_t				*coords_base_;
	coord3_t				*coords_traj_[MAX_THREADS];
	size_t					remsize_;
	char					*remarks_;
	size_t					solvsize_;
	bool					chargeok_;
	TrajectoryCallback_t	callback_;
	trajItemPDB_t			*trajItems_;
	size_t					framebase_;
	size_t					framesize_;
	FILE					*file_traj_[MAX_THREADS];
	char					*frame_buf_[MAX_THREADS];
};

static CTopology topologyLocal;
ITopology *topology = &topologyLocal;

//////////////////////////////////////////////////////////////////////////

const real CTopology :: c_AmberChargeScale = real( 18.2223 );

static void Stub_ProcessTrajectoryThread_PDB( uint32 threadnum, uint32 num )
{
	topologyLocal.ProcessTrajectoryThread_PDB( threadnum, num );
}

static void Stub_ProcessTrajectoryThread_AMBER( uint32 threadnum, uint32 num )
{
	topologyLocal.ProcessTrajectoryThread_AMBER( threadnum, num );
}

//////////////////////////////////////////////////////////////////////////

static void QRes_LoadCharges( ICfgFile *cfg )
{
	name_t resname;
	name_t atname;
	uint64 key;

	while ( cfg->ParseToken( true ) ) {
		size_t len = strlen( cfg->GetToken() );
		const size_t maxlen = 4;
		memcpy( resname.string, cfg->GetToken(), std::min( len, maxlen ) );
		for ( size_t i = 0; i < len; ++i ) resname.string[i] = toupper( resname.string[i] );
		for ( size_t i = len; i < maxlen; ++i ) resname.string[i] = ' ';
		cfg->ParseToken( false );

		len = strlen( cfg->GetToken() );
		memcpy( atname.string, cfg->GetToken(), std::min( len, maxlen ) );
		for ( size_t i = 0; i < len; ++i ) atname.string[i] = toupper( atname.string[i] );
		for ( size_t i = len; i < maxlen; ++i ) atname.string[i] = ' ';
		cfg->ParseToken( false );

		real charge = utils->Atof( cfg->GetToken() ) * CTopology::c_AmberChargeScale;
		cfg->SkipRestOfLine();

		key = resname.integer;
		key = ( key << uint64( 32 ) ) | atname.integer;
		topologyLocal.chargeMap_.insert( std::make_pair( key, charge ) );
	}
}

static void VdWRes_LoadRadii( ICfgFile *cfg )
{
	name_t resname;
	name_t atname;
	uint64 key;

	while ( cfg->ParseToken( true ) ) {
		size_t len = strlen( cfg->GetToken() );
		const size_t maxlen = 4;
		memcpy( resname.string, cfg->GetToken(), std::min( len, maxlen ) );
		for ( size_t i = 0; i < len; ++i ) resname.string[i] = toupper( resname.string[i] );
		for ( size_t i = len; i < maxlen; ++i ) resname.string[i] = ' ';
		cfg->ParseToken( false );

		len = strlen( cfg->GetToken() );
		memcpy( atname.string, cfg->GetToken(), std::min( len, maxlen ) );
		for ( size_t i = 0; i < len; ++i ) atname.string[i] = toupper( atname.string[i] );
		for ( size_t i = len; i < maxlen; ++i ) atname.string[i] = ' ';
		cfg->ParseToken( false );

		real radius = utils->Atof( cfg->GetToken() );
		cfg->SkipRestOfLine();

		key = resname.integer;
		key = ( key << uint64( 32 ) ) | atname.integer;
		topologyLocal.radiusMap_.insert( std::make_pair( key, radius ) );
	}
}

static void SolvRes_LoadResidues( ICfgFile *cfg )
{
	name_t curres;
	name_t mapres;

	while ( cfg->ParseToken( true ) ) {
		size_t len = strlen( cfg->GetToken() );
		const size_t maxlen = 4;
		memcpy( curres.string, cfg->GetToken(), std::min( len, maxlen ) );
		for ( size_t i = len; i < maxlen; ++i ) curres.string[i] = ' ';
		if ( cfg->TokenAvailable() ) {
			cfg->ParseToken( false );
			len = strlen( cfg->GetToken() );
			memcpy( mapres.string, cfg->GetToken(), std::min( len, maxlen ) );
			for ( size_t i = len; i < maxlen; ++i ) mapres.string[i] = ' ';
			cfg->SkipRestOfLine();
		} else {
			mapres.integer = curres.integer;
		}
		topologyLocal.solventMap_.insert( std::make_pair( curres.integer, mapres.integer ) );
	}
}

CTopology :: CTopology() : atcount_( 0 ), atoms_( nullptr ), coords_base_( nullptr ),
						   remsize_( 0 ), remarks_( nullptr ), solvsize_( 0 ), chargeok_( false ), callback_( nullptr )
{
	memset( coords_traj_, 0, sizeof(coords_traj_) );
	memset( file_traj_, 0, sizeof(file_traj_) );
	memset( frame_buf_, 0, sizeof(frame_buf_) );
}

CTopology :: ~CTopology()
{
	// since this class is statically allocated,
	// we must have all dynamic data already deleted
	assert( atoms_ == nullptr );
	assert( coords_base_ == nullptr );
	assert( coords_traj_[0] == nullptr );
	assert( file_traj_[0] == nullptr );
	assert( remarks_ == nullptr );
	assert( frame_buf_[0] == nullptr );
}

void CTopology :: Initialize()
{
	// load charges, VdW radii and solvent residues from configs
	configs->ForEach( CONFIG_TYPE_QRES, QRes_LoadCharges );
	configs->ForEach( CONFIG_TYPE_VDWRES, VdWRes_LoadRadii );
	configs->ForEach( CONFIG_TYPE_SOLVRES, SolvRes_LoadResidues );
}

void CTopology :: Clear()
{
	atcount_ = 0;
	remsize_ = 0;
	solvsize_ = 0;
	chargeok_ = false;
	callback_ = nullptr;

	memset( coords_traj_, 0, sizeof(coords_traj_) );
	memset( file_traj_, 0, sizeof(file_traj_) );
	memset( frame_buf_, 0, sizeof(frame_buf_) );

	if ( atoms_ ) {
		utils->Free( atoms_ );
		atoms_ = nullptr;
	}
	if ( coords_base_ ) {
		utils->Free( coords_base_ );
		coords_base_ = nullptr;
	}
	for ( uint32 i = 0; i < MAX_THREADS; ++i ) {
		if ( coords_traj_[i] ) {
			utils->Free( coords_traj_[i] );
			coords_traj_[i] = nullptr;
		}
		if ( frame_buf_[i] ) {
			utils->Free( frame_buf_[i] );
			frame_buf_[i] = nullptr;
		}
	}
	if ( remarks_ ) {
		utils->Free( remarks_ );
		remarks_ = nullptr;
	}
}

bool CTopology :: Load( const char *topFile, int topNature, const char *crdFile, int crdNature )
{
	assert( topFile != nullptr );
	assert( crdFile != nullptr );

	// must be cleared first
	Clear();

	// check if topology and coords are the same
	const bool sameFiles = !strcmp( topFile, crdFile );

	// load topology
	if ( !this->LoadTopology( topFile, topNature, sameFiles ) )
		return false;

	// load coordinates, if not loaded
	if ( !coords_base_ ) {
		if ( !this->LoadCoordinates( crdFile, crdNature ) )
			return false;
	}

	// sanity check
	assert( atoms_ != nullptr );
	assert( coords_base_ != nullptr );

	// post-process the topology
	this->PostProcess();


#if 0
	// print topology and initial coordinates in PDB format
	this->Print();
#endif

	return true;
}

bool CTopology :: Save( const char *outFile )
{
	FILE *fp;
	if ( fopen_s( &fp, outFile, "w" ) ) {
		utils->Warning( "failed to open \"%s\" for writing!\n", outFile );
		return false;
	}

	console->Print( "Writing: \"%s\"...\n", outFile );

	fprintf( fp, "REMARK %-72s\n", "--------------------------------------------------------------" );
	fprintf( fp, "REMARK %-72s\n", " Topology converted by " PROGRAM_LARGE_NAME );
	fprintf( fp, "REMARK %-72s\n", "--------------------------------------------------------------" );

	if ( remarks_ )
		fwrite( remarks_, remsize_, 1, fp );

	uint16 chain_num = 1;
	uint16 top_flags = ~0;
	uint32 max_resnum = 0;
	uint32 serial_num = 1;

	const atom_t *top = atoms_;
	const coord3_t *crd = coords_base_;

	// first pass: write non-solvent atoms
	for ( size_t i = 0; i < atcount_; ++i, ++top, ++crd ) {
		if ( top->flags & ( AF_SKIP | AF_SOLVENT ) )
			continue;
		const uint16 tflags = top->flags & ( AF_PROTEIN | AF_NUCLEIC | AF_SOLVENT );
		if ( i && ( ( top->chainnum != chain_num ) || ( top_flags != tflags ) ) ) {
			fprintf( fp, "%-6s%5u\n", "TER", serial_num-1 );
			chain_num = top->chainnum;
		}
		top_flags = tflags;
		if ( top->resnum > max_resnum )
			max_resnum = top->resnum;
		if ( top->resnum > 9999 ) {
			fprintf( fp, "%-6s%5u %c%c%c%c %3s %c%5u   %8.3f%8.3f%8.3f\n", "ATOM", 
				serial_num, top->title.string[0], top->title.string[1], top->title.string[2], top->title.string[3],
				top->residue.string, top->chainid, top->resnum,
				crd->x, crd->y, crd->z );
		} else {
			fprintf( fp, "%-6s%5u %c%c%c%c %3s %c%4u    %8.3f%8.3f%8.3f\n", "ATOM", 
				serial_num, top->title.string[0], top->title.string[1], top->title.string[2], top->title.string[3],
				top->residue.string, top->chainid, top->resnum,
				crd->x, crd->y, crd->z );
		}
		++serial_num;
	}

	// second pass: write solvent atoms
	top = atoms_;
	crd = coords_base_;
	uint32 last_resnum = 0, curr_resnum = max_resnum;

	for ( size_t i = 0; i < atcount_; ++i, ++top, ++crd ) {
		if ( top->flags & AF_SKIP )
			continue;
		if ( !( top->flags & AF_SOLVENT ) )
			continue;
		const uint16 tflags = top->flags & ( AF_PROTEIN | AF_NUCLEIC | AF_SOLVENT );
		if ( ( top->chainnum != chain_num ) || ( top_flags != tflags ) ) {
			fprintf( fp, "%-6s%5u\n", "TER", serial_num-1 );
			chain_num = top->chainnum;
		}
		top_flags = tflags;
		if ( last_resnum != top->resnum ) {
			last_resnum = top->resnum;
			++curr_resnum;
		}
#if defined(DEBUG_SOLVENT_RESNUM)
		curr_resnum = top->resnum;
#endif
		if ( curr_resnum > 9999 ) {
			fprintf( fp, "%-6s%5u %c%c%c%c %3s %c%5u   %8.3f%8.3f%8.3f\n", "ATOM", 
				serial_num, top->title.string[0], top->title.string[1], top->title.string[2], top->title.string[3],
				top->residue.string, top->chainid, curr_resnum,
				crd->x, crd->y, crd->z );
		} else {
			fprintf( fp, "%-6s%5u %c%c%c%c %3s %c%4u    %8.3f%8.3f%8.3f\n", "ATOM", 
				serial_num, top->title.string[0], top->title.string[1], top->title.string[2], top->title.string[3],
				top->residue.string, top->chainid, curr_resnum,
				crd->x, crd->y, crd->z );
		}
		++serial_num;
	}

	fprintf( fp, "%-6s%5u\n", "TER", serial_num-1 );
	fprintf( fp, "END\n" );

	fclose( fp );
	return true;
}

bool CTopology :: LoadTopology( const char *topFile, int topNature, bool attemptLoadCoords )
{
	console->Print( "Loading: \"%s\"...\n", topFile );

	switch ( topNature ) {
	case TYP_PDB: return LoadTopology_PDB( topFile, attemptLoadCoords );
	case TYP_PRMTOP: return LoadTopology_AMBER( topFile );
	default: break;
	}
	utils->Warning( "unsupported topology nature \"%s\" (%i)\n", NatureHelper( topNature ).toString(), topNature );
	return false;
}

bool CTopology :: LoadCoordinates( const char *crdFile, int crdNature )
{
	assert( atcount_ != 0 );

	if ( !coords_base_ )
		coords_base_ = reinterpret_cast<coord3_t*>( utils->Alloc( sizeof(coord3_t)*atcount_ ) );

	console->Print( "Loading: \"%s\"...\n", crdFile );

	switch ( crdNature ) {
	case TYP_PDB: return LoadCoordinates_PDB( crdFile, coords_base_ );
	case TYP_INPCRD: return LoadCoordinates_AMBER( crdFile, coords_base_ );
	default: break;
	}
	utils->Warning( "unsupported coordinate nature \"%s\" (%i)\n", NatureHelper( crdNature ).toString(), crdNature );
	return false;
}

uint32 CTopology :: ProcessTrajectory( const char *trajFile, int trajNature, size_t firstSnap, TrajectoryCallback_t func, bool pacifier )
{
	assert( atcount_ != 0 );
	assert( func != nullptr );

	for ( int i = 0; i < ThreadCount(); ++i ) {
		if ( !coords_traj_[i] )
			coords_traj_[i] = reinterpret_cast<coord3_t*>( utils->Alloc( sizeof(coord3_t)*atcount_ ) );
	}

	console->Print( "Loading: \"%s\"...\n", trajFile );

	switch ( trajNature ) {
	case TYP_LIST: return ProcessTrajectory_PDB( trajFile, firstSnap, func, pacifier );
	case TYP_MDCRD: return ProcessTrajectory_AMBER( trajFile, firstSnap, func, pacifier );
	default: break;
	}
	utils->Warning( "unsupported trajectory nature \"%s\" (%i)\n", NatureHelper( trajNature ).toString(), trajNature );
	return 0;
}

void CTopology :: PostProcess()
{
	// assign atomic flags and correct names for terminal aminoacids
	const name_t cAtName_Protein_Backbone0 = { { 'C', ' ', ' ', ' ' } };
	const name_t cAtName_Protein_Backbone1 = { { 'C', 'A', ' ', ' ' } };
	const name_t cAtName_Protein_Backbone2 = { { 'N', ' ', ' ', ' ' } };
	const name_t cAtName_Protein_Backbone3 = { { 'O', ' ', ' ', ' ' } };
	const name_t cAtName_Protein_CTermMark = { { 'O', 'X', 'T', ' ' } };
	const name_t cAtName_Protein_NTermHyd0 = { { 'H', '1', ' ', ' ' } };
	const name_t cAtName_Protein_NTermHyd1 = { { 'H', '2', ' ', ' ' } };
	const name_t cAtName_Protein_NTermHyd2 = { { 'H', '3', ' ', ' ' } };
	const name_t cAtName_Nucleic_Backbone0 = { { 'O', '5', '\'', ' ' } };
	const name_t cAtName_Nucleic_Backbone1 = { { 'C', '5', '\'', ' ' } };
	const name_t cAtName_Nucleic_Backbone2 = { { 'C', '4', '\'', ' ' } };
	const name_t cAtName_Nucleic_Backbone3 = { { 'O', '3', '\'', ' ' } };
	const name_t cAtName_Nucleic_Backbone4 = { { 'C', '3', '\'', ' ' } };
	const uint32 c_full_protein_mask = 15;	// -----1111
	const uint32 c_nterm_protein_mask = 15;
	const uint32 c_full_nucleic_mask = 496;	// 111110000
	const uint32 c_solvent_mask = BIT( 31 );

	uint32 mask = 0, cterm = 0, nmask = 0, old_resname = 0, old_resnum = 0, old_chainnum = 0, oldold_chainnum = 0;
	uint32 start_atom = 0, solventatoms__per_residue = 0;

	name_t solvent_residue_name = { { ' ', ' ', ' ', ' ' } };
	for ( int i = 0; i < 4; ++i ) {
		if ( !gpGlobals->solvent_title[i] )
			break;
		solvent_residue_name.string[i] = gpGlobals->solvent_title[i];
	}

	logfile->Print( "Post-processing topology...\n" );
	
	uint32 atcount32 = static_cast<uint32>( atcount_ );
	atom_t *at = atoms_;
	name_t xres = { { 0, 0, 0, 0 } };
	for ( uint32 i = 0; i < (uint32)atcount_; ++i, ++at ) {
		bool is_solvent = false;
		if ( at->xtitle.string[0] == 'H' )
			at->flags |= AF_HYDROGEN;
		if ( old_resnum != at->resnum ) {
			if ( old_resname ) {
				if ( mask == c_full_protein_mask ) {
					const bool addc = cterm || is_solvent || old_chainnum != at->chainnum;
					const bool addn = nmask == c_nterm_protein_mask || oldold_chainnum != at->chainnum;
					for ( uint32 j = start_atom; j < i; ++j ) {
						atoms_[j].flags |= AF_PROTEIN;
						atoms_[j].rfirst = start_atom;
						atoms_[j].rcount = i - start_atom;
						atoms_[j].xresidue.integer = xres.integer;
						atoms_[j].restopo.integer = xres.integer;
						if ( addc ) {
							atoms_[j].flags |= AF_AMINOCT;
							if ( atoms_[j].restopo.string[3] != 'C' )
								atoms_[j].restopo.string[3] = 'C';
						} else if ( addn ) {
							atoms_[j].flags |= AF_AMINONT;
							if ( atoms_[j].restopo.string[3] != 'N' )
								atoms_[j].restopo.string[3] = 'N';
						}
					}
				} else if ( mask == c_full_nucleic_mask ) {
					for ( uint32 j = start_atom; j < i; ++j ) {
						atoms_[j].flags |= AF_NUCLEIC;
						atoms_[j].rfirst = start_atom;
						atoms_[j].rcount = i - start_atom;
						atoms_[j].xresidue.integer = xres.integer;
						atoms_[j].restopo.integer = xres.integer;
					}
				} else if ( mask == c_solvent_mask ) {
					uint32 at_count = i - start_atom;
					if ( solventatoms__per_residue == 0 )
						solventatoms__per_residue = at_count;
					else if ( solventatoms__per_residue != at_count )
						utils->Fatal( "Invalid atom count in solvent residue %u (%u should be %u)!\n",
									  atoms_[start_atom].resnum, (uint32)at_count, (uint32)solventatoms__per_residue );
					for ( uint32 j = start_atom; j < i; ++j ) {
						atoms_[j].flags |= AF_SOLVENT;
						atoms_[j].rfirst = start_atom;
						atoms_[j].rcount = i - start_atom;
						atoms_[j].xresidue.integer = xres.integer;
						atoms_[j].restopo.integer = xres.integer;
						atoms_[j].chainid = 'W';
					}
				} else {
					for ( uint32 j = start_atom; j < i; ++j ) {
						atoms_[j].rfirst = start_atom;
						atoms_[j].rcount = i - start_atom;
						atoms_[j].xresidue.integer = xres.integer;
						atoms_[j].restopo.integer = xres.integer;
					}
				}
			}
			old_resname = at->xresidue.integer;
			old_resnum = at->resnum;
			oldold_chainnum = old_chainnum;
			old_chainnum = at->chainnum;
			start_atom = i;
			mask = 0;
			nmask = 0;
			cterm = 0;
			auto it = solventMap_.find( old_resname );
			if ( it != solventMap_.cend() && it->second == solvent_residue_name.integer ) {
				xres.integer = solvent_residue_name.integer;
				is_solvent = true;
			} else xres.integer = old_resname;
		}
		if ( is_solvent ) mask = c_solvent_mask;
		if ( mask == c_solvent_mask )
			continue;
		if ( at->xtitle.integer == cAtName_Protein_CTermMark.integer ) cterm = 1;
		else if ( at->xtitle.integer == cAtName_Protein_Backbone0.integer ) mask |= BIT( 0 );
		else if ( at->xtitle.integer == cAtName_Protein_Backbone1.integer ) mask |= BIT( 1 );
		else if ( at->xtitle.integer == cAtName_Protein_Backbone2.integer ) { mask |= BIT( 2 ); nmask |= BIT( 0 ); }
		else if ( at->xtitle.integer == cAtName_Protein_Backbone3.integer ) mask |= BIT( 3 );
		else if ( at->xtitle.integer == cAtName_Nucleic_Backbone0.integer ) mask |= BIT( 4 );
		else if ( at->xtitle.integer == cAtName_Nucleic_Backbone1.integer ) mask |= BIT( 5 );
		else if ( at->xtitle.integer == cAtName_Nucleic_Backbone2.integer ) mask |= BIT( 6 );
		else if ( at->xtitle.integer == cAtName_Nucleic_Backbone3.integer ) mask |= BIT( 7 );
		else if ( at->xtitle.integer == cAtName_Nucleic_Backbone4.integer ) mask |= BIT( 8 );
		else if ( at->xtitle.integer == cAtName_Protein_NTermHyd0.integer ) nmask |= BIT( 1 );
		else if ( at->xtitle.integer == cAtName_Protein_NTermHyd1.integer ) nmask |= BIT( 2 );
		else if ( at->xtitle.integer == cAtName_Protein_NTermHyd2.integer ) nmask |= BIT( 3 );
	}
	if ( mask == c_full_protein_mask ) {
		for ( uint32 j = start_atom; j < atcount32; ++j ) {
			atoms_[j].flags |= AF_PROTEIN | AF_AMINOCT;
			atoms_[j].rfirst = start_atom;
			atoms_[j].rcount = atcount32 - start_atom;
			atoms_[j].xresidue.integer = xres.integer;
			atoms_[j].restopo.integer = xres.integer;
			if ( atoms_[j].restopo.string[3] != 'C' )
				atoms_[j].restopo.string[3] = 'C';
		}
	} else if ( mask == c_full_nucleic_mask ) {
		for ( uint32 j = start_atom; j < atcount32; ++j ) {
			atoms_[j].flags |= AF_NUCLEIC;
			atoms_[j].rfirst = start_atom;
			atoms_[j].rcount = atcount32 - start_atom;
			atoms_[j].xresidue.integer = xres.integer;
			atoms_[j].restopo.integer = xres.integer;
		}
	} else if ( mask == c_solvent_mask ) {
		uint32 at_count = atcount32 - start_atom;
		if ( solventatoms__per_residue == 0 )
			solventatoms__per_residue = at_count;
		else if ( solventatoms__per_residue != at_count )
			utils->Fatal( "Invalid atom count in solvent residue %u (%u should be %u)!\n",
			atoms_[start_atom].resnum, (uint32)at_count, (uint32)solventatoms__per_residue );
		for ( uint32 j = start_atom; j < atcount32; ++j ) {
			atoms_[j].flags |= AF_SOLVENT;
			atoms_[j].rfirst = start_atom;
			atoms_[j].rcount = atcount32 - start_atom;
			atoms_[j].xresidue.integer = xres.integer;
			atoms_[j].restopo.integer = xres.integer;
			atoms_[j].chainid = 'W';
		}
	} else {
		for ( uint32 j = start_atom; j < atcount32; ++j ) {
			atoms_[j].rfirst = start_atom;
			atoms_[j].rcount = atcount32 - start_atom;
			atoms_[j].xresidue.integer = xres.integer;
			atoms_[j].restopo.integer = xres.integer;
		}
	}

	if ( !solventatoms__per_residue )
		utils->Fatal( "Source structure contains no solvent atoms!\n" );

	solvsize_ = solventatoms__per_residue;

	// assign VdW radii
	logfile->Print( "Assigning VdW radii...\n" );
	const real tol = real( 1.0 ) - gpGlobals->vdw_tolerance;
	at = atoms_;
	for ( size_t i = 0; i < atcount_; ++i, ++at ) {
		if ( !( at->flags & AF_SOLVENT ) )
			continue;
		uint64 key = at->restopo.integer;
		key = ( key << uint64( 32 ) ) | at->xtitle.integer;
		auto it = radiusMap_.find( key );
		if ( radiusMap_.cend() == it ) {
			utils->Warning( "can't assign VdW radius for %c%c%c%c %c%c%c%c!\n", 
				at->restopo.string[0], at->restopo.string[1], at->restopo.string[2], at->restopo.string[3],
				at->xtitle.string[0], at->xtitle.string[1], at->xtitle.string[2], at->xtitle.string[3] );
			continue;
		}
		at->radius = it->second * tol;
	}

	// assign charges
	if ( !chargeok_ && gpGlobals->electrostatic_coeff > 0 ) {
		logfile->Print( "Assigning partial charges...\n" );
		at = atoms_;
		for ( size_t i = 0; i < atcount_; ++i, ++at ) {
			uint64 key = at->restopo.integer;
			key = ( key << uint64( 32 ) ) | at->xtitle.integer;
			auto it = chargeMap_.find( key );
			if ( chargeMap_.cend() == it ) {
				utils->Warning( "can't assign partial charge for %c%c%c%c %c%c%c%c!\n", 
								at->restopo.string[0], at->restopo.string[1], at->restopo.string[2], at->restopo.string[3],
								at->xtitle.string[0], at->xtitle.string[1], at->xtitle.string[2], at->xtitle.string[3] );
				continue;
			}
			at->charge = it->second;
		}
	}
}

void CTopology :: Print() const
{
	assert( atoms_ != nullptr );
	assert( coords_base_ != nullptr );

	const atom_t *top = atoms_;
	const coord3_t *crd = coords_base_;
	uint16 chain_num = 1;

	// print full topology with initial coordinates
	logfile->Print( "--------------------------- SOURCE TOPOLOGY ---------------------------\n" );
	for ( size_t i = 0; i < atcount_; ++i, ++top, ++crd ) {
		if ( i && top->chainnum != chain_num ) {
			logfile->Print( "%-6s%5u\n", "TER", (top-1)->serial );
			chain_num = top->chainnum;
		}
		if ( top->flags & AF_SOLVENT ) {
			logfile->Print( "%-6s%5u %c%c%c%c %c%c%c%c %c%4u    %8.3f%8.3f%8.3f %16.8e %4u %8.4f\n", "ATOM", 
				top->serial, 
				top->title.string[0], top->title.string[1], top->title.string[2], top->title.string[3],
				top->restopo.string[0], top->restopo.string[1], top->restopo.string[2], top->restopo.string[3],
				top->chainid, top->resnum,
				crd->x, crd->y, crd->z, top->charge, top->flags, top->radius );
		} else {
			logfile->Print( "%-6s%5u %c%c%c%c %c%c%c%c %c%4u    %8.3f%8.3f%8.3f %16.8e %4u\n", "ATOM", 
				top->serial, 
				top->title.string[0], top->title.string[1], top->title.string[2], top->title.string[3],
				top->restopo.string[0], top->restopo.string[1], top->restopo.string[2], top->restopo.string[3],
				top->chainid, top->resnum,
				crd->x, crd->y, crd->z, top->charge, top->flags );
		}
	}
	logfile->Print( "-----------------------------------------------------------------------\n" );
	logfile->Print( "TOTAL: %u atom(s), %u chain(s)\n", (uint32)atcount_, chain_num + 1 );
	logfile->Print( "-----------------------------------------------------------------------\n" );
}

void CTopology :: CopyTrimmed( name_t *dst, const name_t *src ) const
{
	size_t dstpos = 0;
	const uint8 *srcbytes = reinterpret_cast<const uint8*>( src->string );
	uint8 *dstbytes = reinterpret_cast<uint8*>( dst->string );
	for ( size_t i = 0; i < 4; ++i, ++srcbytes ) {
		if ( *srcbytes > 32 ) dstbytes[dstpos++] = toupper( *srcbytes );
	}
	for ( size_t i = dstpos; i < 4; ++i )
		dstbytes[i] = ' ';
}

void CTopology :: ParseTopology_PDBLine( const char *line, atom_t *top ) const
{
	assert( top != nullptr );
	top->serial = utils->Atoi( line + 6, 5 );
	// check for BioPASED format
	if ( line[12] == ' ' && line[16] >= '0' && line[16] <= '9' )
		memcpy( top->title.string, &line[13], 4 );
	else
		memcpy( top->title.string, &line[12], 4 );
	CopyTrimmed( &top->xtitle, &top->title );
	memcpy( top->residue.string, &line[17], 3 );
	top->residue.string[3] = '\0';
	top->xresidue.integer = top->residue.integer;
	top->xresidue.string[3] = ' ';
	top->chainid = static_cast<uint8>( line[21] );
	top->resnum = utils->Atoi( line + 22, 4 );
	top->flags = 0;
}

void CTopology :: ParseCoordinates_PDBLine( const char *line, coord3_t *crd ) const
{
	assert( crd != nullptr );
	crd->x = utils->Atof( line + 30, 8 );
	crd->y = utils->Atof( line + 38, 8 );
	crd->z = utils->Atof( line + 46, 8 );
}

bool CTopology :: LoadTopology_PDB( const char *topFile, bool loadCoords )
{
	FILE *fp;
	char line[96];

	assert( atcount_ == 0 );
	assert( atoms_ == nullptr );

	chargeok_ = false;

	if ( fopen_s( &fp, topFile, "rb" ) )
		utils->Fatal( "failed to open \"%s\" for reading!\n", topFile );

	// count total number of atoms and size of remarks
	atcount_ = 0;
	remsize_ = 0;
	while ( fgets( line, sizeof(line), fp ) ) {
		if ( !strncmp( line, "ATOM  ", 6 ) || !strncmp( line, "HETATM", 6 ) )
			++atcount_;
		else if ( !strncmp( line, "REMARK", 6 ) )
			remsize_ += 80;
	}

	if ( !atcount_ )
		utils->Fatal( "\"%s\" doesn't contain any atoms!\n", topFile );

	rewind( fp );

	// allocate atoms
	atoms_ = reinterpret_cast<atom_t*>( utils->Alloc( sizeof(atom_t)*atcount_ ) );
	if ( loadCoords )
		coords_base_ = reinterpret_cast<coord3_t*>( utils->Alloc( sizeof(coord3_t)*atcount_ ) );
	if ( remsize_ ) {
		remarks_ = reinterpret_cast<char*>( utils->Alloc( remsize_ ) );
		memset( remarks_, ' ', remsize_ );
	}

	// parse atoms (and their coords, if needed)
	uint8 chain_num = 1;
	char *current_remark = remarks_;
	atom_t *current_atom = atoms_;
	coord3_t *current_coords = coords_base_;
	while ( fgets( line, sizeof(line), fp ) ) {
		if ( !strncmp( line, "ATOM  ", 6 ) || !strncmp( line, "HETATM", 6 ) ) {
			ParseTopology_PDBLine( line, current_atom );
			current_atom->chainnum = chain_num;
			++current_atom;
			if ( loadCoords ) {
				ParseCoordinates_PDBLine( line, current_coords );
				++current_coords;
			}
		} else if ( !strncmp( line, "TER   ", 6 ) ) {
			++chain_num;
		} else if ( !strncmp( line, "REMARK", 6 ) ) {
			const size_t linsize = std::min( size_t( 79 ), strlen( line ) );
			memcpy( current_remark, line, linsize );
			for ( size_t i = 0; i < linsize; ++i ) {
				if ( current_remark[i] < 32 ) current_remark[i] = ' ';
			}
			current_remark[79] = '\n';
			current_remark += 80;
		}
	}

	fclose( fp );
	return true;
}

bool CTopology :: LoadTopology_AMBER( const char *topFile )
{
	FILE *fp;
	char line[96];

	assert( atcount_ == 0 );
	assert( atoms_ == nullptr );

	chargeok_ = false;

	if ( fopen_s( &fp, topFile, "rb" ) )
		utils->Fatal( "failed to open \"%s\" for reading!\n", topFile );

	// get number of atoms and residues
	atcount_ = 0;
	size_t rescount = 0;
	while ( fgets( line, sizeof(line), fp ) ) {
		if ( *line != '%' ) continue;
		if ( !strncmp( line, "%FLAG POINTERS", 14 ) ) {
			fgets( line, sizeof(line), fp );
			if ( strncmp( line, "%FORMAT(10I8)", 13 ) )
				utils->Fatal( "invalid FORMAT in section POINTERS in \"%s\"!\n", topFile );
			fgets( line, sizeof(line), fp );
			atcount_ = utils->Atoi( line, 8 );
			fgets( line, sizeof(line), fp );
			rescount = utils->Atoi( line + 8, 8 );
			break;
		}
	}

	if ( !atcount_ )
		utils->Fatal( "\"%s\" doesn't contain any atoms!\n", topFile );
	if ( !rescount )
		utils->Fatal( "\"%s\" doesn't contain any residues!\n", topFile );

	// allocate atoms
	atoms_ = reinterpret_cast<atom_t*>( utils->Alloc( sizeof(atom_t)*atcount_ ) );

	// allocate temporary residue titles
	name_t *resnames = reinterpret_cast<name_t*>( utils->Alloc( sizeof(name_t)*rescount ) );

	// read atom info
	while ( fgets( line, sizeof(line), fp ) ) {
		if ( *line != '%' ) continue;
		if ( !strncmp( line, "%FLAG ATOM_NAME", 15 ) ) {
			// parse atom titles
			fgets( line, sizeof(line), fp );
			if ( strncmp( line, "%FORMAT(20a4)", 13 ) )
				utils->Fatal( "invalid FORMAT in section ATOM_NAME in \"%s\"!\n", topFile );
			atom_t *at = atoms_;
			for ( size_t i = 0; i < atcount_; ++i, ++at ) {
				const size_t col = i % 20;
				if ( !col ) fgets( line, sizeof(line), fp );
				at->serial = static_cast<uint32>( i + 1 );
				memcpy( at->title.string, line + 4*col, 4 );
				CopyTrimmed( &at->xtitle, &at->title );
			}
		} else if ( !strncmp( line, "%FLAG CHARGE", 12 ) && gpGlobals->read_charges ) {
			// parse atom charges
			fgets( line, sizeof(line), fp );
			if ( strncmp( line, "%FORMAT(5E16.8)", 15 ) )
				utils->Fatal( "invalid FORMAT in section CHARGE in \"%s\"!\n", topFile );
			atom_t *at = atoms_;
			for ( size_t i = 0; i < atcount_; ++i, ++at ) {
				const size_t col = i % 5;
				if ( !col ) fgets( line, sizeof(line), fp );
				at->charge = utils->Atof( line + 16*col, 16 );
			}
			chargeok_ = true;
		} else if ( !strncmp( line, "%FLAG RESIDUE_LABEL", 19 ) ) {
			// parse residue names
			fgets( line, sizeof(line), fp );
			if ( strncmp( line, "%FORMAT(20a4)", 13 ) )
				utils->Fatal( "invalid FORMAT in section RESIDUE_LABEL in \"%s\"!\n", topFile );
			name_t *res = resnames;
			for ( size_t i = 0; i < rescount; ++i, ++res ) {
				const size_t col = i % 20;
				if ( !col ) fgets( line, sizeof(line), fp );
				for ( size_t i = 0; i < 4; ++i )
					res->string[i] = toupper( line[4*col+i] );
				if ( res->string[3] == ' ' ) res->string[3] = '\0';
			}
		} else if ( !strncmp( line, "%FLAG RESIDUE_POINTER", 21 ) ) {
			// assign residue names to atoms
			fgets( line, sizeof(line), fp );
			if ( strncmp( line, "%FORMAT(10I8)", 13 ) )
				utils->Fatal( "invalid FORMAT in section RESIDUE_POINTER in \"%s\"!\n", topFile );
			name_t *prevres, *res = resnames;
			size_t prev = 0, current;
			int resid = 1;
			for ( size_t i = 0; i < rescount; ++i, ++res ) {
				const size_t col = i % 10;
				if ( !col ) fgets( line, sizeof(line), fp );
				current = utils->Atoi( line + 8*col, 8 );
				assert( current > prev );
				if ( prev ) {
					for ( size_t i = prev; i < current; ++i ) {
						atoms_[i-1].chainid = 'A';
						atoms_[i-1].chainnum = 1;
						atoms_[i-1].resnum = resid;
						atoms_[i-1].residue.integer = prevres->integer;
						atoms_[i-1].xresidue.integer = prevres->integer;
						if ( prevres->string[3] == '\0' ) atoms_[i-1].xresidue.string[3] = ' ';
					}
					++resid;
				}
				prev = current;
				prevres = res;
			}
			if ( prev ) {
				for ( size_t i = prev; i <= atcount_; ++i ) {
					atoms_[i-1].chainid = 'A';
					atoms_[i-1].chainnum = 1;
					atoms_[i-1].resnum = resid;
					atoms_[i-1].residue.integer = prevres->integer;
					atoms_[i-1].xresidue.integer = prevres->integer;
					if ( prevres->string[3] == '\0' ) atoms_[i-1].xresidue.string[3] = ' ';
				}
			}
		}
	}

	utils->Free( resnames );

	fclose( fp );
	return true;
}

bool CTopology :: LoadCoordinates_PDB( const char *crdFile, coord3_t *out_coords )
{
	FILE *fp;
	char line[96];

	assert( atcount_ != 0 );
	assert( out_coords != nullptr );

	if ( fopen_s( &fp, crdFile, "rb" ) )
		utils->Fatal( "failed to open \"%s\" for reading!\n", crdFile );

	// parse coords
	coord3_t *current_coords = out_coords;
	size_t counter = 0;
	while ( fgets( line, sizeof(line), fp ) ) {
		if ( !strncmp( line, "ATOM  ", 6 ) || !strncmp( line, "HETATM", 6 ) ) {
			if ( counter++ == atcount_ )
				utils->Fatal( "topology and coordinate files have different atom counts!\n" );
			ParseCoordinates_PDBLine( line, current_coords );
			++current_coords;
		}
	}
	
	fclose( fp );
	return true;
}

bool CTopology :: LoadCoordinates_AMBER( const char *crdFile, coord3_t *out_coords )
{
	FILE *fp;
	char line[96];

	assert( atcount_ != 0 );
	assert( out_coords != nullptr );

	if ( fopen_s( &fp, crdFile, "rb" ) )
		utils->Fatal( "failed to open \"%s\" for reading!\n", crdFile );

	// skip header
	fgets( line, sizeof(line), fp );

	// check number of atoms
	fgets( line, sizeof(line), fp );
	if ( utils->Atoi( line ) != (int)atcount_ )
		utils->Fatal( "topology and coordinate files have different atom counts!\n" );

	// parse coords
	coord3_t *current_coords = out_coords;
	for ( size_t i = 0; i < atcount_; ++i, ++current_coords ) {
		const size_t col = i & 1;
		if ( !col ) fgets( line, sizeof(line), fp );
		current_coords->x = utils->Atof( line + 12*3*col, 12 );
		current_coords->y = utils->Atof( line + 12*3*col + 12, 12 );
		current_coords->z = utils->Atof( line + 12*3*col + 24, 12 );
	}

	fclose( fp );
	return true;
}

void CTopology :: ProcessTrajectoryThread_PDB( uint32 threadnum, uint32 num )
{
	trajItemPDB_t *item = &trajItems_[num];
	assert( item->pdbfile != nullptr );
	assert( coords_traj_[threadnum] != nullptr );

	if ( LoadCoordinates_PDB( item->pdbfile, coords_traj_[threadnum] ) )
		callback_( threadnum, num, coords_traj_[threadnum] );
}

uint32 CTopology :: ProcessTrajectory_PDB( const char *trajFile, size_t firstSnap, TrajectoryCallback_t func, bool pacifier )
{
	FILE *fp;
	char line[MAX_OSPATH];
	char trimline[MAX_OSPATH];
	char trajpath[MAX_OSPATH];
	char fullpath[MAX_OSPATH];
	const int runFlags = RF_PROGRESS | ( pacifier ? RF_PACIFIER : 0 );

	callback_ = func;

	if ( fopen_s( &fp, trajFile, "rb" ) )
		utils->Fatal( "failed to open \"%s\" for reading!\n", trajFile );

	utils->ExtractFilePath( trajpath, sizeof(trajpath), trajFile );
	if ( !0[trajpath] ) strcpy_s( trajpath, "." );

	uint32 snapshotNum = 0;
	size_t c = 0;

	while ( fgets( line, sizeof(line), fp ) ) {
		// skip empty lines
		if ( static_cast<uint8>( line[0] ) <= 32 )
			continue;
		// skip N first snapshots
		if ( c++ < firstSnap )
			continue;
		++snapshotNum;
	}

	trajItems_ = reinterpret_cast<trajItemPDB_t*>( utils->Alloc( sizeof(trajItemPDB_t) * snapshotNum ) );
	assert( trajItems_ != nullptr );

	rewind( fp );
	c = 0;

	trajItemPDB_t *curItem = trajItems_;
	while ( fgets( line, sizeof(line), fp ) ) {
		// skip empty lines
		if ( static_cast<uint8>( line[0] ) <= 32 )
			continue;
		// skip N first snapshots
		if ( c++ < firstSnap )
			continue;
		// get pdb name
		utils->Trim( trimline, sizeof(trimline), line );
		strcpy_s( fullpath, trajpath );
		strcat_s( fullpath, "/" );
		strcat_s( fullpath, trimline );
		curItem->pdbfile = utils->StrDup( fullpath );
		++curItem;
	}

	fclose( fp );

	// process the trajectory items
#if defined(_QTASSE)
	console->Print( "<b>%s:</b>\n", "ProcessTrajectory" );
#else
	console->Print( CC_WHITE "%s:\n", "ProcessTrajectory" );
#endif
	RunThreadsOnIndividual( snapshotNum, runFlags, Stub_ProcessTrajectoryThread_PDB );

	// free trajectory items
	curItem = trajItems_;
	for ( uint32 i = 0; i < snapshotNum; ++i, ++curItem ) {
		assert( curItem->pdbfile != nullptr );
		utils->Free( curItem->pdbfile );
	}
	utils->Free( trajItems_ );
	trajItems_ = nullptr;

	callback_ = nullptr;

	return ThreadInterrupted() ? 0 : snapshotNum;
}

void CTopology :: ProcessTrajectoryThread_AMBER( uint32 threadnum, uint32 num )
{
	assert( coords_traj_[threadnum] != nullptr );
	FILE *fp = file_traj_[threadnum];
	char *fb = frame_buf_[threadnum];
	char line[96];

	fu_seek( fp, framebase_ + framesize_ * num, SEEK_SET );
	const size_t readsize = fread( fb, 1, framesize_, fp );

	// parse coords
	bool eof = false;
	size_t framepos = 0;
	real *current_coords = &coords_traj_[threadnum]->x;
	for ( size_t i = 0; i < atcount_ && !eof; ++i ) {
		for ( size_t j = 0; j < 3; ++j, ++current_coords ) {
			const size_t col = ( i * 3 + j ) % 10;
			if ( !col ) {
				const size_t linebase = framepos;
				while ( fb[framepos] && fb[framepos] != '\n' ) ++framepos;
				if ( framepos >= readsize - 1 ) {
					eof = true;
					break;
				}
				const size_t linesize = std::min( framepos - linebase, sizeof(line)-1 );
				assert( linesize > 0 );
				if ( linesize == 0 ) {
					eof = true;
					break;
				}
				memcpy( line, fb + linebase, linesize );
				line[linesize] = '\0';
				++framepos;
			}
			*current_coords = utils->Atof( line + 8*col, 8 );
		}
	}

	if ( eof ) {
		logfile->Print( "EOF while parsing frame %u at pos %u/%u\n", num, (unsigned)framepos, (unsigned)framesize_ );
		utils->Fatal( "unexpected EOF in AMBER trajectory!\n" );
	}

	callback_( threadnum, num, coords_traj_[threadnum] );
}

uint32 CTopology :: ProcessTrajectory_AMBER( const char *trajFile, size_t firstSnap, TrajectoryCallback_t func, bool pacifier )
{
	FILE *fp;
	char line[96];
	const int runFlags = RF_PROGRESS | ( pacifier ? RF_PACIFIER : 0 );

	callback_ = func;

	if ( fopen_s( &fp, trajFile, "rb" ) )
		utils->Fatal( "failed to open \"%s\" for reading!\n", trajFile );

	// skip header
	fgets( line, sizeof(line), fp );

	// read the whole frame
	fileOfs_t frameSizeInLines = static_cast<fileOfs_t>( ( atcount_ * 3 + 9 ) / 10 );
	fileOfs_t frameStart = fu_tell( fp );
	for ( fileOfs_t i = 0; i < frameSizeInLines; ++i ) {
		if ( !fgets( line, sizeof(line), fp ) )
			utils->Fatal( "incomplete frame in \"%s\"!\n", trajFile );
	}
	fileOfs_t frameEnd = fu_tell( fp );
	// now read the next line and check if it has exactly 3 items
	// if that's the case, assume we have PBC enabled
	if ( fgets( line, sizeof(line), fp ) ) {
		float temp[4];
		if ( sscanf_s( line, "%f %f %f %f", &temp[0], &temp[1], &temp[2], &temp[3] ) == 3 ) {
			++frameSizeInLines;
			frameEnd = fu_tell( fp );
		}
	}
	// now we have the whole frame read
	// estimate its size in bytes
	fileOfs_t frameSizeInBytes = frameEnd - frameStart;

	// count total frames
	fu_seek( fp, 0, SEEK_END );
	fileOfs_t fileEnd = fu_tell( fp );
	fileOfs_t totalFrames = ( fileEnd - frameStart ) / frameSizeInBytes;
	assert( totalFrames > 0 );
	fclose( fp );
	uint32 snapshotNum = (uint32)( totalFrames - firstSnap );

	framebase_ = static_cast<size_t>( frameStart + frameSizeInBytes * firstSnap );
	framesize_ = static_cast<size_t>( frameSizeInBytes );

	// open a file for each thread
	int numthreads = ThreadCount();
	for ( int i = 0; i < numthreads; ++i ) {
		if ( fopen_s( &file_traj_[i], trajFile, "rb" ) )
			utils->Fatal( "failed to open \"%s\" for reading (thread %i)!\n", trajFile, i );
		frame_buf_[i] = reinterpret_cast<char*>( utils->Alloc( framesize_ ) );
	}

	// process the trajectory items
#if defined(_QTASSE)
	console->Print( "<b>%s:</b>\n", "ProcessTrajectory" );
#else
	console->Print( CC_WHITE "%s:\n", "ProcessTrajectory" );
#endif
	RunThreadsOnIndividual( snapshotNum, runFlags, Stub_ProcessTrajectoryThread_AMBER );

	// free trajectory items
	utils->Free( trajItems_ );
	trajItems_ = nullptr;

	// close a file for each thread
	for ( int i = 0; i < numthreads; ++i ) {
		fclose( file_traj_[i] );
		file_traj_[i] = nullptr;
		utils->Free( frame_buf_[i] );
		frame_buf_[i] = nullptr;
	}

	callback_ = nullptr;

	return ThreadInterrupted() ? 0 : snapshotNum;
}
