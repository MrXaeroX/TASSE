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
#ifndef TASSE_THREADS_H
#define TASSE_THREADS_H

// TASSE multithreading functions

#if defined(_WIN32)
#define USE_WIN32_THREADS
#define MAX_THREADS		32
#elif defined(_LINUX)
#define USE_POSIX_THREADS
#define MAX_THREADS		32
#else
//single-threaded otherwise
#define MAX_THREADS		1
#endif

// RunThreadsOn flags
#define RF_PROGRESS		BIT( 0 )
#define RF_PACIFIER		BIT( 1 )

typedef void (*ThreadStub_t)( uint32, uint32 );

extern void ThreadInterrupt();
extern bool ThreadInterrupted();
extern void ThreadSetDefault( int count, bool low_priority );
extern void ThreadLock();
extern void ThreadUnlock();
extern int  ThreadCount();
extern void ThreadCleanup();
extern void RunThreadsOn( uint32 workcnt, uint32 flags, ThreadStub_t func );
extern void RunThreadsOnIndividual( uint32 workcnt, uint32 flags, ThreadStub_t func );

#endif //TASSE_THREADS_H
