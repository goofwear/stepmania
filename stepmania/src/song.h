#pragma once
/*
-----------------------------------------------------------------------------
 Class: Song

 Desc: Holds data about a piece of music that can be played by one or more
	Games.

 Copyright (c) 2001-2002 by the persons listed below.  All rights reserved.
	Chris Danford
-----------------------------------------------------------------------------
*/


#include "NoteMetadata.h"

#include "GameConstantsAndTypes.h"
#include "RageUtil.h"
//enum DanceStyle;	// why is this needed?



struct BPMSegment 
{
	BPMSegment() { m_fStartBeat = m_fBPM = -1; };
	float m_fStartBeat;
	float m_fBPM;
};

struct FreezeSegment 
{
	FreezeSegment() { m_fStartBeat = m_fFreezeSeconds = -1; };
	float m_fStartBeat;
	float m_fFreezeSeconds;
};


class Song
{
public:
	Song();

	bool LoadFromSongDir( CString sDir );
	bool LoadFromDWIFile( CString sPath );
	bool LoadFromBMSDir( CString sDir );
	bool LoadFromSMDir( CString sDir );

	void Save();

private:

	void TidyUpData();	// call after loading to clean up invalid data

public:
	CString GetSongFilePath()		{return m_sSongFilePath; };
	CString GetSongFileDir()		{return m_sSongDir; };
	CString GetGroupName()			{return m_sGroupName; };
	CString GetMusicPath()			{return m_sMusicPath; };
	void GetMusicSampleRange( float &fStartSec, float &fEndSec ) { fStartSec = m_fMusicSampleStartSeconds; fEndSec = m_fMusicSampleStartSeconds + m_fMusicSampleLengthSeconds; };
	CString GetBannerPath()			{return m_sBannerPath; };
	CString GetBackgroundPath()		{return m_sBackgroundPath; };
	CString GetBackgroundMoviePath(){return m_sBackgroundMoviePath; };
	CString GetCDTitlePath()		{return m_sCDTitlePath; };


	bool HasMusic()				{return m_sMusicPath != ""			&&	DoesFileExist(GetMusicPath()); };
	bool HasBanner()			{return m_sBannerPath != ""			&&  DoesFileExist(GetBannerPath()); };
	bool HasBackground()		{return m_sBackgroundPath != ""		&&  DoesFileExist(GetBackgroundPath()); };
	bool HasBackgroundMovie()	{return m_sBackgroundMoviePath != ""&&  DoesFileExist(GetBackgroundMoviePath()); };
	bool HasCDTitle()			{return m_sCDTitlePath != ""		&&  DoesFileExist(GetCDTitlePath()); };


	CString GetMainTitle()		{return m_sMainTitle; };
	CString GetSubTitle()		{return m_sSubTitle; };
	CString GetFullTitle()		{return m_sMainTitle + " " + m_sSubTitle; };
	void GetMainAndSubTitlesFromFullTitle( CString sFullTitle, CString &sMainTitleOut, CString &sSubTitleOut );

	CString GetArtist()				{return m_sArtist; };
	CString GetCredit()				{return m_sCredit; };
	float GetBeatOffsetInSeconds()	{return m_fOffsetInSeconds; };
	void SetBeatOffsetInSeconds(float fNewOffset)	{m_fOffsetInSeconds = fNewOffset; };
	void GetMinMaxBPM( float &fMinBPM, float &fMaxBPM )
	{
		fMaxBPM = 0;
		fMinBPM = 100000;	// inf
		for( int i=0; i<m_BPMSegments.GetSize(); i++ ) 
		{
			BPMSegment &seg = m_BPMSegments[i];
			fMaxBPM = max( seg.m_fBPM, fMaxBPM );
			fMinBPM = min( seg.m_fBPM, fMinBPM );
		}
	};
	void GetBeatAndBPSFromElapsedTime( float fElapsedTime, float &fBeatOut, float &fBPSOut );
	float GetElapsedTimeFromBeat( float fBeat );
	void GetNoteMetadatasThatMatchGameAndStyle( CString sGame, CString sStyle, CArray<NoteMetadata*, NoteMetadata*>& arrayAddTo );

	int GetNumTimesPlayed()
	{
		int iTotalNumTimesPlayed = 0;
		for( int i=0; i<m_arrayNoteMetadatas.GetSize(); i++ )
		{
			iTotalNumTimesPlayed += m_arrayNoteMetadatas[i].m_iNumTimesPlayed;
		}
		return iTotalNumTimesPlayed;
	}

	bool HasChangedSinceLastSave()	{ return m_bChangedSinceSave;	}
	void SetChangedSinceLastSave()	{ m_bChangedSinceSave = true;	}

	Grade GetGradeForDifficultyClass( CString sGame, CString sStyle, DifficultyClass dc );


private:
	CString m_sSongFilePath;
	CString m_sSongDir;
	CString m_sGroupName;

	bool	m_bChangedSinceSave;
	CString	m_sMainTitle;
	CString	m_sSubTitle;
	CString	m_sArtist;
	CString	m_sCredit;
	float	m_fOffsetInSeconds;

	CString	m_sMusicPath;
	DWORD	m_dwMusicBytes;
	float	m_fMusicSampleStartSeconds, m_fMusicSampleLengthSeconds;
	CString	m_sBannerPath;
	CString	m_sBackgroundPath;
	CString	m_sBackgroundMoviePath;
	CString	m_sCDTitlePath;

	CArray<BPMSegment, BPMSegment&> m_BPMSegments;	// this must be sorted before dancing
	CArray<FreezeSegment, FreezeSegment&> m_FreezeSegments;	// this must be sorted before dancing

public:
	CArray<NoteMetadata, NoteMetadata&> m_arrayNoteMetadatas;
};


void SortSongPointerArrayByTitle( CArray<Song*, Song*> &arraySongPointers );
void SortSongPointerArrayByBPM( CArray<Song*, Song*> &arraySongPointers );
void SortSongPointerArrayByArtist( CArray<Song*, Song*> &arraySongPointers );
void SortSongPointerArrayByGroup( CArray<Song*, Song*> &arraySongPointers );
void SortSongPointerArrayByMostPlayed( CArray<Song*, Song*> &arraySongPointers );


