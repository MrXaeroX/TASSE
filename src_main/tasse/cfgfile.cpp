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

class CCfgFile : public ICfgFile
{
	typedef enum {
		CONFIG_PARSE_NORMAL = 0,
		CONFIG_PARSE_ERROR,
		CONFIG_PARSE_EOF
	} eConfigParse;

public:
	typedef struct {
		const char	*signature;
		size_t		siglen;
		eConfigType type;
	} ConfigSignatureMap_t;

	explicit CCfgFile( const char *filename );
	virtual ~CCfgFile();

	bool IsOfType( eConfigType type ) const { return ( type_ == type ); }
	bool IsValid() const { return ( filebase_ != nullptr ); };
	bool IsEOF() const { return ( filepos_ == nullptr ); }

	real GetVersion() const { return version_; }
	virtual const char *GetToken() const { return token_; }
	virtual bool ParseToken( bool crossline );
	virtual void UnparseToken();
	virtual bool MatchToken( const char *match );
	virtual bool TokenAvailable();
	virtual void SkipRestOfLine();
	virtual void Rewind();

private:
	CCfgFile( const CCfgFile &other );
	CCfgFile& operator = ( const CCfgFile& other );

	void Precache( const char *filename );
	bool IsSingleChar( uint8 c ) const;

private:
	static const size_t c_ConfigMaxToken = 512;

private:
	eConfigType		type_;
	real			version_;
	eConfigParse	parseState_;
	bool			tokenReady_;
	uint8			*filebase_;
	uint8			*filepos_;
	uint8			*fileend_;
	char			token_[c_ConfigMaxToken];
};

class CCfgList : public ICfgList
{
	typedef std::list<CCfgFile*> ConfigList;
public:
	virtual ~CCfgList() { Unload(); }
	virtual void Load();
	virtual void Unload();
	virtual void ForEach( eConfigType type, ConfigCallback_t func );
private:
	ConfigList list_;
};

static CCfgList configsLocal;
ICfgList *configs = &configsLocal;

//////////////////////////////////////////////////////////////////////////

static CCfgFile::ConfigSignatureMap_t s_csm[] = {
	{ "HBparms",	0, CONFIG_TYPE_HBPARMS	},
	{ "HBres",		0, CONFIG_TYPE_HBRES	},
	{ "Qres",		0, CONFIG_TYPE_QRES		},
	{ "Solvres",	0, CONFIG_TYPE_SOLVRES	},
	{ "VdWres",		0, CONFIG_TYPE_VDWRES	}
};

CCfgFile :: CCfgFile( const char *filename ) : type_( CONFIG_TYPE_UNKNOWN ), version_( 0.0f ), parseState_( CONFIG_PARSE_NORMAL ),
											   tokenReady_( false ), filebase_( nullptr ), filepos_( nullptr ), fileend_( nullptr )
{
	Precache( filename );
}

CCfgFile :: ~CCfgFile()
{
	utils->Free( filebase_ );
}

void CCfgFile :: Precache( const char *filename )
{
	assert( filename != nullptr );

	char shortname[MAX_OSPATH];
	utils->ExtractFileName( shortname, sizeof(shortname), filename );

	console->NPrint( "Loading: \"%s\"... ", shortname );
	logfile->Print( "Loading: \"%s\"... ", filename );

	FILE *fp;
	if ( fopen_s( &fp, filename, "rb" ) ) {
		console->Print( "failed\n" );
		return;
	}

	fseek( fp, 0, SEEK_END );
	long end = ftell( fp );
	rewind( fp );

	filebase_ = reinterpret_cast<uint8*>( utils->Alloc( end + 1 ) );
	memset( filebase_, 0, end + 1 );

	long rv = static_cast<long>( fread( filebase_, 1, end, fp ) );
	if ( end != rv ) {
		utils->Free( filebase_ );
		filebase_ = nullptr;
		fclose( fp );
		console->Print( "failed\n" );
		return;
	}

	fclose( fp );

	filepos_ = filebase_;
	fileend_ = filebase_ + end;
	type_ = CONFIG_TYPE_UNKNOWN;
	version_ = 0.0f;

	if ( this->ParseToken( true ) && parseState_ == CONFIG_PARSE_NORMAL ) {
		if ( token_[0] == '!' && token_[1] == '!' ) {
			const size_t sigcount = sizeof(s_csm) / sizeof(s_csm[0]);
			for ( size_t i = 0; i < sigcount; ++i ) {
				if ( !s_csm[i].siglen )
					s_csm[i].siglen = strlen( s_csm[i].signature );
				if ( !_strnicmp( s_csm[i].signature, token_ + 2, s_csm[i].siglen ) ) {
					type_ = s_csm[i].type;
					version_ = utils->Atof( token_ + 2 + s_csm[i].siglen );
					break;
				}
			}
		}
	}

	if ( type_ == CONFIG_TYPE_UNKNOWN )
		this->UnparseToken();

	console->Print( "ok\n" );
}

bool CCfgFile :: TokenAvailable()
{
	// check parse state
	if ( parseState_ != CONFIG_PARSE_NORMAL )
		return false;

	const uint8 *search_p = filepos_;

	// check parse position
	if ( search_p >= fileend_ )
		return false;

	// check for newline
	while ( *search_p <= 0x20 ) {
		if ( *search_p == '\n' )
			return false;
		if ( ++search_p == filepos_ )
			return false;
	}

	// check for comment
	if ( ( *search_p == '/' ) && ( *(search_p + 1) == '/' ) )
		return false;

	return true;
}

void CCfgFile :: SkipRestOfLine()
{
	// skip to the newline
	while ( this->TokenAvailable() )
		this->ParseToken( false );
}

void CCfgFile :: UnparseToken()
{
	assert( tokenReady_ == false );
	tokenReady_ = true;
}

bool CCfgFile :: IsSingleChar( uint8 c ) const
{
	return ( c == '{' || c == '}' || c == ',' || c == '@' ||
			 c == '(' || c == ')' || c == '=' || c == ';' || 
			 c == '[' || c == ']' || c == ':' );
}

bool CCfgFile :: ParseToken( bool crossline )
{
	if ( parseState_ != CONFIG_PARSE_NORMAL ) {
		token_[0] = 0;
		return false;
	}

	// is a token already waiting?
	if ( tokenReady_ ) {
		tokenReady_ = false;
		return true;
	}

	token_[0] = 0;

	if ( filepos_ >= fileend_ ) {
		parseState_ = CONFIG_PARSE_EOF;
		return false;
	}

	// skip space
skipspace:
	while ( 0[filepos_] <= 0x20 ) {
		if ( filepos_ >= fileend_ ) {
			parseState_ = CONFIG_PARSE_EOF;
			return false;
		}
		if ( *filepos_++ == '\n' ) {
			if ( !crossline ) {
				--filepos_;
				return false;
			}
		}
	}

	if ( filepos_ >= fileend_ ) {
		parseState_ = CONFIG_PARSE_EOF;
		return false;
	}

	// '//' comments
	if ( ( 0[filepos_] == '/' ) && ( 1[filepos_] == '/' ) ) {
		if ( !crossline )
			return false;
		while ( *filepos_++ != '\n' ) {
			if ( filepos_ >= fileend_ ) {
				parseState_ = CONFIG_PARSE_EOF;
				return false;
			}
		}
		goto skipspace;
	}

	// '/* */' comments
	if ( ( 0[filepos_] == '/' ) && ( 1[filepos_] == '*' ) ) {
		if ( !crossline )
			return false;
		filepos_ += 2;
		while ( ( 0[filepos_] != '*' ) || ( 1[filepos_] != '/' ) ) {
			filepos_++;
			if ( filepos_ >= fileend_ ) {
				parseState_ = CONFIG_PARSE_EOF;
				return false;
			}
		}
		filepos_ += 2;
		goto skipspace;
	}

	// copy token
	char *token_p = token_;

	if ( 0[filepos_] == '"' ) {
		// quoted token
		++filepos_;
		while ( 0[filepos_] != '"' ) {
			*token_p++ = *filepos_++;
			if ( filepos_ == fileend_ )
				break;
			if ( token_p == &token_[c_ConfigMaxToken] ) {
				parseState_ = CONFIG_PARSE_ERROR;
				return false;
			}
		}
		++filepos_;
	} else {
		// single character
		uint8 c = *filepos_;
		if ( IsSingleChar( c ) ) {
			*token_p++ = c;
			*token_p = 0;
			++filepos_;
			return true;
		}

		// regular token
		while ( 0[filepos_] > 0x20 ) {
			*token_p++ = *filepos_++;
			if ( filepos_ == fileend_ )
				break;
			uint8 c = *filepos_;
			if ( IsSingleChar( c ) )
				break;
			if ( token_p == &token_[c_ConfigMaxToken] ) {
				parseState_ = CONFIG_PARSE_ERROR;
				return false;
			}
		}
	}

	*token_p = 0;
	return true;
}

bool CCfgFile :: MatchToken( const char *match )
{
	this->ParseToken( true );
	return ( strcmp( token_, match ) == 0 );
}

void CCfgFile :: Rewind()
{
	filepos_ = filebase_;
	tokenReady_ = false;
	parseState_ = CONFIG_PARSE_NORMAL;
}

//////////////////////////////////////////////////////////////////////////

void CCfgList :: Load()
{
	Unload();

	console->Print( "Scanning for config files in directory \"%s\"...\n", PROGRAM_CONFIG_PATH );

	char configpath[MAX_OSPATH];
	char fullname[MAX_OSPATH];
	int numconfigfiles = 0;

	utils->GetMainDirectory( configpath, sizeof(configpath) );
	strncat_s( configpath, "/" PROGRAM_CONFIG_PATH, sizeof(configpath)-1 );

	char **configfiles = utils->GetFileList( configpath, ".cfg", &numconfigfiles );

	for ( int i = 0; i < numconfigfiles; ++i ) {
		memset( fullname, 0, sizeof(fullname) );
		strncat_s( fullname, configpath, sizeof(fullname)-1 );
		strncat_s( fullname, "/", sizeof(fullname)-1 );
		strncat_s( fullname, configfiles[i], sizeof(fullname)-1 );
		CCfgFile *cfg = new CCfgFile( fullname );
		if ( cfg->IsValid() ) 
			list_.push_front( cfg );
		else
			delete cfg;
	}
	utils->FreeFileList( configfiles );
}

void CCfgList :: Unload()
{
	const auto itEnd = list_.end();
	for ( auto it = list_.begin(); it != itEnd; ++it )
		delete *it;
	list_.clear();
}

void CCfgList :: ForEach( eConfigType type, ConfigCallback_t func )
{
	assert( func != nullptr );
	const auto itEnd = list_.end();
	for ( auto it = list_.begin(); it != itEnd; ++it ) {
		if ( type == CONFIG_TYPE_DONTCARE || (*it)->IsOfType( type ) )
			func( *it );
	}
}
