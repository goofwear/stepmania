#pragma once
/*
-----------------------------------------------------------------------------
 File: NoteData.h

 Desc: Holds data about the notes that the player is supposed to hit.  NoteData
	is organized by:
	track - corresponds to different columns of notes on the screen
	index - corresponds to subdivisions of beats

 Copyright (c) 2001-2002 by the persons listed below.  All rights reserved.
	Chris Danford
-----------------------------------------------------------------------------
*/

#include "GameTypes.h"

typedef unsigned char TapNote;
typedef unsigned char TrackNumber;
typedef unsigned short NoteIndex;
// '1' = tap note
// '2' = hold note begin
// '3' = hold note end  ('1' can also end a HoldNote)
// ... for future expansion


enum TapNoteScore { 
	TNS_NONE, 
	TNS_PERFECT, 
	TNS_GREAT,
	TNS_GOOD,
	TNS_BOO,
	TNS_MISS
};

//enum TapNoteTiming { 
//	TNT_NONE, 
//	TNT_EARLY, 
//	TNT_LATE 
//};


struct HoldNote
{
	TrackNumber m_iTrack;	
	NoteIndex m_iStartIndex;
	NoteIndex m_iEndIndex;
};

enum HoldNoteResult 
{ 
	HNR_NONE,	// this HoldNote has not been scored yet
	HNR_OK,		// the HoldNote has passed and was successfully held all the way through
	HNR_NG		// the HoldNote has passed and they missed it
};

struct HoldNoteScore
{
	TapNoteScore	m_TapNoteScore;		// The scoring of the tap Note that begins the hold.  
										// This is judeged separately from the actual hold.
	HoldNoteResult	m_Result;
	float			m_fLife;	// 1.0 means this HoldNote has full life.
								// 0.0 means this HoldNote is dead
								// When this value hits 0.0 for the first time, 
								// m_HoldScore becomes HSS_NG.
								// If the life is > 0.0 when the HoldNote ends, then
								// m_HoldScore becomes HSS_OK.
	HoldNoteScore() 
	{ 
		m_TapNoteScore = TNS_NONE; 
		m_Result = HNR_NONE; 
		m_fLife = 1.0f; 
	};
};


const int MAX_NOTE_TRACKS	=	16;


const int MAX_MEASURES		= 200;	// this should be long enough to hold 10:00 minute songs (
const int BEATS_PER_MEASURE = 4;
const int MAX_BEATS			= MAX_MEASURES * BEATS_PER_MEASURE;

const int ELEMENTS_PER_BEAT	= 12;	// It is important that this number is evenly divisible by 2, 3, 4, and 8
const int ELEMENTS_PER_MEASURE = ELEMENTS_PER_BEAT * BEATS_PER_MEASURE;
const int MAX_TAP_NOTE_ELEMENTS = MAX_BEATS*ELEMENTS_PER_BEAT;

const int MAX_HOLD_NOTE_ELEMENTS = 200;

inline int   BeatToNoteIndex( float fBeatNum )			{ return int( fBeatNum * ELEMENTS_PER_BEAT + 0.5f); };	// round
inline int   BeatToNoteIndexNotRounded( float fBeatNum ){ return (int)( fBeatNum * ELEMENTS_PER_BEAT ); };
inline float NoteIndexToBeat( float fNoteIndex )		{ return fNoteIndex / (float)ELEMENTS_PER_BEAT;	 };
inline float NoteIndexToBeat( int iNoteIndex )			{ return NoteIndexToBeat( (float)iNoteIndex );	 };


enum NoteType 
{ 
	NOTE_4TH,	// quarter notes
	NOTE_8TH,	// eighth notes
	NOTE_12TH,	// triplets
	NOTE_16TH,	// sixteenth notes
	NUM_NOTE_TYPES
};

inline bool IsNoteOfType( int iNoteIndex, NoteType t )
{ 
	switch( t )
	{
	case NOTE_4TH:	return iNoteIndex % (ELEMENTS_PER_MEASURE/4) == 0;
	case NOTE_8TH:	return iNoteIndex % (ELEMENTS_PER_MEASURE/8) == 0;
	case NOTE_12TH:	return iNoteIndex % (ELEMENTS_PER_MEASURE/12) == 0;
	case NOTE_16TH:	return iNoteIndex % (ELEMENTS_PER_MEASURE/16) == 0;
	default:		ASSERT( false );	return false;
	}
};



class NoteData
{
public:
	NoteData();

	int				m_iNumTracks;
	TapNote			m_TapNotes[MAX_NOTE_TRACKS][MAX_TAP_NOTE_ELEMENTS];
	HoldNote		m_HoldNotes[MAX_HOLD_NOTE_ELEMENTS];
	int				m_iNumHoldNotes;

	void ClearRange( NoteIndex iNoteIndexBegin, NoteIndex iNoteIndexEnd );
	void ClearAll() { ClearRange( 0, MAX_TAP_NOTE_ELEMENTS ); };
	void CopyRange( NoteData* pFrom, NoteIndex iNoteIndexBegin, NoteIndex iNoteIndexEnd );
	void CopyAll( NoteData* pFrom ) { m_iNumTracks = pFrom->m_iNumTracks; CopyRange( pFrom, 0, MAX_TAP_NOTE_ELEMENTS ); };

	inline bool IsRowEmpty( NoteIndex index )
	{
		for( int t=0; t<m_iNumTracks; t++ )
		{
			if( m_TapNotes[t][index] != '0' )
				return false;
		}
		return true;
	}

	// used in edit/record
	void AddHoldNote( HoldNote newNote );	// add note hold note merging overlapping HoldNotes and destroying TapNotes underneath
	void RemoveHoldNote( int index );

	// used for saving
	void GetMeasureStrings( CStringArray &arrayMeasureStrings );		// for saving to a .notes file

	// used for loading
	void SetFromMeasureStrings( CStringArray &arrayMeasureStrings );	// for loading from a .notes file
	void SetFromDWIInfo();	// for loading from a .dwi file
	void SetFromBMSInfo();	// for loading from a .bms file

	// statistics
	float GetLastBeat();	// return the beat number of the last beat in the song
	int GetNumTapNotes();
	int GetNumHoldNotes();

	// Transformations
	void LoadTransformed( NoteData* pOriginal, int iNewNumTracks, TrackNumber iNewToOriginalTrack[] );

	void CropToLeftSide();
	void CropToRightSide();
	void RemoveHoldNotes();
	void Turn( PlayerOptions::TurnType tt );
	void MakeLittle();

	void SnapToNearestNoteType( NoteType nt1, NoteType nt2, float fBeginBeat, float fEndBeat );

	// Convert between HoldNote representation and '2' and '3' markers in TapNotes
	void Convert2sAnd3sToHoldNotes();
	void ConvertHoldNotesTo2sAnd3s();

};
