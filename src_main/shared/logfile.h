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
#ifndef TASSE_LOGFILE_H
#define TASSE_LOGFILE_H

// TASSE log file interface
// prints formatted messages to the log file
// color codes will be stripped

interface ILogFile
{
	virtual void Open( const char *filename ) = 0;
	virtual void Print( const char *fmt, ... ) = 0;
	virtual void Warning( const char *fmt, ... ) = 0;
	virtual void Write( const char *msg ) = 0;
	virtual void Close() = 0;
	virtual void LogTimeElapsed( double elapsed_time ) = 0;
};

extern ILogFile *logfile;

#endif //TASSE_LOGFILE_H
