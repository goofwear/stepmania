#ifndef CRASH_HANDLER_INTERNAL_H
#define CRASH_HANDLER_INTERNAL_H

#include "Backtrace.h"
#define BACKTRACE_MAX_SIZE 1024

struct CrashData
{
	enum CrashType
	{
		/* We received a fatal signal.  si and uc are valid. */
		SIGNAL,

		/* We're forcing a crash (eg. failed ASSERT). */
		FORCE_CRASH_THIS_THREAD,

		/* Deadlock detected; give a stack trace for two threads. */
		FORCE_CRASH_DEADLOCK
	} type;

	/* Everything except FORCE_CRASH_THIS_THREAD: */
	const void *BacktracePointers[BACKTRACE_MAX_SIZE];

	/* FORCE_CRASH_DEADLOCK only: */
	const void *BacktracePointers2[BACKTRACE_MAX_SIZE];

	/* SIGNAL only: */
	int signal;
	siginfo_t si;
	
	/* FORCE_CRASH_THIS_THREAD and FORCE_CRASH_DEADLOCK only: */
	char reason[256];
};

#define CHILD_MAGIC_PARAMETER "--private-do-crash-handler"

const char *SignalName( int signo );

#endif
