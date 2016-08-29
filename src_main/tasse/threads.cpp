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

#if defined(_DEBUG)
#define THREAD_DEBUG
#endif

#if !defined(THREAD_DEBUG)
#define ThreadDebug( x )	do {} while ( false )
#elif defined(_WIN32)
#define ThreadDebug( x )	OutputDebugString( x )
#else
#define ThreadDebug( x )	puts( x )
#endif

#if defined(_QTASSE)
#define THREAD_CHECK_TIMEOUT	50
#else
#define THREAD_CHECK_TIMEOUT	200
#endif

#define THREADTIMES_SIZE		100
#define THREADTIMES_SIZE_F		(float)(THREADTIMES_SIZE)

static uint32 dispatch = 0;
static uint32 workcount = 0;
static float progress = 0.0f;
static uint32 runflags = 0;
static bool thread_interrupt = false;
static ThreadStub_t workfunction;
static double threadtimes[THREADTIMES_SIZE];
static uint32 oldf = 0;
extern void ThreadUpdateGUI( float progress );

void ThreadInterrupt()
{
	thread_interrupt = true;
}

bool ThreadInterrupted()
{
	return thread_interrupt;
}

static uint32 ThreadGetWork()
{
	ThreadLock();

	if ( thread_interrupt ) {
		ThreadDebug( "ThreadGetWork: thread interrupted\n" );
		ThreadUnlock();
		return -1;
	}
	if ( dispatch > workcount ) {
		ThreadDebug( "ThreadGetWork: dispatch > workcount!\n" );
		ThreadUnlock();
		return -1;
	}
	if ( dispatch == workcount ) {
		ThreadDebug( "ThreadGetWork: dispatch == workcount, work is complete\n" );
		ThreadUnlock();
		return -1;
	}

	progress = (float)dispatch / workcount;

	if ( runflags & RF_PROGRESS ) {
		const uint32 f = THREADTIMES_SIZE * dispatch / workcount;

		if ( runflags & RF_PACIFIER ) {
			fprintf_s( stdout, "\r%6u /%6u", dispatch, workcount );

			if ( f != oldf ) {
				double ct = utils->FloatMilliseconds() * 0.001;

				// fill in current time for threadtimes record
				for ( uint32 i = oldf; i <= f; ++i ) {
					if ( threadtimes[i] < 1 ) 
						threadtimes[i] = ct;
				}
				oldf = f;

				if ( f > 10 ) {
					double finish = (ct - threadtimes[0]) * (THREADTIMES_SIZE_F - f) / f;
					double finish2 = 10.0 * (ct - threadtimes[f - 10]) * (THREADTIMES_SIZE_F - f) / THREADTIMES_SIZE_F;
					double finish3 = THREADTIMES_SIZE_F * (ct - threadtimes[f - 1]) * (THREADTIMES_SIZE_F - f) / THREADTIMES_SIZE_F;

					if ( finish > 1.0 ) {
						fprintf_s( stdout, "  (%u%%: est. time to completion %ld/%ld/%ld secs)   ", f, (long)(finish), (long)(finish2), (long)(finish3) );
						fflush( stdout );
					} else {
						fprintf_s( stdout, "  (%u%%: est. time to completion <1 sec)         ", f );
						fflush( stdout );
					}
				}
			}
		} else {
			if ( f != oldf ) {
				oldf = f;
				if ( f && !( f % 10 ) ) {
#if defined(_QTASSE)
					console->NPrint( "%u%%...", f );
#else
					fprintf_s( stdout, "%u%%...", f );
					fflush( stdout );
#endif
				}
			}
		}
	}

	uint32 r = dispatch++;
	ThreadUnlock();

	return r;
}

static void ThreadWorkerFunction( const uint32 threadnum, const uint32 )
{
	int work;

	while ( ( work = ThreadGetWork() ) != -1 ) {
#if defined(THREAD_DEBUG)
		char msgBuf[256];
		memset( msgBuf, 0, sizeof(msgBuf) );
		sprintf_s( msgBuf, sizeof(msgBuf), "Thread %i: work %i\n", threadnum, work );
		ThreadDebug( msgBuf );
#endif
		workfunction( threadnum, work );
	}

	ThreadDebug( "ThreadWorkerFunction: exit!\n" );
}

void RunThreadsOnIndividual( uint32 workcnt, uint32 flags, ThreadStub_t func )
{
	workfunction = func;
	RunThreadsOn( workcnt, flags, ThreadWorkerFunction );
}

#if defined(USE_WIN32_THREADS)

static int numthreads = -1;
static int oldpriority = 0;
static bool threaded = false;
static bool lowpriority = false;
static bool crit_init = false;
static CRITICAL_SECTION crit;
static int enter;
ThreadStub_t thread_entry;

void ThreadSetDefault( int count, bool low_priority )
{
	lowpriority = low_priority;
	numthreads = count;

	if ( numthreads == -1 ) {
		SYSTEM_INFO info;
		GetSystemInfo(&info);
		numthreads = info.dwNumberOfProcessors;
	}

	if ( numthreads < 1 )
		numthreads = 1;
	else if ( numthreads > MAX_THREADS )
		numthreads = MAX_THREADS;

	logfile->Print( "ThreadSetDefault: %i thread(s)\n", numthreads );

	if ( numthreads > 1 && !crit_init ) {
		InitializeCriticalSection( &crit );
		crit_init = true;
	}
}

void ThreadCleanup()
{
	if ( crit_init ) {
		DeleteCriticalSection( &crit );
		crit_init = false;
	}
}

int ThreadCount()
{
	return numthreads;
}

void ThreadLock()
{
	if ( !threaded )
		return;
	EnterCriticalSection( &crit );
	if ( enter )
		ThreadDebug( "ThreadLock: recursive thread lock!\n" );
	++enter;
}

void ThreadUnlock()
{
	if (!threaded)
		return;
	if ( !enter )
		ThreadDebug( "ThreadUnlock: no lock!\n" );
	--enter;
	LeaveCriticalSection( &crit );
}

static void ThreadSetPriority()
{
	int newpriority = lowpriority ? IDLE_PRIORITY_CLASS : NORMAL_PRIORITY_CLASS;
	oldpriority = GetPriorityClass( GetCurrentProcess() );
	SetPriorityClass( GetCurrentProcess(), newpriority );
}

static void ThreadResetPriority()
{
	SetPriorityClass( GetCurrentProcess(), oldpriority );
}

static DWORD WINAPI ThreadEntryStub( LPVOID pParam )
{
	thread_entry( (uint32)pParam, 0 );
	return 0;
}

void RunThreadsOn( uint32 workcnt, uint32 flags, ThreadStub_t func )
{
	DWORD threadid[MAX_THREADS];
	HANDLE threadhandle[MAX_THREADS];
	double start = utils->FloatMilliseconds() * 0.001;

	dispatch = 0;
	workcount = workcnt;
	runflags = flags;
	thread_interrupt = false;

	if ( flags & RF_PACIFIER )
		memset( threadtimes, 0, sizeof(threadtimes) );

	ThreadSetPriority();

	if ( ThreadCount() <= 1 ) {
		// single-threaded
		func( 0, 0 );
		ThreadResetPriority();
		goto finalmsg;
	}

	threaded = true;
	thread_entry = func;

	// create threads
	for ( int i = 0; i < numthreads; ++i ) {
		HANDLE hThread = CreateThread( nullptr, 0, (LPTHREAD_START_ROUTINE)ThreadEntryStub, (LPVOID)i, CREATE_SUSPENDED, &threadid[i] );
		if ( hThread )
			threadhandle[i] = hThread;
		else
			ThreadDebug( "Unable to create thread!\n" );
	}

	// start threads
	for ( long i = 0; i < numthreads; ++i ) {
		if ( ResumeThread( threadhandle[i] ) == 0xffffffff )
			ThreadDebug( "Unable to resume thread!\n" );
	}

#if defined(THREAD_DEBUG)
	char msgBuf[256];
	memset( msgBuf, 0, sizeof(msgBuf) );
	sprintf_s( msgBuf, sizeof(msgBuf), "Started %i threads\n", numthreads );
	ThreadDebug( msgBuf );
#endif

	// wait for threads to complete
	for ( ;; ) {
		DWORD dwWaitResult = WaitForMultipleObjects( numthreads, threadhandle, TRUE, THREAD_CHECK_TIMEOUT );
		if ( dwWaitResult == WAIT_TIMEOUT ) {
#if defined(_QTASSE)
			ThreadUpdateGUI( progress );
#endif
		} else break;
	}

	thread_entry = nullptr;
	threaded = false;
	ThreadResetPriority();

finalmsg:
	if ( ( flags & RF_PROGRESS ) && !thread_interrupt ) {
		double end = utils->FloatMilliseconds() * 0.001;
		if ( flags & RF_PACIFIER ) {
			fprintf_s( stdout, "\r%60s\r", "" );
			fflush( stdout );
		} else {
#if defined(_QTASSE)
			console->NPrint( "100%% " );
#else
			fprintf_s( stdout, "100%% " );
			fflush( stdout );
#endif
		}
#if defined(_QTASSE)
		console->NPrint( "(%.2f seconds)\n", end - start );
#else
		fprintf_s( stdout, "(%.2f seconds)\n", end - start );
#endif
	}
}

#elif defined(USE_POSIX_THREADS)

static int numthreads = -1;
static bool threaded = false;
static bool lowpriority = false;
pthread_mutex_t *pth_mutex = nullptr;
pthread_mutexattr_t pth_mutex_attr;
static int enter;
ThreadStub_t thread_entry;
static unsigned long sys_timeBase = 0;
static int curtime;

static int ThreadMilliseconds()
{
	struct timeval tp;
	gettimeofday( &tp, nullptr );
	if ( !sys_timeBase ) {
		sys_timeBase = tp.tv_sec;
		return tp.tv_usec/1000;
	}
	curtime = ( tp.tv_sec - sys_timeBase ) * 1000 + tp.tv_usec / 1000;
	return curtime;
}

void ThreadSetDefault( int count, bool low_priority )
{
	lowpriority = low_priority;
	numthreads = count;

	if ( numthreads == -1 ) {
		// poll /proc/cpuinfo
		FILE *fp = nullptr;
		if ( fopen_s( &fp, "/proc/cpuinfo", "r" ) ) {
			numthreads = 1;
		} else {
			char buf[1024];
			memset(buf,0,sizeof(buf));
			numthreads = 0;
			while ( !feof( fp ) ) {
				if ( !fgets( buf, sizeof(buf)-1, fp ) )
					break;
				if ( !_strnicmp( buf, "processor", 9 ) )
					++numthreads;
			}
			fclose( fp );
		}
	}

	if ( numthreads < 1 )
		numthreads = 1;
	else if ( numthreads > MAX_THREADS )
		numthreads = MAX_THREADS;

	logfile->Print( "ThreadSetDefault: %i thread(s)\n", numthreads );

	if ( numthreads > 1 && !pth_mutex ) {
		pth_mutex = (pthread_mutex_t*)malloc( sizeof(*pth_mutex) );
		pthread_mutexattr_init( &pth_mutex_attr );
		pthread_mutex_init( pth_mutex, &pth_mutex_attr );
	}
}

void ThreadCleanup()
{
	if ( pth_mutex ) {
		free( pth_mutex );
		pth_mutex = nullptr;
	}
}

int ThreadCount()
{
	return numthreads;
}

void ThreadLock()
{
	if ( !threaded )
		return;
	pthread_mutex_lock( pth_mutex );
	if ( enter )
		ThreadDebug( "ThreadLock: recursive thread lock!\n" );
	++enter;
}

void ThreadUnlock()
{
	if ( !threaded )
		return;
	if ( !enter )
		ThreadDebug( "ThreadUnlock: no lock!\n" );
	--enter;
	pthread_mutex_unlock( pth_mutex );
}

static void ThreadSetPriority()
{
	if ( lowpriority )
		setpriority( PRIO_PROCESS, 0, PRIO_MIN );
}

static void ThreadResetPriority()
{
	setpriority( PRIO_PROCESS, 0, 0 );
}

static void *ThreadEntryStub( void *pParam )
{
	long lParam = (long)pParam;
	thread_entry( (uint32)lParam, 0 );
	return nullptr;
}

void RunThreadsOn( uint32 workcnt, uint32 flags, ThreadStub_t func )
{
	pthread_t threadhandle[MAX_THREADS];
	pthread_attr_t threadattrib;
	double start = utils->FloatMilliseconds() * 0.001;
	int starttime = 0;

	progress = 0;
	dispatch = 0;
	workcount = workcnt;
	runflags = flags;
	thread_interrupt = false;

	if ( flags & RF_PACIFIER )
		memset( threadtimes, 0, sizeof(threadtimes) );

	ThreadSetPriority();

	if ( ThreadCount() <= 1 ) {
		// single-threaded
		func( 0, 0 );
		ThreadResetPriority();
		goto finalmsg;
	}

	if ( flags & RF_PROGRESS )
		start = utils->FloatMilliseconds() * 0.001;

	threaded = true;
	thread_entry = func;

	pthread_attr_init( &threadattrib );

	for ( long i = 0; i < numthreads; ++i ) {
		if ( pthread_create( &threadhandle[i], &threadattrib, ThreadEntryStub, (void*)i ) == -1 )
			ThreadDebug( "Unable to create thread!\n" );
	}

#if defined(THREAD_DEBUG)
	char msgBuf[256];
	memset( msgBuf, 0, sizeof(msgBuf) );
	sprintf_s( msgBuf, sizeof(msgBuf), "Started %i threads\n", numthreads );
	ThreadDebug( msgBuf );
#endif

	// wait for threads to complete
	starttime = ThreadMilliseconds();

	for ( ;; ) {
		// check timeout
		int newtime = ThreadMilliseconds();
		if ( newtime - starttime >= THREAD_CHECK_TIMEOUT ) {
			starttime = newtime;

			// check if threads completed
			bool threads_done = true;
			for ( int i = 0; i < numthreads; ++i ) {
				if ( !pthread_kill( threadhandle[i], 0 ) ) {
					// thread alive
					threads_done = false;
					break;
				}
			}

			if ( threads_done )
				break;
#if defined(_QTASSE)
			ThreadUpdateGUI( progress );
#endif
		}
	}

	thread_entry = nullptr;
	threaded = false;
	ThreadResetPriority();

finalmsg:
	if ( ( flags & RF_PROGRESS ) && !thread_interrupt ) {
		double end = utils->FloatMilliseconds() * 0.001;
		if ( flags & RF_PACIFIER ) {
			fprintf_s( stdout, "\r%60s\r", "" );
			fflush( stdout );
		} else {
#if defined(_QTASSE)
			console->NPrint( "%u%% ", 100 );
#else
			fprintf_s( stdout, "%u%% ", 100 );
			fflush( stdout );
#endif
		}
#if defined(_QTASSE)
		console->NPrint( "(%.2f seconds)\n", end - start );
#else
		fprintf_s( stdout, "(%.2f seconds)\n", end - start );
#endif
	}
}

#else

void ThreadSetDefault( int, bool ) {}
void ThreadCleanup() {}
void ThreadLock() {}
void ThreadUnlock() {}
int ThreadCount() { return 1; }

void RunThreadsOn( uint32 workcnt, uint32 flags, ThreadStub_t func )
{
	double start = utils->FloatMilliseconds() * 0.001;

	progress = 0;
	dispatch = 0;
	workcount = workcnt;
	runflags = flags;
	thread_interrupt = false;

	if ( flags & RF_PACIFIER )
		memset( threadtimes, 0, sizeof(threadtimes) );

	func( 0, 0 );

	if ( ( flags & RF_PROGRESS ) && !thread_interrupt ) {
		double end = utils->FloatMilliseconds() * 0.001;
		if ( flags & RF_PACIFIER ) {
			fprintf_s( stdout, "\r%60s\r", "" );
			fflush( stdout );
		} else {
#if defined(_QTASSE)
			console->NPrint( "100%% " );
#else
			fprintf_s( stdout, "100%% " );
			fflush( stdout );
#endif
		}
#if defined(_QTASSE)
		console->NPrint( "(%.2f seconds)\n", end - start );
#else
		fprintf_s( stdout, "(%.2f seconds)\n", end - start );
#endif
	}
}

#endif
