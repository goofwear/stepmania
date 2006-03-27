/* SMLoader - Reads a Song from an .SM file. */

#ifndef NotesLoaderSM_H
#define NotesLoaderSM_H

#include "NotesLoader.h"
#include "GameConstantsAndTypes.h"

class MsdFile;
class Song;
class Steps;
class TimingData;

class SMLoader: public NotesLoader
{
	static void LoadFromSMTokens( 
		RString sStepsType, 
		RString sDescription,
		RString sDifficulty,
		RString sMeter,
		RString sRadarValues,
		RString sNoteData,		
		Steps &out);

	bool FromCache;

public:
	SMLoader() { FromCache = false; }
	bool LoadFromSMFile( const RString &sPath, Song &out );
	bool LoadFromSMFile( const RString &sPath, Song &out, bool cache )
	{
		FromCache=cache;
		return LoadFromSMFile( sPath, out );
	}

	void GetApplicableFiles( const RString &sPath, vector<RString> &out );
	bool LoadFromDir( const RString &sPath, Song &out );
	void TidyUpData( Song &song, bool cache );
	static bool LoadTimingFromFile( const RString &fn, TimingData &out );
	static void LoadTimingFromSMFile( const MsdFile &msd, TimingData &out );
	static bool LoadEditFromFile( RString sEditFilePath, ProfileSlot slot, bool bAddStepsToSong );
	static bool LoadEditFromBuffer( const RString &sBuffer, const RString &sEditFilePath, ProfileSlot slot );
	static bool LoadEditFromMsd( const MsdFile &msd, const RString &sEditFilePath, ProfileSlot slot, bool bAddStepsToSong );
};

#endif

/*
 * (c) 2001-2004 Chris Danford, Glenn Maynard
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
