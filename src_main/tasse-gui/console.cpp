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
#include "window.h"

#include <QtGui/QApplication>

class CConsole : public IConsole
{
public:
	virtual void Print( const char *fmt, ... );
	virtual void NPrint( const char *fmt, ... );
	virtual void Write( const char *msg );
private:
	static int s_colorMap[CC_NUM_CODES];
};

static CConsole consoleLocal;
IConsole *console = &consoleLocal;

//////////////////////////////////////////////////////////////////////////

int CConsole :: s_colorMap[CC_NUM_CODES] = {
	0,
	0xffffff,
	0xff0000,
	0x00ff00,
	0xff8800,
	0x0000ff,
	0x00ffff,
	0xff00ff
};

void CConsole :: Write( const char *msg )
{
	char buffer[512];
	size_t pos = 0;
	int color = 0;

	for ( const char *s = msg; *s; ) {
		if ( 0[s] == '^' ) {
			const int c = 1[s] - '0';
			if ( c >= 0 && c < CC_NUM_CODES ) {
				if ( pos ) {
					buffer[pos] = '\0';
					CMainWindow::Instance().consolePuts( buffer, color );
					pos = 0;
				}
				color = s_colorMap[c];
				s += 2;	continue;
			}
		}
		buffer[pos] = *s++;
		if ( ++pos == sizeof(buffer) - 1 ) {
			buffer[pos] = '\0';
			CMainWindow::Instance().consolePuts( buffer, color );
			pos = 0;
		}

	}
	if ( pos ) {
		buffer[pos] = '\0';
		CMainWindow::Instance().consolePuts( buffer, color );
	}

	QApplication::processEvents();
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
