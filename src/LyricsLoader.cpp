#include "global.h"
/*
	Loader for lyrics files
	By: Kevin Slaughter
*/
#include "RageLog.h"
#include "RageException.h"
#include "RageUtil.h"
#include "LyricsLoader.h"
#include "ThemeManager.h"
#include "RageFile.h"
#include <map>
using namespace std;

#define LYRICS_DEFAULT_COLOR      THEME->GetMetricC("ScreenGameplay","LyricsDefaultColor")

static int CompareLyricSegments(const LyricSegment &seg1, const LyricSegment &seg2)
{
   return seg1.m_fStartTime < seg2.m_fStartTime;
}

bool LyricsLoader::LoadFromLRCFile(const CString& sPath, Song& out)
{
	LOG->Trace( "LyricsLoader::LoadFromLRCFile(%s)", sPath.c_str() );
	
	RageFile input( sPath );
	
	if (!input.IsOpen())
	{
		LOG->Warn("Error opening file '%s' for reading.", sPath.c_str());
		return false;
	}
	
	RageColor CurrentColor = LYRICS_DEFAULT_COLOR;
	
	out.m_LyricSegments.clear();
	
	while( 1 )
	{
		CString line;
		int ret = input.GetLine( line );
		if( ret == 0 )
			break;
		if( ret == -1 )
		{
			LOG->Warn("Error reading %s: %s", input.GetPath().c_str(), input.GetError().c_str() );
			break;
		}

		if(!line.compare(0, 2, "//"))
			continue;
		
		/* "[data1] data2".  Ignore whitespace at the beginning of the line. */
		static Regex x("^ *\\[([^]]+)\\] *(.*)$");
		
		vector<CString> matches;
		if(!x.Compare(line, matches))
			continue;
		ASSERT( matches.size() == 2 );
		
		CString &sValueName = matches[0];
		CString &sValueData = matches[1];
		StripCrnl(sValueData);
		
		// handle the data
		if( 0==stricmp(sValueName,"COLOUR") || 0==stricmp(sValueName,"COLOR") )
		{
			// set color var here for this segment
			int r, g, b;
			int result = sscanf( sValueData.c_str(), "0x%2x%2x%2x", &r, &g, &b );
			if(result != 3)
			{
				LOG->Trace( "The color value '%s' in '%s' is invalid.",
				sValueData.c_str(), sPath.c_str() );
				continue;
			}
			
			CurrentColor = RageColor(r / 256.0f, g / 256.0f, b / 256.0f, 1);
			continue;
		}
		
		{
			/* If we've gotten this far, and no other statement caught
			* this value before this does, assume it's a time value. */		
			
			LyricSegment seg;
			seg.m_Color = CurrentColor;
			seg.m_fStartTime = TimeToSeconds(sValueName);
			seg.m_sLyric = sValueData;
			
			seg.m_sLyric.Replace( "|","\n" ); // Pipe symbols denote a new line in LRC files
			out.AddLyricSegment( seg );
		}
	}
	
	sort( out.m_LyricSegments.begin(), out.m_LyricSegments.end(), CompareLyricSegments );
	LOG->Trace( "LyricsLoader::LoadFromLRCFile done" );
	
	return true;
}
	
/*
-----------------------------------------------------------------------------
 Copyright (c) 2003 by the person(s) listed below.  All rights reserved.
	Kevin Slaughter
	Glenn Maynard
-----------------------------------------------------------------------------
*/
