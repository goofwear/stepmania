#ifndef RAGELOG_H
#define RAGELOG_H

/*
-----------------------------------------------------------------------------
 Class: RageLog

 Desc: Manages logging

 Copyright (c) 2001-2002 by the person(s) listed below.  All rights reserved.
	Chris Danford
-----------------------------------------------------------------------------
*/

#include <stdio.h>

class RageLog
{
public:
	RageLog();
	~RageLog();

	void Trace( const char *fmt, ...);
	void Trace( HRESULT hr, const char *fmt, ...);
	void Warn( const char *fmt, ...);
	void Flush();

	void ShowConsole();
	void HideConsole();

protected:
	FILE* m_fileLog;
	/* Let's keep a list of all warnings, so we can display them later.
	 * We could write this to a file, instead, but for now let's not
	 * clutter the top dir ... */
	CStringArray warnings;
};

extern RageLog*	LOG;	// global and accessable from anywhere in our program

#endif