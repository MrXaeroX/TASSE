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

class CConsole : public IConsole
{
public:
	virtual void Print( const char *fmt, ... );
	virtual void NPrint( const char *fmt, ... );
	virtual void Write( const char *msg );
protected:
	void SetColor( int color );
private:
	static int s_colorMap[CC_NUM_CODES];
};

static CConsole consoleLocal;
IConsole *console = &consoleLocal;

//////////////////////////////////////////////////////////////////////////

int CConsole :: s_colorMap[CC_NUM_CODES] = {
#if defined(_WIN32)
	FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE,
	FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY,
	FOREGROUND_RED | FOREGROUND_INTENSITY,
	FOREGROUND_GREEN | FOREGROUND_INTENSITY,
	FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY,
	FOREGROUND_BLUE | FOREGROUND_INTENSITY,
	FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY,
	FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY
#else //!_WIN32
	0,
	97,
	91,
	92,
	93,
	94,
	96,
	95
#endif //_WIN32
};

void CConsole :: SetColor( int color )
{
#if defined(_WIN32)
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	HANDLE hStdHandle = GetStdHandle( STD_OUTPUT_HANDLE );

	// use csbi for the wAttributes word
	if ( GetConsoleScreenBufferInfo( hStdHandle, &csbi )) {
		// mask out all but the background attribute, and add in the foreground color
		WORD wColor = ( csbi.wAttributes & 0xF0 ) + ( s_colorMap[color] & 0x0F );
		SetConsoleTextAttribute( hStdHandle, wColor );     
	}
#else //!_WIN32
	fprintf( stdout, "\033[%im", s_colorMap[color] );
#endif //_WIN32
}

void CConsole :: Write( const char *msg )
{
	bool cc = false;
	for ( const char *s = msg; *s; ) {
		if ( 0[s] == '^' ) {
			const int c = 1[s] - '0';
			if ( c >= 0 && c < CC_NUM_CODES ) {
				cc = c != 0;
				this->SetColor( c );
				s += 2;	continue;
			}
		}
		fputc( *s++, stdout );
	}
	if ( cc ) 
		this->SetColor( 0 );
}

void CConsole :: Print( const char *fmt, ... )
{
	assert( fmt != nullptr );
	char output[MAX_CONSOLE_OUTPUT_STRING];

	va_list argptr;
	va_start( argptr, fmt );
	_vsnprintf_s( output, sizeof(output), sizeof(output)-1, fmt, argptr );
	va_end( argptr );

	this->Write( output );
	logfile->Write( output );
}

void CConsole :: NPrint( const char *fmt, ... )
{
	assert( fmt != nullptr );
	char output[MAX_CONSOLE_OUTPUT_STRING];

	va_list argptr;
	va_start( argptr, fmt );
	_vsnprintf_s( output, sizeof(output), sizeof(output)-1, fmt, argptr );
	va_end( argptr );

	this->Write( output );
}
