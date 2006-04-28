#include "global.h"
#include "ArchHooks.h"
#include "RageLog.h"
#include "RageThreads.h"

bool ArchHooks::s_bQuitting = false;
bool ArchHooks::s_bToggleWindowed = false;
static bool g_bHasFocus = true;
// Keep from pulling RageThreads.h into ArchHooks.h
static RageMutex g_Mutex( "ArchHooks" );
ArchHooks *HOOKS = NULL;

bool ArchHooks::GetAndClearToggleWindowed()
{
	LockMut( g_Mutex );
	bool bToggle = s_bToggleWindowed;
	
	s_bToggleWindowed = false;
	return bToggle;
}

void ArchHooks::SetToggleWindowed()
{
	LockMut( g_Mutex );
	s_bToggleWindowed = true;
}

#include "Selector_ArchHooks.h"
ArchHooks *MakeArchHooks()
{
	return new ARCH_HOOKS;
}

void ArchHooks::SetHasFocus( bool bHasFocus )
{
	if( bHasFocus == g_bHasFocus )
		return;
	g_bHasFocus = bHasFocus;

	LOG->Trace( "App %s focus", bHasFocus? "has":"doesn't have" );
}

bool ArchHooks::AppHasFocus()
{
	return g_bHasFocus;
}

bool ArchHooks::GoToURL( RString sUrl )
{
	return false;
}

/*
 * (c) 2003-2004 Glenn Maynard, Chris Danford
 * All rights reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, and/or sell copies of the Software, and to permit persons to
 * whom the Software is furnished to do so, provided that the above
 * copyright notice(s) and this permission notice appear in all copies of
 * the Software and that both the above copyright notice(s) and this
 * permission notice appear in supporting documentation.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF
 * THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS
 * INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
