/* BackgroundUtil - Shared background-related routines. */

#ifndef BackgroundUtil_H
#define BackgroundUtil_H

class Song;

const CString SBE_Centered			= "Centered";
const CString SBE_StretchNormal		= "StretchNormal";
const CString SBE_StretchNoLoop		= "StretchNoLoop";
const CString SBE_StretchRewind		= "StretchRewind";
const CString SBT_CrossFade			= "CrossFade";

const CString RANDOM_BACKGROUND_FILE = "-random-";
const CString SONG_BACKGROUND_FILE = "songbackground";

struct BackgroundDef
{
	BackgroundDef()
	{
	}
	bool operator<( const BackgroundDef &other ) const
	{
#define COMPARE(x) if( x < other.x ) return true; else if( x > other.x ) return false;
		COMPARE( m_sEffect );
		COMPARE( m_sFile1 );
		COMPARE( m_sFile2 );
#undef COMPARE
		return false;
	}
	bool operator==( const BackgroundDef &other ) const
	{
		return 
			m_sEffect == other.m_sEffect &&
			m_sFile1 == other.m_sFile1 &&
			m_sFile2 == other.m_sFile2;
	}
	bool IsEmpty() { return m_sFile1.empty() && m_sFile2.empty(); }
	CString	m_sEffect;	// "" == automatically choose
	CString m_sFile1;	// must not be ""
	CString m_sFile2;	// may be ""
};

struct BackgroundChange : public BackgroundDef
{
	BackgroundChange()
	{
		m_fStartBeat=-1;
		m_fRate=1;
	}
	BackgroundChange( 
		float s, 
		CString f1,
		CString f2=CString(),
		float r=1.f, 
		CString e=SBE_Centered,
		CString t=CString()
		)
	{
		m_fStartBeat=s;
		m_sFile1=f1;
		m_sFile2=f2;
		m_fRate=r;
		m_sEffect=e;
		m_sTransition=t;
	}
	float m_fStartBeat;
	float m_fRate;
	CString m_sTransition;
};


namespace BackgroundUtil
{
	void SortBackgroundChangesArray( vector<BackgroundChange> &vBackgroundChanges );
	
	void GetBackgroundEffects(		const CString &sName, vector<CString> &vsPathsOut, vector<CString> &vsNamesOut );
	void GetBackgroundTransitions(	const CString &sName, vector<CString> &vsPathsOut, vector<CString> &vsNamesOut );

	void GetSongBGAnimations(		const Song *pSong, const CString &sMatch, vector<CString> &vsPathsOut, vector<CString> &vsNamesOut );
	void GetSongMovies(				const Song *pSong, const CString &sMatch, vector<CString> &vsPathsOut, vector<CString> &vsNamesOut );
	void GetSongBitmaps(			const Song *pSong, const CString &sMatch, vector<CString> &vsPathsOut, vector<CString> &vsNamesOut );
	void GetGlobalBGAnimations(		const CString &sMatch, vector<CString> &vsPathsOut, vector<CString> &vsNamesOut );
	void GetGlobalRandomMovies(		const CString &sMatch, vector<CString> &vsPathsOut, vector<CString> &vsNamesOut );
};


#endif

/*
 * (c) 2001-2004 Chris Danford, Ben Nordstrom
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
