/* HighScore - Player scoring data that persists between sessions. */

#ifndef HighScore_H
#define HighScore_H

#include "Grade.h"
#include "GameConstantsAndTypes.h"
#include "RadarValues.h"
#include "DateTime.h"
#include "RageUtil_AutoPtr.h"

class XNode;

struct HighScoreImpl;
struct HighScore
{
	HighScore();

	RString	GetName() const;
	Grade GetGrade() const;
	int GetScore() const;
	float GetPercentDP() const;
	float GetSurviveSeconds() const;
	float GetSurvivalSeconds() const;
	RString GetModifiers() const;
	DateTime GetDateTime() const;
	RString GetPlayerGuid() const;
	RString GetMachineGuid() const;
	int GetProductID() const;
	int GetTapNoteScore( TapNoteScore tns ) const;
	int GetHoldNoteScore( HoldNoteScore tns ) const;
	const RadarValues &GetRadarValues() const;
	float GetLifeRemainingSeconds() const;

	void SetName( const RString &sName );
	void SetGrade( Grade g );
	void SetScore( int iScore );
	void SetPercentDP( float f );
	void SetSurviveSeconds( float f );
	void SetModifiers( RString s );
	void SetDateTime( DateTime d );
	void SetPlayerGuid( RString s );
	void SetMachineGuid( RString s );
	void SetProductID( int i );
	void SetTapNoteScore( TapNoteScore tns, int i );
	void SetHoldNoteScore( HoldNoteScore tns, int i );
	void SetRadarValues( const RadarValues &rv );
	void SetLifeRemainingSeconds( float f );

	RString *GetNameMutable();
	const RString *GetNameMutable() const { return const_cast<RString *> (const_cast<HighScore *>(this)->GetNameMutable()); }

	void Unset();

	bool operator>=( const HighScore& other ) const;
	bool operator==( const HighScore& other ) const;

	XNode* CreateNode() const;
	void LoadFromNode( const XNode* pNode );

	RString GetDisplayName() const;
private:
	HiddenPtr<HighScoreImpl> m_Impl;
};

struct HighScoreList
{
public:
	HighScoreList()
	{
		iNumTimesPlayed = 0;
	}
	void Init();
	
	int GetNumTimesPlayed() const
	{
		return iNumTimesPlayed;
	}
	DateTime GetLastPlayed() const
	{
		ASSERT( iNumTimesPlayed > 0 );	// don't call this unless the song has been played
		return dtLastPlayed;
	}
	const HighScore& GetTopScore() const;

	void AddHighScore( HighScore hs, int &iIndexOut, bool bIsMachine );
	void IncrementPlayCount( DateTime dtLastPlayed );
	void RemoveAllButOneOfEachName();
	void ClampSize( bool bIsMachine );

	XNode* CreateNode() const;
	void LoadFromNode( const XNode* pNode );

	vector<HighScore> vHighScores;
private:
	int iNumTimesPlayed;
	DateTime dtLastPlayed;	// meaningless if iNumTimesPlayed == 0
	
};

struct Screenshot
{
	RString sFileName;	// no directory part - just the file name
	RString sMD5;		// MD5 hash of the screenshot file
	HighScore highScore;

	XNode* CreateNode() const;
	void LoadFromNode( const XNode* pNode );
};

#endif

/*
 * (c) 2004 Chris Danford
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
