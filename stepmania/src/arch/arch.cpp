/*
 * This file provides functions to create driver objects.
 */

#include "global.h"
#include "RageLog.h"
#include "RageUtil.h"
#include "PrefsManager.h"
#include "arch.h"
#include "Foreach.h"
#include "LocalizedString.h"
#include "arch_default.h"


void MakeLightsDrivers( const RString &driver, vector<LightsDriver *> &Add )
{
	LOG->Trace( "Initializing lights driver: %s", driver.c_str() );

	LightsDriver *ret = NULL;

#ifdef USE_LIGHTS_DRIVER_LINUX_PARALLEL
	if( !driver.CompareNoCase("LinuxParallel") )	ret = new LightsDriver_LinuxParallel;
#endif
#ifdef USE_LIGHTS_DRIVER_LINUX_WEEDTECH
	if( !driver.CompareNoCase("WeedTech") )		ret = new LightsDriver_LinuxWeedTech;
#endif
#ifdef USE_LIGHTS_DRIVER_WIN32_PARALLEL
	if( !driver.CompareNoCase("Parallel") )		ret = new LightsDriver_Win32Parallel;
#endif
#ifdef USE_LIGHTS_DRIVER_EXPORT
	if( !driver.CompareNoCase("Export") )		ret = new LightsDriver_Export;
#endif

	if( ret == NULL && driver.CompareNoCase("Null") )
		LOG->Trace( "Unknown lights driver name: %s", driver.c_str() );
	else if( ret != NULL )
		Add.push_back( ret );

	Add.push_back( new LightsDriver_SystemMessage );
}

LoadingWindow *MakeLoadingWindow()
{
	if( !PREFSMAN->m_bShowLoadingWindow )
		return new LoadingWindow_Null;
#if defined(LINUX) && !defined(HAVE_GTK)
	return new LoadingWindow_Null;
#endif
	/* Don't load NULL by default. */
	const RString drivers = "xbox,win32,cocoa,gtk";
	vector<RString> DriversToTry;
	split( drivers, ",", DriversToTry, true );

	ASSERT( DriversToTry.size() != 0 );

	RString Driver;
	LoadingWindow *ret = NULL;

	for( unsigned i = 0; ret == NULL && i < DriversToTry.size(); ++i )
	{
		Driver = DriversToTry[i];

#ifdef USE_LOADING_WINDOW_COCOA
		if( !DriversToTry[i].CompareNoCase("Cocoa") )	ret = new LoadingWindow_Cocoa;
#endif
#ifdef USE_LOADING_WINDOW_GTK
		if( !DriversToTry[i].CompareNoCase("Gtk") )	ret = new LoadingWindow_Gtk;
#endif
#ifdef USE_LOADING_WINDOW_NULL
		if( !DriversToTry[i].CompareNoCase("Null") )	ret = new LoadingWindow_Null;
#endif
#ifdef USE_LOADING_WINDOW_WIN32
		if( !DriversToTry[i].CompareNoCase("Win32") )	ret = new LoadingWindow_Win32;
#endif
#ifdef USE_LOADING_WINDOW_XBOX
		if( !DriversToTry[i].CompareNoCase("Xbox") )	ret = new LoadingWindow_Xbox;
#endif

			
		if( ret == NULL )
			continue;

		RString sError = ret->Init();
		if( sError != "" )
		{
			LOG->Info( "Couldn't load driver %s: %s", DriversToTry[i].c_str(), sError.c_str() );
			SAFE_DELETE( ret );
		}
	}
	
	if(ret)
		LOG->Info( "Loading window: %s", Driver.c_str() );
	
	return ret;
}

#if defined(SUPPORT_OPENGL)
LowLevelWindow *MakeLowLevelWindow()
{
	return new ARCH_LOW_LEVEL_WINDOW;
}
#endif

MemoryCardDriver *MakeMemoryCardDriver()
{
	MemoryCardDriver *ret = NULL;

#ifdef ARCH_MEMORY_CARD_DRIVER
	ret = new ARCH_MEMORY_CARD_DRIVER;
#endif

	if( !ret )
		ret = new MemoryCardDriver_Null;
	
	return ret;
}

/*
 * (c) 2002-2005 Glenn Maynard, Ben Anderson
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
