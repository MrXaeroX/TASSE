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

class CLogFile : public ILogFile
{
public:
	CLogFile() : fp_( nullptr ), nolog_( false ) {}
	virtual ~CLogFile() { Close(); }
	virtual void Open( const char *filename );
	virtual void Print( const char *fmt, ... );
	virtual void Warning( const char *fmt, ... );
	virtual void Write( const char *msg );
	virtual void Close();
	virtual void LogTimeElapsed( double elapsed_time );
private:
	CLogFile( const CLogFile &other );
	CLogFile& operator = ( const CLogFile& other );
	void FormatTimeString( char *buf, size_t bufSize, const time_t *t ) const;
	void SecondsToDHMS( uint32 elapsed_time, uint32 &days, uint32 &hours, uint32 &minutes, uint32 &seconds ) const;
private:
	static const size_t c_MaxLogFileOutputString;
	static const char c_DefaultLogFileName[];
private:
	FILE	*fp_;
	bool	nolog_;
};

static CLogFile logfileLocal;
ILogFile *logfile = &logfileLocal;

//////////////////////////////////////////////////////////////////////////

const size_t CLogFile :: c_MaxLogFileOutputString = 2048;
const char CLogFile :: c_DefaultLogFileName[] = "tasse.log";

void CLogFile :: FormatTimeString( char *buf, size_t bufSize, const time_t *t ) const
{
#if defined(_WIN32)
#if defined(_MSC_VER)
	ctime_s( buf, bufSize, t );
#else
	const char *s = ctime( t );
	strncat_s( buf, bufSize, s, bufSize - 1 );
#endif
#else
	(void)( bufSize );	// don't warn about unreferenced parameter
	ctime_r( t, buf );
#endif
}

void CLogFile :: SecondsToDHMS( uint32 elapsed_time, uint32 &days, uint32 &hours, uint32 &minutes, uint32 &seconds ) const
{
	seconds = elapsed_time % 60;
	elapsed_time /= 60;

	minutes = elapsed_time % 60;
	elapsed_time /= 60;

	hours = elapsed_time % 24;
	elapsed_time /= 24;

	days = elapsed_time;
}

void CLogFile :: Open( const char *filename )
{
	assert( filename != nullptr );

	if ( fopen_s( &fp_, filename, "w" ) ) {
		nolog_ = true;
		return;
	}

	time_t t;
	time( &t );

	char timeBuf[64];
	FormatTimeString( timeBuf, sizeof(timeBuf), &t );

	fprintf( fp_, "=========================================================================\n" );
	fprintf( fp_, "\t" PROGRAM_SHORT_NAME " (" PROGRAM_EXE_TYPE " " PROGRAM_ARCH ") started at %s", timeBuf );
	fprintf( fp_, "=========================================================================\n" );

	nolog_ = false;
}

void CLogFile :: Close()
{
	if ( fp_ ) {
		time_t t;
		time( &t );

		char timeBuf[64];
		FormatTimeString( timeBuf, sizeof(timeBuf), &t );

		fprintf( fp_, "=========================================================================\n" );
		fprintf( fp_, "\t" PROGRAM_SHORT_NAME " (" PROGRAM_EXE_TYPE " " PROGRAM_ARCH ") shutdown at %s", timeBuf );
		fprintf( fp_, "=========================================================================\n" );

		fclose( fp_ );
		fp_ = nullptr;
	}
}

void CLogFile :: Write( const char *msg )
{
	if ( !nolog_ && !fp_ )
		this->Open( c_DefaultLogFileName );

	if ( nolog_ )
		return;

	for ( const char *s = msg; *s; ) {
		if ( 0[s] == '^' ) {
			const int c = 1[s] - '0';
			if ( c >= 0 && c < CC_NUM_CODES ) {
				s += 2;	continue;
			}
		}
		fputc( *s++, fp_ );
	}

#if defined(_WIN32) && defined(_DEBUG)
	OutputDebugString( msg );
#endif
}

void CLogFile :: Print( const char *fmt, ... )
{
	assert( fmt != nullptr );
	char output[c_MaxLogFileOutputString];

	va_list argptr;
	va_start( argptr, fmt );
	_vsnprintf_s( output, sizeof(output), sizeof(output)-1, fmt, argptr );
	va_end( argptr );

	this->Write( output );

	if ( gpGlobals->verbose )
		console->Write( output );
}

void CLogFile :: Warning( const char *fmt, ... )
{
	assert( fmt != nullptr );
	char output[c_MaxLogFileOutputString];

	va_list argptr;
	va_start( argptr, fmt );
	_vsnprintf_s( output, sizeof(output), sizeof(output)-1, fmt, argptr );
	va_end( argptr );

	this->Print( CC_YELLOW "WARNING: " CC_DEFAULT "%s", output );
}

void CLogFile :: LogTimeElapsed( double elapsed_time )
{
	const double elapsed_seconds = elapsed_time / 1000.0;
	uint32 itime = static_cast<uint32>( elapsed_seconds );
	uint32 days = 0;
	uint32 hours = 0;
	uint32 minutes = 0;
	uint32 seconds = 0;

	SecondsToDHMS( itime, days, hours, minutes, seconds );

	if ( days ) {
		Print( "%.2f seconds elapsed [%ud %uh %um %us]\n", elapsed_seconds, days, hours, minutes, seconds );
	} else if ( hours ) {
		Print( "%.2f seconds elapsed [%uh %um %us]\n", elapsed_seconds, hours, minutes, seconds );
	} else if ( minutes ) {
		Print( "%.2f seconds elapsed [%um %us]\n", elapsed_seconds, minutes, seconds );
	} else {
		Print( "%.2f seconds elapsed\n", elapsed_seconds );
	}
}
