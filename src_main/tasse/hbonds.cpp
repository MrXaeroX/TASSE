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
#include <topology.h>
#include <hbonds.h>

#define DAF_PROTEIN		BIT( 0 )
#define DAF_NUCLEIC		BIT( 1 )
#define DAF_NTERM		BIT( 2 )
#define DAF_CTERM		BIT( 3 )

// Max hydrogen atoms per donor
#define MAX_H			4

// Marker for invalid unsigned values
#define UINT16_BAD		uint16( ~0 )
#define UINT32_BAD		uint32( ~0 )

// HBSolvent flags
#define HBSF_ALLOC		BIT( 0 )
#define HBSF_VALID		BIT( 1 )

typedef struct {
	name_t	rtitle;					// Residue title
	name_t	xtitle;					// Donor atom title
	name_t	htitle;					// Hydrogen atom title
	uint8	code;					// Triplet H-code
	uint8	group;					// Group index
	uint16	flags;					// Donor flags
} DonorInfo;

typedef struct {
	name_t	rtitle;					// Residue title
	name_t	ytitle;					// Acceptor atom title
	uint8	code;					// Triplet Y-code
	uint8	group;					// Group index
	uint16	flags;					// Acceptor flags
} AcceptorInfo;

class CHBonds : public IHBonds
{
	typedef struct {
		real			r;				// Rmin
		real			e;				// Em
		real			a;				// A12
		real			b;				// B6
	} TripletParms;

	typedef struct {
		uint32			xy_index;		// Donor/acceptor atom index
		uint32			xy_remap;		// Remapped donor/acceptor atom index (for grouping)
		uint32			r_index;		// Residue index
		uint16			group;			// Group index (0 = no grouping)
		uint16			y_code;			// FF code for acceptor (UINT32_BAD if not an acceptor)
		uint32			h_indices[MAX_H];	// Hydrogen atom indices for donor (UINT32_BAD means end-of-list)
		uint8			codes[MAX_H];	// FF codes for corresponding hydrogens
	} HBAtom;

	typedef struct {
		uint32			xy_remap;		// remapped donor/acceptor atom index for the group
		uint16			group;			// unique group index
		char			title[64];		// group title (constructed from group atoms titles)
	} HBGroup;

	typedef struct {
		uint32			s_index;		// Solvent atom index
		uint32			b_index;		// Biopolymer atom index
		real			energy;			// Energy value
	} HBBridge;

	typedef struct stHBSolvent {
		struct stHBSolvent *next;		// Next solvent atom info in thread chain
		struct stHBSolvent *chain;		// Next solvent atom info in score chain
		struct stHBSolvent *gblock;		// Precached pointer to the global block
		uint32			flags;			// Flags (HBSF_xxx)
		uint32			snaps;			// Number of snapshots where this solvent occurs
		uint32			s_index;		// Solvent atom index
		real			energy;			// Energy in the best position/orientation
		coord4_t		coords[1];		// Best coordinates for all atoms (variable sized)
	} HBSolvent;

	typedef struct {
		uint32			index0;			// First atom index of the pair
		uint32			index1;			// Second atom index of the pair
	} HBPair;

	typedef struct {
		uint32			index0;			// First atom index of the triplet
		uint32			index1;			// Second atom index of the triplet
		uint32			index2;			// Third atom index of the triplet
	} HBTriplet;

	typedef struct {
		uint32			score;			// Sum of local scores for this pair/triplet
		uint32			snaps;			// Number of snapshots where this pair/triplet occurs
		real			energy;			// Overall bonding energy
		HBSolvent		*solv;			// Solvent atoms chain
	} HBGlobalScore;

	typedef struct {
		uint32			score;			// Local score (i.e. number of solvent atoms that connect the pair/triplet)
		real			energy;			// Bonding energy
		HBSolvent		*solv;			// Solvent atoms chain
		HBGlobalScore	*global;		// Precached pointer to the score in global triplet map
	} HBLocalScore;

	typedef struct {
		uint32			index0;			// First atom index of the pair
		uint32			index1;			// Second atom index of the pair
		uint32			score;			// Final score
		real			occurence;		// Occurence in the trajectory, percentage (0-100)
		real			energy;			// Bonding energy averaged along the trajectory
	} HBFinalPair;

	typedef struct {
		uint32			index0;			// First atom index of the triplet
		uint32			index1;			// Second atom index of the triplet
		uint32			index2;			// Third atom index of the triplet
		uint32			score;			// Final score
		real			occurence;		// Occurence in the trajectory, percentage (0-100)
		real			energy;			// Bonding energy averaged along the trajectory
	} HBFinalTriplet;

	typedef struct {
		uint32			snaps;			// Number of snapshots where this solvent makes a bridge
		uint32			atindex;		// Index of atom of the solvent (arbitrary; to access residue info)
		real			energy;			// Energy in the best position/orientation
		coord4_t		coords[1];		// Best coordinates for all atoms (variable sized)
	} HBFinalSolvent;

	typedef struct {
		size_t			frames;
		real			minTime;
		real			maxTime;
		real			avgTime;
	} HBPerfCounter;

	typedef struct {
		HBPerfCounter	pcMicroset;
		HBPerfCounter	pcTuples;
		HBPerfCounter	pcTotal;
	} HBPerfCounters;

	typedef std::vector<HBBridge> HBBridgeVec;
	typedef std::map<uint32,HBGroup> HBGroupMap;
	typedef std::map<uint32,std::string> HBGroupTitleMap;
	typedef std::map<HBPair,HBLocalScore> HBPairMap;
	typedef std::map<HBTriplet,HBLocalScore> HBTripletMap;
	typedef std::map<HBPair,HBGlobalScore> HBPairScoreMap;
	typedef std::map<HBTriplet,HBGlobalScore> HBTripletScoreMap;
	typedef std::vector<HBFinalPair> HBFinalPairVec;
	typedef std::vector<HBFinalTriplet> HBFinalTripletVec;
	typedef std::map<uint32,HBFinalSolvent*> HBSolventMap;

	friend bool operator < ( const CHBonds::HBPair &x, const CHBonds::HBPair &y );
	friend bool operator < ( const CHBonds::HBTriplet &x, const CHBonds::HBTriplet &y );
	friend bool operator < ( const CHBonds::HBBridge &x, const CHBonds::HBBridge &y );
	friend bool operator < ( const CHBonds::HBFinalPair &x, const CHBonds::HBFinalPair &y );
	friend bool operator < ( const CHBonds::HBFinalTriplet &x, const CHBonds::HBFinalTriplet &y );

	typedef struct {
		HBBridgeVec		bridges;	// Thread-local bridge array (to avoid reallocations and therefore unnecessary syncs)
		HBSolvent		*solvData;	// Thread-local chain of allocated solvent data
		HBSolvent		*solvFree;	// Thread-local chain of free solvent data
	} ThreadLocal;

public:
	typedef std::map<std::string,int> TitleMap;
	typedef std::vector<DonorInfo> DonorInfoVec;
	typedef std::vector<AcceptorInfo> AcceptorInfoVec;
	typedef std::vector<HBAtom> HBAtomVec;

public:
	CHBonds();
	virtual	void Initialize();
	virtual	void Setup( uint32 numthreads );
	virtual void Clear();
	virtual void BuildAtomLists();
	virtual void CalcMicrosets( const uint32 threadNum, const uint32 snapshotNum, const coord3_t *coords );
	virtual void BuildFinalSolvent( uint32 totalFrames );
	virtual	bool PrintFinalTuples( uint32 totalFrames, const char *tupleFile ) const;
	virtual void PrintPerformanceCounters();

protected:
	void PrintInformation();
	real CalcEnergy( const HBAtom *atX, const HBAtom *atY, const atom_t *atoms, const coord3_t *coords ) const;
	real AverageEnergy( real e1, real e2 ) const;
	real AverageEnergy( real e1, real e2, real e3 ) const;
	void ProcessMicroset( ThreadLocal *tl, const HBBridge *firstBridge, size_t numBridges, const atom_t *atoms, const coord3_t *coords, HBPairMap &localPairMap, HBTripletMap &localTripletMap );
	void GetOutputAtomTitle( const atom_t *at, const uint32 index, char *outBuffer, size_t outBufferSize ) const;

	void AllocateSolventBlocks( ThreadLocal *tl, uint32 blockCount ) const;
	void DeallocateSolventBlocks( ThreadLocal *tl ) const;
	HBSolvent *GrabSolventBlock( ThreadLocal *tl ) const;
	HBSolvent *FindGlobalBlock( HBSolvent *block, HBSolvent *list ) const;
	void ReturnSolventBlock( ThreadLocal *tl, HBSolvent *block ) const;
	void BuildBlockInfo( HBSolvent *block, const atom_t *atoms, const coord3_t *coords ) const;
	void AddFinalSolventBlocks( HBSolventMap &finalSolventMap, atom_t *atoms, const HBSolvent *blocklist ) const;
	bool TestFinalSolventBlocks( const HBFinalSolvent *s1, const HBFinalSolvent *s2 ) const;

	void UpdatePerformanceCounter( HBPerfCounter *pc, real value ) const;
	void FinalizePerformanceCounters();

public:
	TitleMap			titleMapH_;
	TitleMap			titleMapY_;
	int					tripletMaxH_;
	int					tripletMaxY_;
	TripletParms		**tripletParms_;
	DonorInfoVec		donorInfo_;
	AcceptorInfoVec		acceptorInfo_;
	HBPerfCounters		perfCounters_;

private:
	HBAtomVec			hbBiopolyList_;
	HBAtomVec			hbSolventList_;
	HBGroupTitleMap		hbGroupTitleMap_;
	HBPairScoreMap		hbPairScoreMap_;
	HBTripletScoreMap	hbTripletScoreMap_;
	uint16				groupIndex_;
	uint32				numThreads_;
	ThreadLocal			tl_[MAX_THREADS];

	bool				init_;
	bool				group_bonds_;
	size_t				s_siz_;
	real				rc_sq_;
	real				crf_a_;
	real				crf_b_;
	real				dd_e_;
};

static CHBonds hbondsLocal;
IHBonds *hbonds = &hbondsLocal;

//////////////////////////////////////////////////////////////////////////

bool operator < ( const CHBonds::HBPair &x, const CHBonds::HBPair &y )
{
	return ( x.index0 == y.index0 ) ? ( x.index1 < y.index1 ) : ( x.index0 < y.index0 ); 
}

bool operator < ( const CHBonds::HBTriplet &x, const CHBonds::HBTriplet &y )
{
	return ( x.index0 == y.index0 ) ? 
			( ( x.index1 == y.index1 ) ? ( x.index2 < y.index2 ) : ( x.index1 < y.index1 ) ) 
			: ( x.index0 < y.index0 ); 
}

bool operator < ( const CHBonds::HBBridge &x, const CHBonds::HBBridge &y )
{
	return ( x.s_index == y.s_index ) ? ( x.b_index < y.b_index ) : ( x.s_index < y.s_index ); 
}

bool operator < ( const CHBonds::HBFinalPair &x, const CHBonds::HBFinalPair &y )
{
	return ( x.score == y.score ) ? 
			( ( x.occurence == y.occurence ) ? ( x.energy < y.energy ) : ( x.occurence > y.occurence ) ) 
			: ( x.score > y.score ); 
}

bool operator < ( const CHBonds::HBFinalTriplet &x, const CHBonds::HBFinalTriplet &y )
{
	return ( x.score == y.score ) ? 
			( ( x.occurence == y.occurence ) ? ( x.energy < y.energy ) : ( x.occurence > y.occurence ) ) 
			: ( x.score > y.score ); 
}

//////////////////////////////////////////////////////////////////////////

static void HBParms_GetDimensions( ICfgFile *cfg )
{
	int hCounter = static_cast<int>( hbondsLocal.titleMapH_.size() + 1 );
	int yCounter = static_cast<int>( hbondsLocal.titleMapY_.size() + 1 );

	while ( cfg->ParseToken( true ) ) {
		std::string strCode = cfg->GetToken();
		if ( strCode.length() > 0 && ( hbondsLocal.titleMapH_.end() == hbondsLocal.titleMapH_.find( strCode ) ) ) {
			hbondsLocal.titleMapH_.insert( std::make_pair( strCode, hCounter ) );
			++hCounter;
		}
		cfg->ParseToken( false );
		strCode = cfg->GetToken();
		if ( strCode.length() > 0 && ( hbondsLocal.titleMapY_.end() == hbondsLocal.titleMapY_.find( strCode ) ) ) {
			hbondsLocal.titleMapY_.insert( std::make_pair( strCode, yCounter ) );
			++yCounter;
		}
		cfg->SkipRestOfLine();
	}
	cfg->Rewind();
}

static void HBParms_FillParms( ICfgFile *cfg )
{
	while ( cfg->ParseToken( true ) ) {
		std::string hCode = cfg->GetToken();
		cfg->ParseToken( false );
		std::string yCode = cfg->GetToken();
		cfg->ParseToken( false );
		real fRmin = utils->Atof( cfg->GetToken() );
		cfg->ParseToken( false );
		real fEm = utils->Atof( cfg->GetToken() );
		cfg->SkipRestOfLine();
		auto hIt = hbondsLocal.titleMapH_.find( hCode );
		int hIndex = ( hIt == hbondsLocal.titleMapH_.end() ) ? 0 : hIt->second;
		if ( hIndex <= 0 || hIndex > hbondsLocal.tripletMaxH_ )
			continue;
		auto yIt = hbondsLocal.titleMapY_.find( yCode );
		int yIndex = ( yIt == hbondsLocal.titleMapY_.end() ) ? 0 : yIt->second ;
		if ( yIndex <= 0 || yIndex > hbondsLocal.tripletMaxY_ )
			continue;
		hbondsLocal.tripletParms_[hIndex-1][yIndex-1].r = fRmin;
		hbondsLocal.tripletParms_[hIndex-1][yIndex-1].e = fEm;
	}
	cfg->Rewind();
}

static void HBParms_LoadDonorsAndAcceptors( ICfgFile *cfg )
{
	name_t currentRes, currentXY, currentH;
	int defType;

	while ( cfg->ParseToken( true ) ) {
		// get residue
		std::string resname = cfg->GetToken();
		if ( !resname.compare( "ANY" ) ) {
			currentRes.integer = 0;
		} else {
			size_t resnamelen = resname.length();
			memcpy( currentRes.string, resname.c_str(), resnamelen );
			for ( size_t i = resnamelen; i < 4; ++i ) currentRes.string[i] = ' ';
		}
		// get D/A code
		cfg->ParseToken( false );
		const char *dacode = cfg->GetToken();
		switch ( *dacode ) {
		case 'D':	defType = 1; break;
		case 'A':	defType = 2; break;
		default:	defType = 0; break;
		}
		if ( !defType ) {
			logfile->Warning( "unknown D/A code \"%s\" in \"%s\"!\n", dacode, resname.c_str() );
			cfg->SkipRestOfLine();
			continue;
		}
		// get XY atom name
		cfg->ParseToken( false );
		std::string xyname = cfg->GetToken();
		size_t namelen = xyname.length();
		memcpy( currentXY.string, xyname.c_str(), namelen );
		for ( size_t i = namelen; i < 4; ++i ) currentXY.string[i] = ' ';
		// get H atom name
		cfg->ParseToken( false );
		std::string hname = cfg->GetToken();
		if ( defType == 2 && hname.compare( "-" ) )
			cfg->UnparseToken();
		if ( defType == 1 ) {
			namelen = hname.length();
			memcpy( currentH.string, hname.c_str(), namelen );
			for ( size_t i = namelen; i < 4; ++i ) currentH.string[i] = ' ';
		} else
			currentH.integer = 0;
		// get FF code
		cfg->ParseToken( false );
		std::string ffname = cfg->GetToken();
		int ffCode;
		if ( defType == 1 ) {
			auto it = hbondsLocal.titleMapH_.find( ffname );
			ffCode = ( it == hbondsLocal.titleMapH_.end() ) ? -1 : ( it->second - 1 );
		} else {
			auto it = hbondsLocal.titleMapY_.find( ffname );
			ffCode = ( it == hbondsLocal.titleMapY_.end() ) ? -1 : ( it->second - 1 );
		}
		if ( ffCode < 0 ) {
			utils->Warning( "unknown FF name \"%s\" in \"%s\"!\n", ffname.c_str(), resname.c_str() );
			cfg->SkipRestOfLine();
			continue;
		}
		// get flags
		uint32 currentFlags = 0;
		uint32 currentGroup = 0;
		while ( cfg->TokenAvailable() ) {
			cfg->ParseToken( false );
			const char *flagvalue = cfg->GetToken();
			if ( !_stricmp( flagvalue, "aa" ) ) {
				currentFlags |= DAF_PROTEIN;
			} else if ( !_stricmp( flagvalue, "na" ) ) {
				currentFlags |= DAF_NUCLEIC;
			} else if ( !_stricmp( flagvalue, "nt" ) ) {
				currentFlags |= DAF_NTERM;
			} else if ( !_stricmp( flagvalue, "ct" ) ) {
				currentFlags |= DAF_CTERM;
			} else if ( flagvalue[0] == 'g' || flagvalue[0] == 'G' ) {
				if ( gpGlobals->group_bonds )
					currentGroup = 1 + utils->Atoi( flagvalue + 1 );
			} else {
				// skip unknown flag
				utils->Warning( "unknown flag \"%s\" in \"%s\"!\n", flagvalue, resname.c_str() );
			}
		}
		if ( defType == 1 ) {
			bool duplicate = false;
			for ( auto it = hbondsLocal.donorInfo_.cbegin(); it != hbondsLocal.donorInfo_.cend(); ++it ) {
				if ( it->flags != currentFlags )
					continue;
				if ( it->rtitle.integer && currentRes.integer && it->rtitle.integer != currentRes.integer )
					continue;
				if ( it->xtitle.integer != currentXY.integer )
					continue;
				if ( it->htitle.integer != currentH.integer )
					continue;
				utils->Warning( "duplicate donor definition for \"%s %s-%s\"!\n", resname.c_str(), xyname.c_str(), hname.c_str() );
				duplicate = true;
				break;
			}
			if ( !duplicate ) {
				CHBonds::DonorInfoVec::value_type di;
				di.rtitle = currentRes;
				di.xtitle = currentXY;
				di.htitle = currentH;
				di.code = ffCode;
				di.flags = currentFlags;
				di.group = currentGroup;
				hbondsLocal.donorInfo_.push_back( di );
			}
		} else {
			bool duplicate = false;
			for ( auto it = hbondsLocal.acceptorInfo_.cbegin(); it != hbondsLocal.acceptorInfo_.cend(); ++it ) {
				if ( it->flags != currentFlags )
					continue;
				if ( it->rtitle.integer && currentRes.integer && it->rtitle.integer != currentRes.integer )
					continue;
				if ( it->ytitle.integer != currentXY.integer )
					continue;
				utils->Warning( "duplicate acceptor definition for \"%s %s\"!\n", resname.c_str(), xyname.c_str() );
				duplicate = true;
				break;
			}
			if ( !duplicate ) {
				CHBonds::AcceptorInfoVec::value_type ai;
				ai.rtitle = currentRes;
				ai.ytitle = currentXY;
				ai.code = static_cast<uint8>( ffCode );
				ai.flags = currentFlags;
				ai.group = currentGroup;
				hbondsLocal.acceptorInfo_.push_back( ai );
			}
		}
	}
}

CHBonds :: CHBonds() : tripletMaxH_( 0 ), tripletMaxY_( 0 ), tripletParms_( nullptr ), groupIndex_( 0 ), numThreads_( 0 ),
					   init_( false ), group_bonds_( false ), s_siz_( 0 ), rc_sq_( 0 ), crf_a_( 0 ), crf_b_( 0 ), dd_e_( 0 )
{
	for ( size_t i = 0; i < MAX_THREADS; ++i )
		tl_[i].solvData = tl_[i].solvFree = nullptr;
}

void CHBonds :: AllocateSolventBlocks( ThreadLocal *tl, uint32 blockCount ) const
{
	assert( tl->solvFree == nullptr );
	assert( s_siz_ > 0 );
	const size_t blockSize = sizeof(HBSolvent) + ( s_siz_ - 1 ) * sizeof(coord4_t);

#if defined(_DEBUG)
	utils->Warning( "ALLOCATING %u solvent blocks! (%.1f kb)\n", blockCount, ( blockCount * blockSize ) / 1024.0 );
#endif

	HBSolvent *block = reinterpret_cast<HBSolvent*>( utils->Alloc( blockCount * blockSize ) );
	if ( !block )
		utils->Fatal( "failed to allocate %u solvent blocks!\n", blockCount );
	uint8 *block_data = reinterpret_cast<uint8*>( block ) + blockSize;
	HBSolvent *curr_block = block;
	for ( uint32 i = 0; i < blockCount-1; ++i, block_data += blockSize ) {
		HBSolvent *next_block = reinterpret_cast<HBSolvent*>( block_data );
		curr_block->next = next_block;
		curr_block->chain = next_block;
		curr_block = next_block;
	}
	block->flags = HBSF_ALLOC;

	curr_block->next = tl->solvData;
	curr_block->chain = tl->solvFree;

	tl->solvData = block;
	tl->solvFree = block;
}

void CHBonds :: DeallocateSolventBlocks( ThreadLocal *tl ) const
{
	std::vector<HBSolvent*> blockPtrList;
	blockPtrList.reserve( 32 );

	for ( auto block = tl->solvData; block; block = block->next )
		if ( block->flags & HBSF_ALLOC )
			blockPtrList.push_back( block );

	for ( auto it = blockPtrList.begin(); it != blockPtrList.end(); ++it  )
		utils->Free( *it );

	tl->solvData = tl->solvFree = nullptr;
}

CHBonds::HBSolvent *CHBonds :: GrabSolventBlock( ThreadLocal *tl ) const
{
	if ( !tl->solvFree )
		AllocateSolventBlocks( tl, 2048 );

	HBSolvent *block = tl->solvFree;
	assert( block != nullptr );
	tl->solvFree = block->chain;
	return block;
}

CHBonds :: HBSolvent *CHBonds :: FindGlobalBlock( HBSolvent *block, HBSolvent *list ) const
{
	assert( block != nullptr );
	assert( list != nullptr );
	for ( ; list; list = list->chain ) {
		if ( list->s_index == block->s_index )
			return list;
	}
	return nullptr;
}

void CHBonds :: ReturnSolventBlock( ThreadLocal *tl, HBSolvent *block ) const
{
	block->flags &= ~HBSF_VALID;
	block->chain = tl->solvFree;
	tl->solvFree = block;
}

void CHBonds :: BuildBlockInfo( HBSolvent *block, const atom_t *atoms, const coord3_t *coords ) const
{
	assert( block != nullptr );
	assert( 0 == ( block->flags & HBSF_VALID ) );
	const atom_t *at = &atoms[block->s_index];

	// store coordinates
	assert( at->rcount == s_siz_ );

	const atom_t *src_atom = &atoms[at->rfirst];
	const coord3_t *src_coord = &coords[at->rfirst];
	coord4_t *dst_coord = block->coords;
	for ( uint32 i = 0; i < at->rcount; ++i, ++src_atom, ++src_coord, ++dst_coord ) {
		dst_coord->x = src_coord->x;
		dst_coord->y = src_coord->y;
		dst_coord->z = src_coord->z;
		dst_coord->r = src_atom->radius;
	}

	block->flags |= HBSF_VALID;
}

void CHBonds :: Initialize()
{
	if ( !init_ ) {
		configs->ForEach( CONFIG_TYPE_HBPARMS, HBParms_GetDimensions );

		tripletMaxH_ = static_cast<int>( titleMapH_.size() );
		tripletMaxY_ = static_cast<int>( titleMapY_.size() );

		if ( !tripletMaxH_ || !tripletMaxY_ ) {
			// create dummy info
			titleMapH_["H"] = 1;
			titleMapY_["O"] = 1;
			tripletMaxH_ = 1;
			tripletMaxY_ = 1;
		}

		// allocate 2D array of triplet parms
		tripletParms_ = new TripletParms*[tripletMaxH_];
		for ( int i = 0; i < tripletMaxH_; ++i )
			tripletParms_[i] = new TripletParms[tripletMaxY_];

		configs->ForEach( CONFIG_TYPE_HBPARMS, HBParms_FillParms );

		// precompute A and B
		for ( int i = 0; i < tripletMaxH_; ++i ) {
			for ( int j = 0; j < tripletMaxY_; ++j ) {
				real r6 = tripletParms_[i][j].r * tripletParms_[i][j].r * tripletParms_[i][j].r;
				r6 *= r6;
				tripletParms_[i][j].a = tripletParms_[i][j].e * r6 * r6;
				tripletParms_[i][j].b = real( 2.0 ) * tripletParms_[i][j].e * r6;
				tripletParms_[i][j].e *= real( -1.0 );
			}
		}
	}

	if ( !init_ || group_bonds_ != gpGlobals->group_bonds ) {
		// update cached value
		group_bonds_ = gpGlobals->group_bonds;

		// clear info and reserve some space
		donorInfo_.clear();
		donorInfo_.reserve( 128 );
		acceptorInfo_.clear();
		acceptorInfo_.reserve( 128 );

		configs->ForEach( CONFIG_TYPE_HBRES, HBParms_LoadDonorsAndAcceptors );

		// dump parsed parms to the log file
		this->PrintInformation();
	}

	// calculate reaction field constants
	// Tironi, I. G., R. Sperb, et al. (1995). "A Generalized Reaction Field Method for Molecular Dynamics Simulations." J. Chem. Phys. 102(13): 5451-5459.
	// Walser, R., P. H. Hunenberger, et al. (2001). "Comparison of Different Schemes to Treat Long-Range Electrostatic Interactions in Molecular Dynamics Simulations of a Protein Crystal." Proteins 44(4): 509-519.
	const real ce = static_cast<real>( ( 1.0 - gpGlobals->dielectric_const ) / ( 1.0 + 2.0 * gpGlobals->dielectric_const ) );
	const real rc = gpGlobals->electrostatic_radius;
	rc_sq_ = rc * rc;
	crf_a_ = -ce / ( rc_sq_ * rc );
	crf_b_ = ( ce - 1 ) / rc;
	dd_e_ = real( 1.0 / 3.3 );	//!TODO: make tweakable?
	rc_sq_ = std::min( rc_sq_, gpGlobals->hbond_max_length * gpGlobals->hbond_max_length );

	logfile->Print( "CrfA = %12.8f\n", crf_a_ );
	logfile->Print( "CrfB = %12.8f\n", crf_b_ );

	init_ = true;
}

void CHBonds :: Setup( uint32 numthreads )
{
	// get solvent size in atoms
	s_siz_ = topology->GetSolventSize();

	// initialize multithreading stuff
	assert( numthreads > 0 && numthreads <= MAX_THREADS );
	numThreads_ = numthreads;
	ThreadLocal *tl = &tl_[0];
	for ( uint32 i = 0; i < numthreads; ++i, ++tl ) {
		tl->bridges.clear();
		tl->bridges.reserve( 1024 );
		if ( tl->solvData != nullptr ) {
			for ( auto block = tl->solvData; block; block = block->next ) {
				if ( block->flags & HBSF_VALID )
					ReturnSolventBlock( tl, block );
			}
			assert( tl_[i].solvFree != nullptr );
		}
		if ( tl->solvFree == nullptr )
			AllocateSolventBlocks( tl, 4096 );
	}

	// clear global data
	hbPairScoreMap_.clear();
	hbTripletScoreMap_.clear();

	// clear performance counters
	memset( &perfCounters_, 0, sizeof(perfCounters_) );
}

void CHBonds :: Clear()
{
	for ( uint32 i = 0; i < numThreads_; ++i )
		DeallocateSolventBlocks( &tl_[i] );

	groupIndex_ = 0;
	if ( tripletParms_ ) {
		for ( int i = 0; i < tripletMaxH_; ++i )
			delete [] tripletParms_[i];
		delete [] tripletParms_;
		tripletParms_ = nullptr;
	}

	titleMapH_.clear();
	titleMapY_.clear();

	init_ = false;
}

void CHBonds :: PrintInformation()
{
	uint32 counter;

	// print triplet parms
	logfile->Print( "---------------------------- TRIPLET PARMS ----------------------------\n" );
	logfile->Print( "    H-Code     Y-Code       Rmin         Em         A12          B6    \n" );
	logfile->Print( "-----------------------------------------------------------------------\n" );
	for ( auto hIt = titleMapH_.begin(); hIt != titleMapH_.end(); ++hIt ) {
		const int i = hIt->second - 1;
		for ( auto yIt = titleMapY_.begin(); yIt != titleMapY_.end(); ++yIt ) {
			const int j = yIt->second - 1;
			logfile->Print( "%6s (%i) %6s (%i) %10.3f %10.3f  %10.3f  %10.3f\n", 
				hIt->first.c_str(), i, yIt->first.c_str(), j,
				tripletParms_[i][j].r, tripletParms_[i][j].e,
				tripletParms_[i][j].a, tripletParms_[i][j].b );
		}
	}
	logfile->Print( "-----------------------------------------------------------------------\n" );

	// print donor info
	logfile->Print( "------------------------------ DONOR INFO -----------------------------\n" );
	logfile->Print( "Residue   Donor    Hydrogen  FF-Code    Flags                          \n" );
	logfile->Print( "-----------------------------------------------------------------------\n" );
	counter = 0;
	for ( auto it = donorInfo_.begin(); it != donorInfo_.end(); ++it ) {
		++counter;
		if ( it->rtitle.integer ) {
			logfile->Print( "%c%c%c%c      %c%c%c%c     %c%c%c%c     %8i %8i\n", 
				it->rtitle.string[0], it->rtitle.string[1], it->rtitle.string[2], it->rtitle.string[3], 
				it->xtitle.string[0], it->xtitle.string[1], it->xtitle.string[2], it->xtitle.string[3], 
				it->htitle.string[0], it->htitle.string[1], it->htitle.string[2], it->htitle.string[3], 
				it->code, it->flags );
		} else {
			logfile->Print( "%-8s  %c%c%c%c     %c%c%c%c     %8i %8i\n", 
				"(any)", 
				it->xtitle.string[0], it->xtitle.string[1], it->xtitle.string[2], it->xtitle.string[3], 
				it->htitle.string[0], it->htitle.string[1], it->htitle.string[2], it->htitle.string[3], 
				it->code, it->flags );
		}
	}
	logfile->Print( "-----------------------------------------------------------------------\n" );
	logfile->Print( "%u total\n", counter );

	// print acceptor info
	logfile->Print( "---------------------------- ACCEPTOR INFO ----------------------------\n" );
	logfile->Print( "Residue   Acceptor Hydrogen  FF-Code    Flags                          \n" );
	logfile->Print( "-----------------------------------------------------------------------\n" );
	counter = 0;
	for ( auto it = acceptorInfo_.begin(); it != acceptorInfo_.end(); ++it ) {
		++counter;
		if ( it->rtitle.integer ) {
			logfile->Print( "%c%c%c%c      %c%c%c%c     ----     %8i %8i\n", 
				it->rtitle.string[0], it->rtitle.string[1], it->rtitle.string[2], it->rtitle.string[3], 
				it->ytitle.string[0], it->ytitle.string[1], it->ytitle.string[2], it->ytitle.string[3], 
				it->code, it->flags );
		} else {
			logfile->Print( "%-8s  %c%c%c%c     ----     %8i %8i\n", 
				"(any)", 
				it->ytitle.string[0], it->ytitle.string[1], it->ytitle.string[2], it->ytitle.string[3], 
				it->code, it->flags );
		}
	}
	logfile->Print( "-----------------------------------------------------------------------\n" );
	logfile->Print( "%u total\n", counter );
}

void CHBonds :: BuildAtomLists()
{
	console->Print( "Building atom lists...\n" );

	hbGroupTitleMap_.clear();

	hbBiopolyList_.clear();
	hbBiopolyList_.reserve( 512 );
	hbSolventList_.clear();
	hbSolventList_.reserve( 1024 );

	const uint32 atcount = static_cast<uint32>( topology->GetAtomCount() );
	const atom_t *atoms = topology->GetAtomArray();
	HBGroupMap groupIndexMap;

	// allocate offsets for atoms
	uint32 *offsets = reinterpret_cast<uint32*>( utils->Alloc( atcount * sizeof(uint32) ) );
	memset( offsets, 0xff, atcount * sizeof(uint32) );

	double startTime = utils->FloatMilliseconds();
	//double baseTime = startTime;

	// build lists of biopolymer and solvent donors
	for ( auto it = donorInfo_.cbegin(); it != donorInfo_.cend(); ++it ) {
		// build atom mask to match
		uint16 atMask = 0;
		if ( it->flags & DAF_PROTEIN ) atMask |= AF_PROTEIN;
		if ( it->flags & DAF_NUCLEIC ) atMask |= AF_NUCLEIC;
		if ( it->flags & DAF_NTERM ) atMask |= AF_AMINONT;
		if ( it->flags & DAF_CTERM ) atMask |= AF_AMINOCT;
		const atom_t *at = atoms;
		uint32 resnum = 0, res_start = 0;
		for ( uint32 i = 0; i < atcount; ++i, ++at ) {
			if ( at->resnum != resnum ) {
				resnum = at->resnum;
				res_start = i;
			}
			// ignore hydrogens
			if ( at->flags & AF_HYDROGEN )
				continue;
			// check if mask matches
			if ( ( at->flags & atMask ) != atMask )
				continue;
			// check if residue name matches
			if ( it->rtitle.integer && it->rtitle.integer != at->xresidue.integer )
				continue;
			// check if atom name matches
			if ( it->xtitle.integer != at->xtitle.integer )
				continue;
			// we have a possible donor
			// find a corresponding hydrogen
			const atom_t *at2 = &atoms[res_start];
			uint32 hindex = atcount;
			for ( uint32 j = res_start; j < atcount && at2->resnum == resnum; ++j, ++at2 ) {
				if ( it->htitle.integer == at2->xtitle.integer ) {
					hindex = j;
					break;
				}
			}
			if ( hindex == atcount )
				continue;
			// we have a real donor
			// if we already registered the heavy atom, push hydrogen to its list
			// otherwise add new atom to the corresponding list
			HBAtomVec *pVec = ( at->flags & AF_SOLVENT ) ? &hbSolventList_ : &hbBiopolyList_;
			if ( offsets[i] != UINT32_BAD ) {
				HBAtom *existing = &pVec->at( offsets[i] );
				uint32 k;
				for ( k = 1; k < MAX_H; ++k ) {
					if ( existing->h_indices[k] == UINT32_BAD )
						break;
				}
				if ( k == MAX_H ) {
					utils->Warning( "too many hydrogens at donor: %c%c%c%c %c%c%c%c\n",
						at->xresidue.string[0], at->xresidue.string[1], at->xresidue.string[2], at->xresidue.string[3], 
						at->xtitle.string[0], at->xtitle.string[1], at->xtitle.string[2], at->xtitle.string[3] );
				} else {
					existing->h_indices[k] = hindex;
					existing->codes[k] = it->code;
				}
			} else {
				HBAtom da;
				memset( da.h_indices, 0xff, sizeof(da.h_indices) );
				da.xy_index = i;
				da.xy_remap = i;
				da.r_index = resnum;
				da.y_code = UINT16_BAD;
				da.h_indices[0] = hindex;
				da.codes[0] = it->code;
				da.group = 0;
				if ( it->group != 0 ) {
					uint32 groupIndexKey = ( static_cast<uint32>( it->group ) << 24 ) | resnum;
					auto gi = groupIndexMap.find( groupIndexKey );
					if ( gi == groupIndexMap.end() ) {
						HBGroup groupInfo;
						da.xy_remap = groupInfo.xy_remap = i;
						da.group = groupInfo.group = ++groupIndex_;
						memset( groupInfo.title, 0, sizeof(groupInfo.title) );
						for ( size_t k = 0; k < 4; ++k ) {
							if ( at->xtitle.string[k] == ' ' ) break;
							groupInfo.title[k] = at->xtitle.string[k];
						}
						groupIndexMap.insert( std::make_pair( groupIndexKey, groupInfo ) );
					} else {
						size_t k = 0, l = strlen( gi->second.title );
						const size_t max_l = sizeof(gi->second.title)-1;
						gi->second.title[l++] = '/';
						for ( ; k < 4 && l < max_l; ++k ) {
							if ( at->xtitle.string[k] == ' ' ) break;
							gi->second.title[l++] = at->xtitle.string[k];
						}
						da.group = gi->second.group;
						da.xy_remap = gi->second.xy_remap;
					}
				}
				offsets[i] = static_cast<uint32>( pVec->size() );
				pVec->push_back( da );
			}
		}
	}

	// register group titles and clear the map
	if ( groupIndexMap.size() != 0 ) {
		for ( auto it = groupIndexMap.cbegin(); it != groupIndexMap.cend(); ++it )
			hbGroupTitleMap_.insert( std::make_pair( it->second.xy_remap, std::string( it->second.title ) ) );
		groupIndexMap.clear();
	}

	double localDonorTime = utils->FloatMilliseconds() - startTime;
	startTime += localDonorTime;

	// build lists of biopolymer and solvent acceptors
	for ( auto it = acceptorInfo_.cbegin(); it != acceptorInfo_.cend(); ++it ) {
		// build atom mask to match
		uint16 atMask = 0;
		if ( it->flags & DAF_PROTEIN ) atMask |= AF_PROTEIN;
		if ( it->flags & DAF_NUCLEIC ) atMask |= AF_NUCLEIC;
		if ( it->flags & DAF_NTERM ) atMask |= AF_AMINONT;
		if ( it->flags & DAF_CTERM ) atMask |= AF_AMINOCT;
		const atom_t *at = atoms;
		for ( uint32 i = 0; i < atcount; ++i, ++at ) {
			// ignore hydrogens
			if ( at->flags & AF_HYDROGEN )
				continue;
			// check if mask matches
			if ( ( at->flags & atMask ) != atMask )
				continue;
			// check if residue name matches
			if ( it->rtitle.integer && it->rtitle.integer != at->xresidue.integer )
				continue;
			// check if atom name matches
			if ( it->ytitle.integer != at->xtitle.integer )
				continue;
			// we have an acceptor
			// if we already registered the heavy atom, assign acceptor code
			// otherwise add new atom to the corresponding list
			HBAtomVec *pVec = ( at->flags & AF_SOLVENT ) ? &hbSolventList_ : &hbBiopolyList_;
			if ( offsets[i] != UINT32_BAD ) {
				HBAtom *existing = &pVec->at( offsets[i] );
				existing->y_code = it->code;
				if ( it->group && !existing->group ) {
					uint32 groupIndexKey = ( static_cast<uint32>( it->group ) << 24 ) | at->resnum;
					auto gi = groupIndexMap.find( groupIndexKey );
					if ( gi == groupIndexMap.end() ) {
						HBGroup groupInfo;
						existing->xy_remap = groupInfo.xy_remap = i;
						existing->group = groupInfo.group = ++groupIndex_;
						memset( groupInfo.title, 0, sizeof(groupInfo.title) );
						for ( size_t k = 0; k < 4; ++k ) {
							if ( at->xtitle.string[k] == ' ' ) break;
							groupInfo.title[k] = at->xtitle.string[k];
						}
						groupIndexMap.insert( std::make_pair( groupIndexKey, groupInfo ) );
					} else {
						size_t k = 0, l = strlen( gi->second.title );
						const size_t max_l = sizeof(gi->second.title)-1;
						gi->second.title[l++] = '/';
						for ( ; k < 4 && l < max_l; ++k ) {
							if ( at->xtitle.string[k] == ' ' ) break;
							gi->second.title[l++] = at->xtitle.string[k];
						}
						existing->group = gi->second.group;
						existing->xy_remap = gi->second.xy_remap;
					}
				}
			} else {
				HBAtom aa;
				memset( aa.h_indices, 0xff, sizeof(aa.h_indices) );
				aa.xy_index = i;
				aa.xy_remap = i;
				aa.r_index = at->resnum;
				aa.y_code = it->code;
				aa.group = 0;
				if ( it->group != 0 ) {
					uint32 groupIndexKey = ( static_cast<uint32>( it->group ) << 24 ) | at->resnum;
					auto gi = groupIndexMap.find( groupIndexKey );
					if ( gi == groupIndexMap.end() ) {
						HBGroup groupInfo;
						aa.xy_remap = groupInfo.xy_remap = i;
						aa.group = groupInfo.group = ++groupIndex_;
						memset( groupInfo.title, 0, sizeof(groupInfo.title) );
						for ( size_t k = 0; k < 4; ++k ) {
							if ( at->xtitle.string[k] == ' ' ) break;
							groupInfo.title[k] = at->xtitle.string[k];
						}
						groupIndexMap.insert( std::make_pair( groupIndexKey, groupInfo ) );
					} else {
						size_t k = 0, l = strlen( gi->second.title );
						const size_t max_l = sizeof(gi->second.title)-1;
						gi->second.title[l++] = '/';
						for ( ; k < 4 && l < max_l; ++k ) {
							if ( at->xtitle.string[k] == ' ' ) break;
							gi->second.title[l++] = at->xtitle.string[k];
						}
						aa.group = gi->second.group;
						aa.xy_remap = gi->second.xy_remap;
					}
				}
				offsets[i] = static_cast<uint32>( pVec->size() );
				pVec->push_back( aa );
			}
		}
	}

	// register group titles and clear the map
	if ( groupIndexMap.size() != 0 ) {
		for ( auto it = groupIndexMap.cbegin(); it != groupIndexMap.cend(); ++it )
			hbGroupTitleMap_.insert( std::make_pair( it->second.xy_remap, std::string( it->second.title ) ) );
		groupIndexMap.clear();
	}

	double localAcceptorTime = utils->FloatMilliseconds() - startTime;
	startTime += localAcceptorTime;

	// free offsets (not needed anymore)
	utils->Free( offsets );

	// sort lists by residue name, then by group, then by xy index
	auto HBAtomLessThan = []( const HBAtom &a, const HBAtom &b ) { return ( a.r_index != b.r_index ) ? ( a.r_index < b.r_index ) : ( ( a.group != b.group ) ? ( a.group < b.group ) : ( a.xy_index < b.xy_index ) ); };
	std::sort( hbBiopolyList_.begin(), hbBiopolyList_.end(), HBAtomLessThan );
	std::sort( hbSolventList_.begin(), hbSolventList_.end(), HBAtomLessThan );

	double localSortTime = utils->FloatMilliseconds() - startTime;
	startTime += localSortTime;

	// sort internal hydrogens and print donor/acceptor lists
	uint32 cbd = 0, cba = 0, csd = 0, csa = 0;
//	logfile->Print( "------------------ BIOPOLYMER DONOR/ACCEPTOR LIST ------------------\n" );
	for ( auto it = hbBiopolyList_.begin(); it != hbBiopolyList_.end(); ++it ) {
		//const atom_t *at = &atoms[it->xy_index];
		//assert( at->resnum == it->r_index );
		if ( it->h_indices[0] != UINT32_BAD ) {
			++cbd;
			std::sort( &it->h_indices[0], &it->h_indices[MAX_H] );
			/*logfile->Print( "%-6s%5u %c%c%c%c %c%c%c%c %c%4u  group=%u\n", "DONOR", 
				at->serial, 
				at->title.string[0], at->title.string[1], at->title.string[2], at->title.string[3],
				at->restopo.string[0], at->restopo.string[1], at->restopo.string[2], at->restopo.string[3],
				at->chainid, at->resnum, it->group );
			for ( size_t i = 0; i < MAX_H && it->h_indices[i] != UINT32_BAD; ++i ) {
				const atom_t *at2 = &atoms[it->h_indices[i]];
				logfile->Print( "%-6s%5u %c%c%c%c %c%c%c%c %c%4u  code=%u\n", "", 
					at2->serial, 
					at2->title.string[0], at2->title.string[1], at2->title.string[2], at2->title.string[3],
					at2->restopo.string[0], at2->restopo.string[1], at2->restopo.string[2], at2->restopo.string[3],
					at2->chainid, at2->resnum, it->codes[i] );
			}*/
		}
		if ( it->y_code != UINT16_BAD ) {
			++cba;
			/*logfile->Print( "%-6s%5u %c%c%c%c %c%c%c%c %c%4u  code=%u  group=%u\n", "ACCEP", 
				at->serial, 
				at->title.string[0], at->title.string[1], at->title.string[2], at->title.string[3],
				at->restopo.string[0], at->restopo.string[1], at->restopo.string[2], at->restopo.string[3],
				at->chainid, at->resnum, it->y_code, it->group );*/
		}
	}
//	logfile->Print( "-------------------- SOLVENT DONOR/ACCEPTOR LIST -------------------\n" );
	for ( auto it = hbSolventList_.begin(); it != hbSolventList_.end(); ++it ) {
		//const atom_t *at = &atoms[it->xy_index];
		//assert( at->resnum == it->r_index );
		if ( it->h_indices[0] != UINT32_BAD ) {
			++csd;
			std::sort( &it->h_indices[0], &it->h_indices[MAX_H] );
			/*logfile->Print( "%-6s%5u %c%c%c%c %c%c%c%c %c%4u  group=%u\n", "DONOR", 
				at->serial, 
				at->title.string[0], at->title.string[1], at->title.string[2], at->title.string[3],
				at->restopo.string[0], at->restopo.string[1], at->restopo.string[2], at->restopo.string[3],
				at->chainid, at->resnum, it->group );
			for ( size_t i = 0; i < MAX_H && it->h_indices[i] != UINT32_BAD; ++i ) {
				const atom_t *at2 = &atoms[it->h_indices[i]];
				logfile->Print( "%-6s%5u %c%c%c%c %c%c%c%c %c%4u  code=%u\n", "", 
					at2->serial, 
					at2->title.string[0], at2->title.string[1], at2->title.string[2], at2->title.string[3],
					at2->restopo.string[0], at2->restopo.string[1], at2->restopo.string[2], at2->restopo.string[3],
					at2->chainid, at2->resnum, it->codes[i] );
			}*/
		}
		if ( it->y_code != UINT16_BAD ) {
			++csa;
			/*logfile->Print( "%-6s%5u %c%c%c%c %c%c%c%c %c%4u  code=%u  group=%u\n", "ACCEP", 
				at->serial, 
				at->title.string[0], at->title.string[1], at->title.string[2], at->title.string[3],
				at->restopo.string[0], at->restopo.string[1], at->restopo.string[2], at->restopo.string[3],
				at->chainid, at->resnum, it->y_code, it->group );*/
		}
	}
	//logfile->Print( "--------------------------------------------------------------------\n" );

	console->Print( "%6u hydrogen bond donors in biopolymer\n", cbd );
	console->Print( "%6u hydrogen bond acceptors in biopolymer\n", cba );
	console->Print( "%6u hydrogen bond total atoms in biopolymer\n", hbBiopolyList_.size() );

	console->Print( "%6u hydrogen bond donors in solvent\n", csd );
	console->Print( "%6u hydrogen bond acceptors in solvent\n", csa );
	console->Print( "%6u hydrogen bond total atoms in solvent\n", hbSolventList_.size() );
	console->Print( "%6u unique donor/acceptor groups\n", groupIndex_ );

#if 0
	logfile->Print( "------- timing -------\n"
					"%6.0f ms donor list\n"
					"%6.0f ms acceptor list\n"
					"%6.0f ms sorting\n"
					"%6.0f ms total time\n",
					localDonorTime, localAcceptorTime, localSortTime, startTime - baseTime );
#endif
}

real CHBonds :: CalcEnergy( const HBAtom *atX, const HBAtom *atY, const atom_t *atoms, const coord3_t *coords ) const
{
	assert( atX->xy_index != atY->xy_index );
	assert( atX->h_indices[0] != UINT32_BAD );
	assert( atY->y_code != UINT16_BAD );

	// value of -sigma^2 needed for 12-6 model
	const real hb_minus_sigma2 = real( -0.018 );

	// get x and y coords
	const coord3_t *crd_x = &coords[atX->xy_index];
	const coord3_t *crd_y = &coords[atY->xy_index];

	// calculate squared x-y distance, x-y distance and inverse x-y distance
	// immediately apply cut-off, if needed
	real dxy_x = crd_y->x - crd_x->x;
	real dxy_y = crd_y->y - crd_x->y;
	real dxy_z = crd_y->z - crd_x->z;
	real rxy_2 = dxy_x*dxy_x + dxy_y*dxy_y + dxy_z*dxy_z;
	if ( rxy_2 > rc_sq_ )
		return 0;

	real rxy = static_cast<real>( sqrt( rxy_2 ) );
	real irxy = real( 1.0 ) / rxy;

	// get y FF code
	const uint32 code_y = atY->y_code;

	// get charges
	const real q_x = atoms[atX->xy_index].charge;
	const real q_y = atoms[atY->xy_index].charge;

	// initialize total energies
	real total_e = irxy * q_x * q_y * ( irxy + crf_a_ * rxy_2 + crf_b_ );
	real total_h = 0;

	// now loop through hydrogens
	for ( size_t i = 0; atX->h_indices[i] != UINT32_BAD && i < MAX_H; ++i ) {
		// get h coords, charge and FF code
		const coord3_t *crd_h = &coords[atX->h_indices[i]];
		const real q_h = atoms[atX->h_indices[i]].charge;
		const uint32 code_h = atX->codes[i];

		// calculate squared x-h distance, x-h distance and inverse x-h distance
		real dxh_x = crd_x->x - crd_h->x;
		real dxh_y = crd_x->y - crd_h->y;
		real dxh_z = crd_x->z - crd_h->z;
		real rxh_2 = dxh_x*dxh_x + dxh_y*dxh_y + dxh_z*dxh_z;
		real rxh = static_cast<real>( sqrt( rxh_2 ) );
		real irxh = real( 1.0 ) / rxh;

		// calculate squared y-h distance, y-h distance and inverse y-h distance
		real dyh_x = crd_y->x - crd_h->x;
		real dyh_y = crd_y->y - crd_h->y;
		real dyh_z = crd_y->z - crd_h->z;
		real ryh_2 = dyh_x*dyh_x + dyh_y*dyh_y + dyh_z*dyh_z;
		real ryh = static_cast<real>( sqrt( ryh_2 ) );
		real iryh = real( 1.0 ) / ryh;

		// calculate electrostatics
		total_e += iryh * q_h * q_y * ( iryh + crf_a_ * ryh_2 + crf_b_ );

		// calculate 1+cos(theta)
		real cost = real( 1.0 ) + ( dyh_x*dxh_x + dyh_y*dxh_y + dyh_z*dxh_z ) * iryh * irxh;

		// calculate exponent
		real expt = static_cast<real>( exp( cost * cost / hb_minus_sigma2 ) );

		// get triplet parms
		const TripletParms *hbparms = &tripletParms_[code_h][code_y];

		// calculate 12-6 energy
		real hb126;
		if ( ryh <= hbparms->r ) hb126 = hbparms->e;
		else {
			real ir6 = iryh * iryh * iryh;
			ir6 *= ir6;
			hb126 = ir6 * ( hbparms->a * ir6 - hbparms->b );
		}

		// add total 12-6 energy
		total_h += expt * hb126;
	}

	total_e *= dd_e_ * gpGlobals->electrostatic_coeff;
	total_h *= gpGlobals->hbond_126_coeff;

	return total_e + total_h;
}

real CHBonds :: AverageEnergy( real e1, real e2 ) const
{
	// use harmonic mean because energy is generally inversely proportional
	// to distance which uses arithmetic mean
	return real( 2.0 ) / ( real( 1.0 ) / e1 + real( 1.0 ) / e2 );
}

real CHBonds :: AverageEnergy( real e1, real e2, real e3 ) const
{
	// use harmonic mean because energy is generally inversely proportional
	// to distance which uses arithmetic mean
	return real( 3.0 ) / ( real( 1.0 ) / e1 + real( 1.0 ) / e2 + real( 1.0 ) / e3 );
}

void CHBonds :: ProcessMicroset( ThreadLocal *tl, const HBBridge *firstBridge, size_t numBridges, const atom_t *atoms, const coord3_t *coords, HBPairMap &localPairMap, HBTripletMap &localTripletMap )
{
	assert( firstBridge != nullptr );

	if ( numBridges <= 1 ) {
		// a single atom, ignore
	} else if ( numBridges == 2 ) {
		// prepare solvent block
		assert( firstBridge[0].s_index == firstBridge[1].s_index );
		real energy = AverageEnergy( firstBridge[0].energy, firstBridge[1].energy );
		HBSolvent *glist;
		HBSolvent *block = GrabSolventBlock( tl );
		block->gblock = nullptr;
		block->energy = energy;
		block->snaps = 1;
		block->s_index = firstBridge[0].s_index;
		// register a pair
		HBPair value;
		value.index0 = firstBridge[0].b_index;
		value.index1 = firstBridge[1].b_index;
		auto existing = localPairMap.find( value );
		if ( existing == localPairMap.end() ) {
			HBLocalScore ls;
			ls.score = 1;
			ls.energy = energy;
			auto globalIt = hbPairScoreMap_.find( value );
			ls.global = ( globalIt == hbPairScoreMap_.end() ) ? nullptr : &globalIt->second;
			glist = ls.global ? ls.global->solv : nullptr;
			block->chain = nullptr;
			ls.solv = block;
			localPairMap.insert( std::make_pair( value, ls ) );
		} else {
			++existing->second.score;
			existing->second.energy += energy;
			glist = existing->second.global ? existing->second.global->solv : nullptr;
			block->chain = existing->second.solv;
			existing->second.solv = block;
		}
		if ( glist ) block->gblock = FindGlobalBlock( block, glist );
		if ( !block->gblock || energy < block->gblock->energy )
			BuildBlockInfo( block, atoms, coords );
	} else if ( numBridges == 3 ) {
		// prepare solvent block
		assert( firstBridge[0].s_index == firstBridge[1].s_index );
		assert( firstBridge[0].s_index == firstBridge[2].s_index );
		real energy = AverageEnergy( firstBridge[0].energy, firstBridge[1].energy, firstBridge[2].energy );
		HBSolvent *glist;
		HBSolvent *block = GrabSolventBlock( tl );
		block->gblock = nullptr;
		block->energy = energy;
		block->snaps = 1;
		block->s_index = firstBridge[0].s_index;
		// register a triplet
		HBTriplet value;
		value.index0 = firstBridge[0].b_index;
		value.index1 = firstBridge[1].b_index;
		value.index2 = firstBridge[2].b_index;
		auto existing = localTripletMap.find( value );
		if ( existing == localTripletMap.end() ) {
			HBLocalScore ls;
			ls.score = 1;
			ls.energy = energy;
			auto globalIt = hbTripletScoreMap_.find( value );
			ls.global = ( globalIt == hbTripletScoreMap_.end() ) ? nullptr : &globalIt->second;
			glist = ls.global ? ls.global->solv : nullptr;
			block->chain = nullptr;
			ls.solv = block;
			localTripletMap.insert( std::make_pair( value, ls ) );
		} else {
			++existing->second.score;
			existing->second.energy += energy;
			glist = existing->second.global ? existing->second.global->solv : nullptr;
			block->chain = existing->second.solv;
			existing->second.solv = block;
		}
		if ( glist ) block->gblock = FindGlobalBlock( block, glist );
		if ( !block->gblock || energy < block->gblock->energy )
			BuildBlockInfo( block, atoms, coords );
	} else {
		// register a bunch of triplets
		for ( std::size_t i = 0; i < numBridges - 2; ++i ) {
			for ( std::size_t j = i + 1; j < numBridges - 1; ++j ) {
				for ( std::size_t k = j + 1; k < numBridges; ++k ) {
					assert( firstBridge[i].s_index == firstBridge[j].s_index );
					assert( firstBridge[i].s_index == firstBridge[k].s_index );
					real energy = AverageEnergy( firstBridge[i].energy, firstBridge[j].energy, firstBridge[k].energy );
					HBSolvent *glist;
					HBSolvent *block = GrabSolventBlock( tl );
					block->gblock = nullptr;
					block->energy = energy;
					block->snaps = 1;
					block->s_index = firstBridge[i].s_index;
					HBTriplet value;
					value.index0 = firstBridge[i].b_index;
					value.index1 = firstBridge[j].b_index;
					value.index2 = firstBridge[k].b_index;
					auto existing = localTripletMap.find( value );
					if ( existing == localTripletMap.end() ) {
						HBLocalScore ls;
						ls.score = 1;
						ls.energy = energy;
						auto globalIt = hbTripletScoreMap_.find( value );
						ls.global = ( globalIt == hbTripletScoreMap_.end() ) ? nullptr : &globalIt->second;
						glist = ls.global ? ls.global->solv : nullptr;
						block->chain = nullptr;
						ls.solv = block;
						localTripletMap.insert( std::make_pair( value, ls ) );
					} else {
						++existing->second.score;
						existing->second.energy += energy;
						glist = existing->second.global ? existing->second.global->solv : nullptr;
						block->chain = existing->second.solv;
						existing->second.solv = block;
					}
					if ( glist ) block->gblock = FindGlobalBlock( block, glist );
					if ( !block->gblock || energy < block->gblock->energy )
						BuildBlockInfo( block, atoms, coords );
				}
			}
		}
	}
}

void CHBonds :: CalcMicrosets( const uint32 threadNum, const uint32 snapshotNum, const coord3_t *coords )
{
	assert( threadNum < numThreads_ );
	assert( topology->GetAtomCount() != 0 );
	ThreadLocal *tl = &tl_[threadNum];

	const atom_t *atoms = topology->GetAtomArray();
	const real cutoff = -gpGlobals->hbond_cutoff_energy;
	uint32 c_donors = 0, c_acceptors = 0, c_microsets = 0;

	double startTime = utils->FloatMilliseconds();
	double baseTime = startTime;

	//////////////////////////////////////////////////////////////////////////
	// FIND H-BOND CONNECTIONS AND BUILD MICROSETS
	//////////////////////////////////////////////////////////////////////////
	// Build a list of biopolymer-solvent h-bonding pairs ("bridges").
	// All biopolymer's donors/acceptors that connect to a single solvent 
	// donor/acceptor, are referred to as "microset". After this list is 
	// sorted by the solvent atom index, we have a consecutive list of 
	// microsets and we'll parse it later.
	//////////////////////////////////////////////////////////////////////////
	tl->bridges.resize( 0 );
	// for all biopolymer donor/acceptor atoms
	const auto itbEnd = hbBiopolyList_.cend();
	for ( auto itb = hbBiopolyList_.cbegin(); itb != itbEnd && !ThreadInterrupted(); ++itb ) {
		bool b_is_donor = ( itb->h_indices[0] != UINT32_BAD );
		bool b_is_accep = ( itb->y_code != UINT16_BAD );

		// for all solvent donor/acceptor atoms
		const auto itsEnd = hbSolventList_.cend();
		for ( auto its = hbSolventList_.cbegin(); its != itsEnd && !ThreadInterrupted(); ++its ) {
			bool s_is_donor = ( its->h_indices[0] != UINT32_BAD );
			bool s_is_accep = ( its->y_code != UINT16_BAD );
			bool valid_bond = false;
			real energy = 0;

			// calculate donor-acceptor energy
			if ( b_is_donor && s_is_accep ) {
				energy = CalcEnergy( &(*itb), &(*its), atoms, coords );
				if ( energy < cutoff ) {
					valid_bond = true;
					++c_donors;
#if 0
					const atom_t *atX = &atoms[itb->xy_index];
					const atom_t *atY = &atoms[its->xy_index];
					logfile->Print( "[%4i] D-A: %s-%i-%c%c%c%c to %s-%i-%c%c%c%c = %f\n", 
						c_donors,
						atX->residue.string, atX->resnum, atX->title.string[0], atX->title.string[1], atX->title.string[2], atX->title.string[3],
						atY->residue.string, atY->resnum, atY->title.string[0], atY->title.string[1], atY->title.string[2], atY->title.string[3],
						energy );
#endif
				}
			}

			// calculate acceptor-donor energy
			if ( !valid_bond && s_is_donor && b_is_accep ) {
				energy = CalcEnergy( &(*its), &(*itb), atoms, coords );
				if ( energy < cutoff ) {
					valid_bond = true;
					++c_acceptors;
#if 0
					const atom_t *atX = &atoms[its->xy_index];
					const atom_t *atY = &atoms[itb->xy_index];
					logfile->Print( "[%4i] A-D: %s-%i-%c%c%c%c to %s-%i-%c%c%c%c = %f\n", 
						c_acceptors,
						atX->residue.string, atX->resnum, atX->title.string[0], atX->title.string[1], atX->title.string[2], atX->title.string[3],
						atY->residue.string, atY->resnum, atY->title.string[0], atY->title.string[1], atY->title.string[2], atY->title.string[3],
						energy );
#endif
				}
			}

			// insert into the set of bridges
			if ( valid_bond ) {
				HBBridge b;
				b.s_index = its->xy_remap;
				b.b_index = itb->xy_remap;
				b.energy = energy;
				tl->bridges.push_back( b );
#if 0
				const atom_t *atX = &atoms[its->xy_index];
				const atom_t *atY = &atoms[itb->xy_index];
				logfile->Print( "BRIDGE: %s-%5i-%c%c%c%c to %s-%5i-%c%c%c%c = %f\n",
					atX->residue.string, atX->resnum, atX->title.string[0], atX->title.string[1], atX->title.string[2], atX->title.string[3],
					atY->residue.string, atY->resnum, atY->title.string[0], atY->title.string[1], atY->title.string[2], atY->title.string[3],
					energy );
#endif

			}
		}
	}
	// check for degenerate case
	if ( !tl->bridges.size() )
		return;

	// sort bridges
	std::sort( tl->bridges.begin(), tl->bridges.end() );

	// begin single-threaded block
	// everything else references global variables (e.g. hbPairScoreMap_) 
	// and can't be done multithreaded
	// but the rest is fast, the main time consumer was building microsets
	ThreadLock();

	double microsetTime = utils->FloatMilliseconds() - startTime;
	startTime += microsetTime;

	//////////////////////////////////////////////////////////////////////////
	// PROCESS MICROSETS
	//////////////////////////////////////////////////////////////////////////
	// Now microsets are ready: bridges are sorted by solvent atom index 
	// first, and biolpolymer atom index next. So, we have a consecutive list
	// of microsets. Process them, building a tuple of biopolymer donors and
	// acceptors that belong to a single microset, and then extracting pairs
	// and triplets from it. Also remember a reference solvent atom, to range
	// solvent molecules later.
	//////////////////////////////////////////////////////////////////////////
	HBPairMap localPairMap;
	HBTripletMap localTripletMap;
	uint32 last_s_index = 0;
	size_t numBridges = 0;
	const HBBridge *pBridge = &tl->bridges.at( 0 );
	const HBBridge *pFirstBridge = nullptr;
	const size_t totalBridges = tl->bridges.size();
	for ( size_t i = 0; i < totalBridges; ++i, ++pBridge ) {
		// get atom indices for (s)olvent and (b)iopolymer
		const uint32 s_index = pBridge->s_index;
		if ( s_index != last_s_index ) {
			// new microset started
			// process the previous microset
			if ( numBridges != 0 ) {
				ProcessMicroset( tl, pFirstBridge, numBridges, atoms, coords, localPairMap, localTripletMap );
				++c_microsets;
			}
			last_s_index = s_index;
			pFirstBridge = pBridge;
			numBridges = 1;
		} else {
			++numBridges;
		}
	}
	if ( numBridges != 0 ) {
		// process the last microset
		ProcessMicroset( tl, pFirstBridge, numBridges, atoms, coords, localPairMap, localTripletMap );
		++c_microsets;
	}

	logfile->Print( "----- CalcMicrosets (%u) thread %u -----\n", snapshotNum, threadNum ); 
	logfile->Print( "%6u donors\n"
					"%6u acceptors\n"
					"%6u microsets\n"
					"%6u pairs\n"
					"%6u triplets\n", 
					c_donors, c_acceptors, c_microsets, localPairMap.size(), localTripletMap.size() );


	// check if we haven't got any pairs or triplets
	if ( !localPairMap.size() && !localTripletMap.size() ) {
		// end single-threaded block
		ThreadUnlock();
		return;
	}

	//////////////////////////////////////////////////////////////////////////
	// ADD GLOBAL PAIR/TRIPLET INFO
	//////////////////////////////////////////////////////////////////////////
	// Merge local pairs and triplets into the global maps.
	// This requires thread synchronization. Also it's ok to dump some 
	// stuff to the logfile during the sync.
	//////////////////////////////////////////////////////////////////////////

	for ( auto itp = localPairMap.cbegin(); itp != localPairMap.cend(); ++itp ) {
#if 0
		const atom_t *at0 = &atoms[itp->first.index0];
		const atom_t *at1 = &atoms[itp->first.index1];
		const uint32 score = itp->second.score;
		logfile->Print( "PAIR: %s-%i-%c%c%c%c, %s-%i-%c%c%c%c (score = %u; energy = %f)\n",
			at0->residue.string, at0->resnum, at0->title.string[0], at0->title.string[1], at0->title.string[2], at0->title.string[3],
			at1->residue.string, at1->resnum, at1->title.string[0], at1->title.string[1], at1->title.string[2], at1->title.string[3],
			score, itp->second.energy );
#endif
		// check precached pointer to existing object
		if ( !itp->second.global ) {
			assert( itp->second.solv && ( 0 != ( itp->second.solv->flags & HBSF_VALID ) ) );
			HBGlobalScore scoreInfo;
			scoreInfo.score = itp->second.score;
			scoreInfo.energy = itp->second.energy;
			scoreInfo.solv = itp->second.solv;
			scoreInfo.snaps = 1;
			hbPairScoreMap_.insert( std::make_pair( itp->first, scoreInfo ) );
		} else {
			itp->second.global->score += itp->second.score;
			itp->second.global->energy += itp->second.energy;
			++itp->second.global->snaps;
			// merge solvent information
			for ( HBSolvent *lblock = itp->second.solv, *nextblock; lblock; lblock = nextblock ) {
				nextblock = lblock->chain;
				if ( !lblock->gblock ) {
					// no global block is associated
					assert( 0 != ( lblock->flags & HBSF_VALID ) );
					lblock->chain = itp->second.global->solv;
					itp->second.global->solv = lblock;
				} else {
					++lblock->gblock->snaps;
					if ( lblock->energy < lblock->gblock->energy ) {
						assert( 0 != ( lblock->flags & HBSF_VALID ) );
						lblock->gblock->energy = lblock->energy;
						memcpy( lblock->gblock->coords, lblock->coords, sizeof(*lblock->coords)*s_siz_ );
					}
					ReturnSolventBlock( tl, lblock );
				}
			}
		}
	}

	for ( auto itt = localTripletMap.cbegin(); itt != localTripletMap.cend(); ++itt ) {
#if 0
		const atom_t *at0 = &atoms[itt->first.index0];
		const atom_t *at1 = &atoms[itt->first.index1];
		const atom_t *at2 = &atoms[itt->first.index2];
		const uint32 score = itt->second.score;
		logfile->Print( "TRIPLET: %s-%i-%c%c%c%c, %s-%i-%c%c%c%c, %s-%i-%c%c%c%c (score = %u)\n",
			at0->residue.string, at0->resnum, at0->title.string[0], at0->title.string[1], at0->title.string[2], at0->title.string[3],
			at1->residue.string, at1->resnum, at1->title.string[0], at1->title.string[1], at1->title.string[2], at1->title.string[3],
			at2->residue.string, at2->resnum, at2->title.string[0], at2->title.string[1], at2->title.string[2], at2->title.string[3],
			score );
#endif
		if ( !itt->second.global ) {
			assert( itt->second.solv && ( 0 != ( itt->second.solv->flags & HBSF_VALID ) ) );
			HBGlobalScore scoreInfo;
			scoreInfo.score = itt->second.score;
			scoreInfo.energy = itt->second.energy;
			scoreInfo.solv = itt->second.solv;
			scoreInfo.snaps = 1;
			hbTripletScoreMap_.insert( std::make_pair( itt->first, scoreInfo ) );
		} else {
			itt->second.global->score += itt->second.score;
			itt->second.global->energy += itt->second.energy;
			++itt->second.global->snaps;
			// merge solvent information
			for ( HBSolvent *lblock = itt->second.solv, *nextblock; lblock; lblock = nextblock ) {
				nextblock = lblock->chain;
				if ( !lblock->gblock ) {
					// no global block is associated
					assert( 0 != ( lblock->flags & HBSF_VALID ) );
					lblock->chain = itt->second.global->solv;
					itt->second.global->solv = lblock;
				} else {
					++lblock->gblock->snaps;
					if ( lblock->energy < lblock->gblock->energy ) {
						assert( 0 != ( lblock->flags & HBSF_VALID ) );
						lblock->gblock->energy = lblock->energy;
						memcpy( lblock->gblock->coords, lblock->coords, sizeof(*lblock->coords)*s_siz_ );
					}
					ReturnSolventBlock( tl, lblock );
				}
			}
		}
	}

	double tupleTime = utils->FloatMilliseconds() - startTime;
	startTime += tupleTime;

	// update performance counters
	UpdatePerformanceCounter( &perfCounters_.pcMicroset, microsetTime );
	UpdatePerformanceCounter( &perfCounters_.pcTuples, tupleTime );
	UpdatePerformanceCounter( &perfCounters_.pcTotal, startTime - baseTime );

	// end single-threaded block
	ThreadUnlock();
}

void CHBonds :: UpdatePerformanceCounter( HBPerfCounter *pc, real value ) const
{
	if ( !pc->frames ) {
		pc->frames = 1;
		pc->minTime = value;
		pc->maxTime = value;
		pc->avgTime = value;
	} else {
		++pc->frames;
		if ( value < pc->minTime ) pc->minTime = value;
		if ( value > pc->maxTime ) pc->maxTime = value;
		pc->avgTime += value;
	}
}

void CHBonds :: FinalizePerformanceCounters()
{
	// calculate average values
	if ( perfCounters_.pcMicroset.frames )
		perfCounters_.pcMicroset.avgTime /= perfCounters_.pcMicroset.frames;
	if ( perfCounters_.pcTuples.frames )
		perfCounters_.pcTuples.avgTime /= perfCounters_.pcTuples.frames;
	if ( perfCounters_.pcTotal.frames )
		perfCounters_.pcTotal.avgTime /= perfCounters_.pcTotal.frames;
}

void CHBonds :: PrintPerformanceCounters()
{
	// print results
	logfile->Print( "\n------------ PERFORMANCE TIMING (ms) ------------\n"
					"%20s  %8s %8s %8s\n", "", "avg", "min", "max" );
	logfile->Print( "%20s: %8.1f %8.1f %8.1f\n", "Building microsets", 
		perfCounters_.pcMicroset.avgTime,
		perfCounters_.pcMicroset.minTime,
		perfCounters_.pcMicroset.maxTime );
	logfile->Print( "%20s: %8.1f %8.1f %8.1f\n", "Find pairs/triplets", 
		perfCounters_.pcTuples.avgTime,
		perfCounters_.pcTuples.minTime,
		perfCounters_.pcTuples.maxTime );
	logfile->Print( "%20s: %8.1f %8.1f %8.1f\n", "Total microset proc", 
		perfCounters_.pcTotal.avgTime,
		perfCounters_.pcTotal.minTime,
		perfCounters_.pcTotal.maxTime );
	logfile->Print( "-------------------------------------------------\n" );
}

void CHBonds :: GetOutputAtomTitle( const atom_t *at, const uint32 index, char *outBuffer, size_t outBufferSize ) const
{
	assert( outBufferSize > 4 );
	auto it = hbGroupTitleMap_.find( index );
	if ( it != hbGroupTitleMap_.cend() ) {
		strcpy_s( outBuffer, outBufferSize, it->second.c_str() );
	} else {
		size_t k = 0;
		for ( ; k < 4 && at->xtitle.string[k] != ' '; ++k )
			outBuffer[k] = at->xtitle.string[k];
		outBuffer[k] = 0;
	}
}

void CHBonds :: AddFinalSolventBlocks( HBSolventMap &finalSolventMap, atom_t *atoms, const HBSolvent *blocklist ) const
{
	const size_t blockSize = sizeof(HBFinalSolvent) + ( s_siz_ - 1 ) * sizeof(coord4_t);

	// find solvent block with the best energy
	const HBSolvent *bestblock = nullptr;
	real bestenergy = 0;
	for ( const HBSolvent *block = blocklist; block; block = block->chain ) {
		const real energy = block->energy * block->snaps;
		if ( !bestblock || energy < bestenergy ) {
			bestblock = block;
			bestenergy = energy;
		}
	}

	assert( bestblock != nullptr );
	if ( !bestblock )
		return;

	// find this solvent block in final map
	const uint32 resnum = atoms[bestblock->s_index].resnum;
	auto existing = finalSolventMap.find( resnum );

	if ( existing != finalSolventMap.end() ) {
		// solvent already exists
		// check if we should replace it because of better energy
		HBFinalSolvent *solv = existing->second;
		if ( bestenergy < solv->energy ) {
			memcpy( solv->coords, bestblock->coords, sizeof(coord4_t) * s_siz_ );
			solv->snaps = bestblock->snaps;
			solv->energy = bestenergy;
		}
	} else {
		// solvent doesn't exist
		// add new solvent record
		HBFinalSolvent *solv = reinterpret_cast<HBFinalSolvent*>( utils->Alloc( blockSize ) );
		memcpy( solv->coords, bestblock->coords, sizeof(coord4_t) * s_siz_ );
		solv->snaps = bestblock->snaps;
		solv->atindex = bestblock->s_index;
		solv->energy = bestenergy;
		finalSolventMap.insert( std::make_pair( resnum, solv ) );
	}
}

bool CHBonds :: TestFinalSolventBlocks( const HBFinalSolvent *s1, const HBFinalSolvent *s2 ) const
{
	// return true if solvents do not overlap
	// return false if they really do
	for ( size_t i = 0; i < s_siz_; ++i ) {
		for ( size_t j = 0; j < s_siz_; ++j ) {
			real dx = s1->coords[i].x - s2->coords[j].x;
			real dy = s1->coords[i].y - s2->coords[j].y;
			real dz = s1->coords[i].z - s2->coords[j].z;
			real rr = s1->coords[i].r + s2->coords[j].r;
			real dd = dx*dx + dy*dy + dz*dz;
			if ( dd < rr * rr )
				return false;
		}
	}
	return true;
}

void CHBonds :: BuildFinalSolvent( uint32 totalFrames )
{
	const uint32 snap_cutoff = static_cast<uint32>( ceil( totalFrames * gpGlobals->occurence_cutoff ) );
	const size_t atomCount = topology->GetAtomCount();
	atom_t *atoms = topology->GetAtomArray();
	coord3_t *coords = topology->GetBaseCoords();
	HBSolventMap finalSolventMap;

	logfile->Print( "\nBuildFinalSolvent:\n" );
	
	for ( auto it = hbPairScoreMap_.cbegin(); it != hbPairScoreMap_.cend(); ++it ) {
		assert( it->second.score > 0 );
		uint32 numSnaps = it->second.snaps;
		if ( numSnaps < snap_cutoff )
			continue;
		// this is a valid tuple
		AddFinalSolventBlocks( finalSolventMap, atoms, it->second.solv );
	}
	for ( auto it = hbTripletScoreMap_.cbegin(); it != hbTripletScoreMap_.cend(); ++it ) {
		assert( it->second.score > 0 );
		uint32 numSnaps = it->second.snaps;
		if ( numSnaps < snap_cutoff )
			continue;
		// this is a valid tuple
		AddFinalSolventBlocks( finalSolventMap, atoms, it->second.solv );
	}

	if ( gpGlobals->vdw_tolerance < 1 ) {
		// filter solvent molecules by geometric proximity means
		// only one solvent with the best energy should be retained
		for ( auto it_source = finalSolventMap.begin(); it_source != finalSolventMap.end(); ++it_source ) {
			HBFinalSolvent *s_source = it_source->second;
			if ( s_source->snaps == 0 )
				continue;
			for ( auto it_check = finalSolventMap.begin(); it_check != finalSolventMap.end(); ++it_check ) {
				HBFinalSolvent *s_check = it_check->second;
				if ( s_check == s_source || s_check->snaps == 0 )
					continue;
				if ( TestFinalSolventBlocks( s_source, s_check ) )
					continue;
				if ( s_source->energy < s_check->energy ) {
					s_check->snaps = 0;
					continue;
				}
				s_source->snaps = 0;
				break;
			}
		}
	}

	// make sure all solvent atoms will be skipped
	// later we'll "unskip" some of them
	for ( size_t i = 0; i < atomCount; ++i )
		if ( atoms[i].flags & AF_SOLVENT ) atoms[i].flags |= AF_SKIP;

	uint32 totalMolecules = 0, totalCulled = 0;
	for ( auto it = finalSolventMap.cbegin(); it != finalSolventMap.cend(); ++it ) {
		// average coordinates and remove the skip flag
		HBFinalSolvent *solv = it->second;
		if ( solv->snaps == 0 ) {
			++totalCulled;
			continue;
		}
		++totalMolecules;
		const atom_t *at = &atoms[solv->atindex];
		logfile->Print( "%-3u %s-%u: snaps = %u energy = %f\n", totalMolecules, at->residue.string, it->first, it->second->snaps, it->second->energy / it->second->snaps );
		for ( uint32 i = 0, idx = at->rfirst; i < at->rcount; ++i, ++idx ) {
			atom_t *ats = &atoms[idx];
			ats->flags &= ~AF_SKIP;
			coords[idx].x = solv->coords[i].x;
			coords[idx].y = solv->coords[i].y;
			coords[idx].z = solv->coords[i].z;
			logfile->Print( "        %c%c%c%c @ ( %6.1f, %6.1f, %6.1f )\n", 
				ats->xtitle.string[0], ats->xtitle.string[1], ats->xtitle.string[2], ats->xtitle.string[3], 
				solv->coords[i].x, solv->coords[i].y, solv->coords[i].z );
		}
	}

	logfile->Print( "%u culled solvent residues\n", totalCulled );
	logfile->Print( "%u final solvent residues\n\n", totalMolecules );

	for ( auto it = finalSolventMap.begin(); it != finalSolventMap.end(); ++it )
		utils->Free( it->second );

	FinalizePerformanceCounters();
}

bool CHBonds :: PrintFinalTuples( uint32 totalFrames, const char *tupleFile ) const
{
	assert( totalFrames > 0 );
	assert( tupleFile[0] != 0 );

	FILE *fp;
	if ( fopen_s( &fp, tupleFile, "w" ) ) {
		utils->Warning( "failed to open \"%s\" for writing\n", tupleFile );
		return false;
	}

	const atom_t *atoms = topology->GetAtomArray();
	char localTitle[3][256], localName[64];

	const uint32 max_hist_bins = 20;
	uint32 num_hist_bins = std::min( max_hist_bins, totalFrames );
	uint32 hist_bin_data[max_hist_bins];
	memset( hist_bin_data, 0, sizeof(hist_bin_data) );

	const double binScale = static_cast<double>( num_hist_bins ) / totalFrames;
	const uint32 total_pairs = static_cast<int>( hbPairScoreMap_.size() + hbTripletScoreMap_.size() * 3 );
	const uint32 snap_cutoff = static_cast<uint32>( ceil( totalFrames * gpGlobals->occurence_cutoff ) );
	const real invTotalFrames = real( 100 ) / totalFrames;

	HBFinalPairVec finalPairs;
	finalPairs.reserve( hbPairScoreMap_.size() );
	for ( auto it = hbPairScoreMap_.cbegin(); it != hbPairScoreMap_.cend(); ++it ) {
		assert( it->second.score > 0 );
		uint32 numSnaps = it->second.snaps;
		uint32 binIndex = static_cast<uint32>( ceil( binScale * numSnaps ) ) - 1;
		assert( binIndex < num_hist_bins );
		++hist_bin_data[binIndex];
		if ( numSnaps < snap_cutoff )
			continue;
		HBFinalPair p;
		p.index0 = it->first.index0;
		p.index1 = it->first.index1;
		p.score = it->second.score;
		p.occurence = it->second.snaps * invTotalFrames;
		p.energy = it->second.energy / it->second.score;
		finalPairs.push_back( p );
#if 0
		char localTitle[3][256], localName[64];
		const atom_t *at0 = &atoms[p.index0];
		const atom_t *at1 = &atoms[p.index1];
		GetOutputAtomTitle( at0, p.index0, localName, sizeof(localName) );
		sprintf_s( localTitle[0], sizeof(localTitle[0]), "%s-%u %s", at0->residue.string, at0->resnum, localName );
		GetOutputAtomTitle( at1, p.index1, localName, sizeof(localName) );
		sprintf_s( localTitle[1], sizeof(localTitle[1]), "%s-%u %s", at1->residue.string, at1->resnum, localName );

		logfile->Print( "PAIR: %6u %-20s %6u %-20s   snaps: %6u\n", at0->serial, localTitle[0], at1->serial, localTitle[1], it->second.snaps );
		for ( HBSolvent *block = it->second.solv; block; block = block->chain ) {
			logfile->Print( "SOLVENT: %6u %6u %16.8f   snaps: %6u @", 
				block->s_index, atoms[block->s_index].resnum, block->energy, block->snaps );
			for ( size_t i = 0; i < s_siz_; ++i )
				logfile->Print( " (%6.1f, %6.1f, %6.1f)", block->coords[i].x, block->coords[i].y, block->coords[i].z );
			logfile->Print( "\n" );
		}
#endif
	}

	HBFinalTripletVec finalTriplets;
	finalTriplets.reserve( hbTripletScoreMap_.size() );
	for ( auto it = hbTripletScoreMap_.cbegin(); it != hbTripletScoreMap_.cend(); ++it ) {
		assert( it->second.score > 0 );
		uint32 numSnaps = it->second.snaps;
		uint32 binIndex = static_cast<uint32>( ceil( binScale * numSnaps ) ) - 1;
		assert( binIndex < num_hist_bins );
		hist_bin_data[binIndex] += 3;
		if ( numSnaps < snap_cutoff )
			continue;
		HBFinalTriplet p;
		p.index0 = it->first.index0;
		p.index1 = it->first.index1;
		p.index2 = it->first.index2;
		p.score = it->second.score;
		p.occurence = it->second.snaps * invTotalFrames;
		p.energy = it->second.energy / it->second.score;
		finalTriplets.push_back( p );
#if 0
		const atom_t *at0 = &atoms[p.index0];
		const atom_t *at1 = &atoms[p.index1];
		const atom_t *at2 = &atoms[p.index2];
		GetOutputAtomTitle( at0, p.index0, localName, sizeof(localName) );
		sprintf_s( localTitle[0], sizeof(localTitle[0]), "%s-%u %s", at0->residue.string, at0->resnum, localName );
		GetOutputAtomTitle( at1, p.index1, localName, sizeof(localName) );
		sprintf_s( localTitle[1], sizeof(localTitle[1]), "%s-%u %s", at1->residue.string, at1->resnum, localName );
		GetOutputAtomTitle( at2, p.index2, localName, sizeof(localName) );
		sprintf_s( localTitle[2], sizeof(localTitle[2]), "%s-%u %s", at2->residue.string, at2->resnum, localName );

		logfile->Print( "TRIPLET: %6u %-20s %6u %-20s %6u %-20s   snaps: %6u\n", 
			at0->serial, localTitle[0], 
			at1->serial, localTitle[1], 
			at2->serial, localTitle[2], 
			it->second.snaps );
		for ( HBSolvent *block = it->second.solv; block; block = block->chain ) {
			logfile->Print( "SOLVENT: %6u %6u %16.8f   snaps: %6u @", 
			block->s_index, atoms[block->s_index].resnum, block->energy, block->snaps );
			for ( size_t i = 0; i < s_siz_; ++i )
				logfile->Print( " (%6.1f, %6.1f, %6.1f)", block->coords[i].x, block->coords[i].y, block->coords[i].z );
			logfile->Print( "\n" );
		}
#endif
	}

	// sort if necessary
	if ( finalPairs.size() > 1 )	std::sort( finalPairs.begin(), finalPairs.end() );
	if ( finalTriplets.size() > 1 )	std::sort( finalTriplets.begin(), finalTriplets.end() );

	// write output
	console->Print( "Writing: \"%s\"...\n", tupleFile );

	fprintf_s( fp, "%s PARAMETERS:\n", PROGRAM_LARGE_NAME );
	fprintf_s( fp, "%20s:\t%.3f\n", "X-Y distance cutoff", gpGlobals->hbond_max_length );
	fprintf_s( fp, "%20s:\t%.3f\n", "Abs energy cutoff", gpGlobals->hbond_cutoff_energy );
	fprintf_s( fp, "%20s:\t%.3f\n", "Occurence cutoff", gpGlobals->occurence_cutoff );
	fprintf_s( fp, "%20s:\t%.3f\n", "Electrostatic coeff", gpGlobals->electrostatic_coeff );
	fprintf_s( fp, "%20s:\t%.3f\n", "HB-12-6 coeff", gpGlobals->hbond_126_coeff );

	if ( total_pairs > 0 ) {
		fprintf_s( fp, "%s\n", "----------------------------------------------------------------------------------------------------" );
		fprintf_s( fp, "%s\n", "                             PAIR OCCURENCE ACCUMULATIVE HISTOGRAM                                  " );
		fprintf_s( fp, "%s\n", "----------------------------------------------------------------------------------------------------" );
		const double inv_num_hist_bins = 100.0 / num_hist_bins;
		uint32 accum = 0;
		for ( uint32 i = 0; i < num_hist_bins; ++i ) {
			accum += hist_bin_data[i];
			double accum_f = static_cast<double>( accum ) / total_pairs;
			uint32 accum_i = static_cast<uint32>( floor( accum_f * 50 + 0.5 ) );
			fprintf_s( fp, "%3.0f%%\t%5.2f\t%6u\t|", ceil( ( i + 1 ) * inv_num_hist_bins ), accum_f, accum );
			for ( uint32 j = 0; j < accum_i; ++j )
				fputc( '=', fp );
			fputc( '\n', fp );
		}
		fprintf_s( fp, "%s\n\n", "----------------------------------------------------------------------------------------------------" );
	}

	if ( finalPairs.size() > 0 ) {
		fprintf_s( fp, "%s\n", "----------------------------------------------------------------------------------------------------" );
		fprintf_s( fp, "                                         %8u PAIRS                                            \n", (uint32)finalPairs.size() );
		fprintf_s( fp, "%s\n", "----------------------------------------------------------------------------------------------------" );
		fprintf_s( fp, "%5s\t%-20s\t%5s\t%-20s\t%6s\t%6s\t%20s\n",
						"s/n 1", "atom 1",
						"s/n 2", "atom 2",
						"score", "occur", "energy, kcal/mol" );
		fprintf_s( fp, "%s\n", "----------------------------------------------------------------------------------------------------" );

		for ( auto it = finalPairs.cbegin(); it != finalPairs.cend(); ++it ) {
			const atom_t *at0 = &atoms[it->index0];
			const atom_t *at1 = &atoms[it->index1];
			GetOutputAtomTitle( at0, it->index0, localName, sizeof(localName) );
			sprintf_s( localTitle[0], sizeof(localTitle[0]), "%s-%u %s", at0->residue.string, at0->resnum, localName );
			GetOutputAtomTitle( at1, it->index1, localName, sizeof(localName) );
			sprintf_s( localTitle[1], sizeof(localTitle[1]), "%s-%u %s", at1->residue.string, at1->resnum, localName );
			fprintf_s( fp, "%5u\t%-20s\t%5u\t%-20s\t%6u\t%5.1f%%\t%20.6f\n",
				at0->serial, localTitle[0],
				at1->serial, localTitle[1],
				it->score, it->occurence, it->energy );
		}
		fprintf_s( fp, "%s\n\n", "----------------------------------------------------------------------------------------------------" );
	}
	if ( finalTriplets.size() > 0 ) {
		fprintf_s( fp, "%s\n", "------------------------------------------------------------------------------------------------------------------------------------" );
		fprintf_s( fp, "                                                 %8u TRIPLETS                                                                 \n", (uint32)finalTriplets.size() );
		fprintf_s( fp, "%s\n", "------------------------------------------------------------------------------------------------------------------------------------" );
		fprintf_s( fp, "%5s\t%-20s\t%5s\t%-20s\t%5s\t%-20s\t%6s\t%6s\t%20s\n",
						"s/n 1", "atom 1",
						"s/n 2", "atom 2",
						"s/n 3", "atom 3",
						"score", "occur", "energy, kcal/mol" );
		fprintf_s( fp, "%s\n", "------------------------------------------------------------------------------------------------------------------------------------" );

		for ( auto it = finalTriplets.cbegin(); it != finalTriplets.cend(); ++it ) {
			const atom_t *at0 = &atoms[it->index0];
			const atom_t *at1 = &atoms[it->index1];
			const atom_t *at2 = &atoms[it->index2];
			GetOutputAtomTitle( at0, it->index0, localName, sizeof(localName) );
			sprintf_s( localTitle[0], sizeof(localTitle[0]), "%s-%u %s", at0->residue.string, at0->resnum, localName );
			GetOutputAtomTitle( at1, it->index1, localName, sizeof(localName) );
			sprintf_s( localTitle[1], sizeof(localTitle[1]), "%s-%u %s", at1->residue.string, at1->resnum, localName );
			GetOutputAtomTitle( at2, it->index2, localName, sizeof(localName) );
			sprintf_s( localTitle[2], sizeof(localTitle[2]), "%s-%u %s", at2->residue.string, at2->resnum, localName );
			fprintf_s( fp, "%5u\t%-20s\t%5u\t%-20s\t%5u\t%-20s\t%6u\t%5.1f%%\t%20.6f\n",
				at0->serial, localTitle[0],
				at1->serial, localTitle[1],
				at2->serial, localTitle[2],
				it->score, it->occurence, it->energy );
		}

		fprintf_s( fp, "%s\n\n", "------------------------------------------------------------------------------------------------------------------------------------" );
	}

	if ( total_pairs == 0 )
		fprintf_s( fp, "%s\n", "No pairs or triplets found!" );

	fclose( fp );
	return true;
}
