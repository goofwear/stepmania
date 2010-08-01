/* DWILoader - reads a Song from a .DWI file. */

#ifndef NOTES_LOADER_DWI_H
#define NOTES_LOADER_DWI_H

#include "GameInput.h"
#include "NotesLoader.h"

class Song;
class Steps;

class DWILoader: public NotesLoader
{
	void DWIcharToNote( char c, GameController i, int &note1Out, int &note2Out );
	void DWIcharToNoteCol( char c, GameController i, int &col1Out, int &col2Out );

	bool LoadFromDWITokens( 
		RString sMode, RString sDescription, RString sNumFeet, RString sStepData1, 
		RString sStepData2,
		Steps &out );

	bool LoadFromDWIFile( const RString &sPath, Song &out );

	static float ParseBrokenDWITimestamp(const RString &arg1, const RString &arg2, const RString &arg3);
	static bool Is192( const RString &str, size_t pos );
	RString m_sLoadingFile;

public:
	void GetApplicableFiles( const RString &sPath, vector<RString> &out );
	bool Loadable( const RString &sPath );
	bool LoadFromDir( const RString &sPath, Song &out );
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
