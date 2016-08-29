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
#ifndef TASSE_CONSOLE_H
#define TASSE_CONSOLE_H

// TASSE console interface
// prints (optionally with colors) formatted messages to the console

// console color format specifiers
#define CC_DEFAULT		"^0"
#define CC_WHITE		"^1"
#define CC_RED			"^2"
#define CC_GREEN		"^3"
#define CC_YELLOW		"^4"
#define CC_BLUE			"^5"
#define CC_CYAN			"^6"
#define CC_MAGENTA		"^7"
#define CC_NUM_CODES	8

#define MAX_CONSOLE_OUTPUT_STRING	4096

interface IConsole
{
	virtual void Print( const char *fmt, ... ) = 0;
	virtual void NPrint( const char *fmt, ... ) = 0;
	virtual void Write( const char *msg ) = 0;
};

extern IConsole *console;

#endif //TASSE_CONSOLE_H
