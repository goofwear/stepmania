#ifndef NOTES_LOADER_BMS_H
#define NOTES_LOADER_BMS_H

#include "NotesLoader.h"
#include <map>

class Song;
class Steps;

class BMSLoader: public NotesLoader
{
	void SlideDuplicateDifficulties( Song &p );

	map<int, float> m_MeasureToTimeSig;
	float GetBeatsPerMeasure( int iMeasure ) const;
	int GetMeasureStartRow( int iMeasureNo ) const;

	typedef multimap<CString, CString> NameToData_t;
	bool ReadBMSFile( const CString &sPath, BMSLoader::NameToData_t &mapNameToData );
	bool LoadFromBMSFile( const CString &sPath, const NameToData_t &mapNameToData, Steps &out1 );
	void ReadGlobalTags( const NameToData_t &mapNameToData, Song &out );
	bool GetTagFromMap( const BMSLoader::NameToData_t &mapNameToData, const CString &sName, CString &sOut );

	CString m_sDir;
	map<CString,int> m_mapWavIdToKeysoundIndex;

public:
	void GetApplicableFiles( CString sPath, CStringArray &out );
	bool LoadFromDir( CString sDir, Song &out );
};

#endif

/*
 * (c) 2001-2003 Chris Danford
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
