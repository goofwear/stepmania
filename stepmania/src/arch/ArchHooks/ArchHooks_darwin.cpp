#include "global.h"
#include "ArchHooks_darwin.h"
#include "RageLog.h"
#include "RageUtil.h"
#include "archutils/Darwin/Crash.h"
#include "archutils/Unix/CrashHandler.h"
#include "archutils/Unix/SignalHandler.h"
#include "ProductInfo.h"
#include <Carbon/Carbon.h>
#include <mach/mach.h>
#include <mach/host_info.h>
#include <mach/mach_time.h>

static bool IsFatalSignal( int signal )
{
	switch( signal )
	{
	case SIGINT:
	case SIGTERM:
	case SIGHUP:
		return false;
	default:
		return true;
	}
}

static void DoCleanShutdown( int signal, siginfo_t *si, const ucontext_t *uc )
{
	if( IsFatalSignal(signal) )
		return;

	/* ^C. */
	ArchHooks::SetUserQuit();
}

#if defined(CRASH_HANDLER)
static void DoCrashSignalHandler( int signal, siginfo_t *si, const ucontext_t *uc )
{
	/* Don't dump a debug file if the user just hit ^C. */
	if( !IsFatalSignal(signal) )
		return;

	CrashHandler::CrashSignalHandler( signal, si, uc );
	/* not reached */
}
#endif

ArchHooks_darwin::ArchHooks_darwin()
{
	/* First, handle non-fatal termination signals. */
	SignalHandler::OnClose( DoCleanShutdown );
	
#if defined(CRASH_HANDLER)
	CrashHandler::CrashHandlerHandleArgs( g_argc, g_argv );
	SignalHandler::OnClose( DoCrashSignalHandler );
#endif
	
	// CF*Copy* functions' return values need to be released, CF*Get* functions' do not.
	CFStringRef key = CFSTR( "ApplicationBundlePath" );
	
	CFBundleRef bundle = CFBundleGetMainBundle();
	CFStringRef appID = CFBundleGetIdentifier( bundle );
	CFStringRef version = CFStringRef( CFBundleGetValueForInfoDictionaryKey(bundle, kCFBundleVersionKey) );
	CFURLRef path = CFBundleCopyBundleURL( bundle );
	CFPropertyListRef value = CFURLCopyPath( path );
	CFPropertyListRef old = CFPreferencesCopyAppValue( key, appID );
	CFMutableDictionaryRef newDict = NULL;
	
	if( old && CFGetTypeID(old) != CFDictionaryGetTypeID() )
	{
		CFRelease( old );
		old = NULL;
	}
	
	if( !old )
	{
		newDict = CFDictionaryCreateMutable( kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks,
						     &kCFTypeDictionaryValueCallBacks );
		CFDictionaryAddValue( newDict, version, value );
	}
	else
	{
		CFTypeRef oldValue;
		CFDictionaryRef dict = CFDictionaryRef( old );
		
		if( !CFDictionaryGetValueIfPresent(dict, version, &oldValue) || !CFEqual(oldValue, value) )
		{
			// The value is either not present or it is but it is different
			newDict = CFDictionaryCreateMutableCopy( kCFAllocatorDefault, 0, dict );
			CFDictionaryAddValue( newDict, version, value );
		}
		CFRelease( old );
	}
	
	if( newDict )
	{
		CFPreferencesSetAppValue( key, newDict, appID );
		if( !CFPreferencesAppSynchronize(appID) )
			LOG->Warn( "Failed to record the run path." );
		CFRelease( newDict );
	}
	CFRelease( value );
	CFRelease( path );
}

void ArchHooks_darwin::DumpDebugInfo()
{
	RString systemVersion;
	OSErr err;
	long code;
	
	/* Get system version */
	err = Gestalt( gestaltSystemVersion, &code );
	if( err == noErr )
	{
		int major = ( (code >> 12) & 0xF ) * 10 + (code >> 8) & 0xF;
		int minor = (code >> 4) & 0xF;
		int revision = code & 0xF;
		
		systemVersion = ssprintf( "Mac OS X %d.%d.%d", major, minor, revision );
	}
	else
		systemVersion = "Unknown system version";
	
	/* Get memory */
	long ram;
	long vRam;
	
	err = Gestalt( gestaltLogicalRAMSize, &vRam );
	if (err != noErr)
		vRam = 0;
	err = Gestalt( gestaltPhysicalRAMSize, &ram );
	if( err == noErr )
	{
		vRam -= ram;
		if( vRam < 0 )
			vRam = 0;
		ram >>= 20;
		vRam >>= 20;
	}
	else
	{
		ram = 0;
		vRam = 0;
	}
	
	/* Get processor information */
	RString processor;
	int numProcessors;	
	struct host_basic_info info;
	mach_port_t host = mach_host_self();
	mach_msg_type_number_t size = HOST_BASIC_INFO_COUNT;
	kern_return_t ret = host_info( host, HOST_BASIC_INFO, (host_info_t)&info, &size );
	
	if( ret )
	{
		numProcessors = -1;
		LOG->Warn("Couldn't get host info.");
	}
	else
	{
		char *cpu_type, *cpu_subtype;
		
		numProcessors = info.avail_cpus;
		slot_name( info.cpu_type, info.cpu_subtype, &cpu_type, &cpu_subtype );
		processor = cpu_subtype;
	}
	
	/* Get processor speed */
	err = Gestalt( gestaltProcClkSpeed, &code );
	if( err != noErr )
		code = 0;
	
	float speed;
	char power;
	
	if( code >= 1000000000 )
	{
		speed = code / 1000000000.0f;
		power = 'G';
	}
	else
	{
		speed = code / 1000000.0f;
		power = 'M';
	}
	
	/* Send all of the information to the log */
	LOG->Info( "Processor: %s (%d)", processor.c_str(), numProcessors );
	LOG->Info( "Clock speed %.2f %cHz", speed, power );
	LOG->Info( "%s", systemVersion.c_str());
	LOG->Info( "Memory: %ld MB total, %ld MB swap", ram, vRam );
}

// In archutils/darwin/PreferredLanguage.m
extern "C"
{
	extern char *GetPreferredLanguage();
}

RString ArchHooks::GetPreferredLanguage()
{
	char *lang = ::GetPreferredLanguage();
	RString ret = lang;
	
	free(lang);
	return ret.ToUpper();
}

int64_t ArchHooks::GetMicrosecondsSinceStart( bool bAccurate )
{
	static uint64_t iStartTime = mach_absolute_time();
	static double factor = 0.0;
	
	if( unlikely(factor == 0.0) )
	{
		mach_timebase_info_data_t timeBase;
		
		mach_timebase_info( &timeBase );
		factor = timeBase.numer / ( 1000.0 * timeBase.denom );
	}
	return int64_t( (mach_absolute_time() - iStartTime) * factor );
}

#include "RageFileManager.h"

void ArchHooks::MountInitialFilesystems( const RString &sDirOfExecutable )
{
	CFBundleRef bundle = CFBundleGetMainBundle();
	CFURLRef bundleURL = CFBundleCopyBundleURL( bundle );
	CFURLRef dirURL = CFURLCreateCopyDeletingLastPathComponent( kCFAllocatorDefault, bundleURL );
	char dir[PATH_MAX];
	
	if( !CFURLGetFileSystemRepresentation(dirURL, true, (UInt8 *)dir, sizeof(dir)) )
		FAIL_M( "CFURLGetFileSystemRepresentation() failed." );
	CFRelease( bundleURL );
	CFRelease( dirURL );
	FILEMAN->Mount( "dir", dir, "/" );
	
	FSRef fs; // This does not need to be "released" by the file manager
	
	// This returns the absolute path for ~/Library/Application Support
	if( FSFindFolder(kUserDomain, kApplicationSupportFolderType, kDontCreateFolder, &fs) )
		FAIL_M( "FSFindFolder() failed." );
	if( FSRefMakePath(&fs, (UInt8 *)dir, sizeof(dir)) )
		FAIL_M( "FSRefMakePath() failed." );
	FILEMAN->Mount( "dir", ssprintf("%s/" PRODUCT_ID "/Save", dir), "/Save" );
	FILEMAN->Mount( "dir", ssprintf("%s/" PRODUCT_ID "/Screenshots", dir), "/Screenshots" );
	/* The Cache directory should probably go in ~/Library/Caches/bundle_identifier/Cache but
	 * why bother we're already using ~/Library/Application Support/PRODUCT_ID. */
	FILEMAN->Mount( "dir", ssprintf("%s/" PRODUCT_ID "/Cache", dir), "/Cache" );
}

/*
 * (c) 2003-2006 Steve Checkoway
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
