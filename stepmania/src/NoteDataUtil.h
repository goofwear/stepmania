/* NoteDataUtil - Utility functions that deal with NoteData. */

#ifndef NOTEDATAUTIL_H
#define NOTEDATAUTIL_H

#include "GameConstantsAndTypes.h"	// for RadarCategory
#include "NoteTypes.h"
#include "NoteData.h"

struct PlayerOptions;
struct RadarValues;

/* Utils for NoteData.  Things should go in here if they can be (cleanly and
 * efficiently) implemented using only NoteData's primitives; this improves
 * abstraction and makes it much easier to change NoteData internally in
 * the future. */
namespace NoteDataUtil
{
	NoteType GetSmallestNoteTypeForMeasure( const NoteData &n, int iMeasureIndex );
	void LoadFromSMNoteDataString( NoteData &out, CString sSMNoteData );
	void GetSMNoteDataString( const NoteData &in, CString &notes_out );
	void LoadTransformedSlidingWindow( const NoteData &in, NoteData &out, int iNewNumTracks );
	void LoadOverlapped( const NoteData &in, NoteData &out, int iNewNumTracks );
	void LoadTransformedLights( const NoteData &in, NoteData &out, int iNewNumTracks );

	// radar values - return between 0.0 and 1.2
	float GetStreamRadarValue( const NoteData &in, float fSongSeconds );
	float GetVoltageRadarValue( const NoteData &in, float fSongSeconds );
	float GetAirRadarValue( const NoteData &in, float fSongSeconds );
	float GetFreezeRadarValue( const NoteData &in, float fSongSeconds );
	float GetChaosRadarValue( const NoteData &in, float fSongSeconds );

	void GetRadarValues( const NoteData &in, float fSongSeconds, RadarValues& out );

	void RemoveHoldNotes( NoteData &inout, float fStartBeat = 0, float fEndBeat = 99999 );
	void RemoveSimultaneousNotes( NoteData &inout, int iMaxSimultaneous, float fStartBeat = 0, float fEndBeat = 99999 );
	void RemoveJumps( NoteData &inout, float fStartBeat = 0, float fEndBeat = 99999 );
	void RemoveHands( NoteData &inout, float fStartBeat = 0, float fEndBeat = 99999 );
	void RemoveQuads( NoteData &inout, float fStartBeat = 0, float fEndBeat = 99999 );
	void RemoveMines( NoteData &inout, float fStartBeat = 0, float fEndBeat = 99999 );
	void RemoveAllButOneTap( NoteData &inout, int row );
	enum TrackMapping { left, right, mirror, shuffle, super_shuffle, stomp, NUM_TRACK_MAPPINGS };
	void Turn( NoteData &inout, StepsType st, TrackMapping tt, float fStartBeat = 0, float fEndBeat = -1 );
	void Little( NoteData &inout, float fStartBeat = 0, float fEndBeat = 99999 );
	void Wide( NoteData &inout, float fStartBeat = 0, float fEndBeat = 99999 );
	void Big( NoteData &inout, float fStartBeat = 0, float fEndBeat = 99999 );
	void Quick( NoteData &inout, float fStartBeat = 0, float fEndBeat = 99999 );
	void BMRize( NoteData &inout, float fStartBeat = 0, float fEndBeat = 99999 );
	void Skippy( NoteData &inout, float fStartBeat = 0, float fEndBeat = 99999 );
	void InsertIntelligentTaps( 
		NoteData &in, 
		float fWindowSizeBeats, 
		float fInsertOffsetBeats, 
		float fWindowStrideBeats, 
		bool bSkippy, 
		float fStartBeat = 0, 
		float fEndBeat = 99999 );
	void AddMines( NoteData &inout, float fStartBeat = 0, float fEndBeat = 99999 );
	void Echo( NoteData &inout, float fStartBeat = 0, float fEndBeat = 99999 );
	void Stomp( NoteData &inout, StepsType st, float fStartBeat = 0, float fEndBeat = 99999 );
	void Planted( NoteData &inout, float fStartBeat = 0, float fEndBeat = 99999 );
	void Floored( NoteData &inout, float fStartBeat = 0, float fEndBeat = 99999 );
	void Twister( NoteData &inout, float fStartBeat = 0, float fEndBeat = 99999 );
	void ConvertTapsToHolds( NoteData &inout, int iSimultaneousHolds, float fStartBeat = 0, float fEndBeat = 99999 );

	// change all TAP_ADDITIONs to TAP_TAPs
	void ConvertAdditionsToRegular( NoteData &inout );

	void Backwards( NoteData &inout );
	void SwapSides( NoteData &inout );
	void CopyLeftToRight( NoteData &inout );
	void CopyRightToLeft( NoteData &inout );
	void ClearLeft( NoteData &inout );
	void ClearRight( NoteData &inout );
	void CollapseToOne( NoteData &inout );
	void CollapseLeft( NoteData &inout );
	void ShiftLeft( NoteData &inout );
	void ShiftRight( NoteData &inout );

	void SnapToNearestNoteType( NoteData &inout, NoteType nt1, NoteType nt2, float fBeginBeat, float fEndBeat );

	inline void SnapToNearestNoteType( NoteData &inout, NoteType nt, float fBeginBeat, float fEndBeat )
	{
		SnapToNearestNoteType( inout, nt, NOTE_TYPE_INVALID, fBeginBeat, fEndBeat );
	}

	void FixImpossibleRows( NoteData &inout, StepsType st );

	// True if no notes in row that aren't true in the mask
	bool RowPassesValidMask( NoteData &inout, int row, const bool bValidMask[] );

	void TransformNoteData( NoteData &nd, const AttackArray &aa, StepsType st, Song* pSong );
	void TransformNoteData( NoteData &nd, const PlayerOptions &po, StepsType st, float fStartBeat = 0, float fEndBeat = 99999 );
	void AddTapAttacks( NoteData &nd, Song* pSong );

	// void Scale( NoteData &nd, float fScale );
	void ScaleRegion( NoteData &nd, float fScale, float fStartBeat = 0, float fEndBeat = 99999);
	inline void Scale( NoteData &nd, float fScale ) { NoteDataUtil::ScaleRegion(nd, fScale); }

	// If fBeatsToShift>0, add blank rows.  If fBeatsToShift<0, delete rows
	void ShiftRows( NoteData &nd, float fStartBeat, float fBeatsToShift );

	void RemoveAllTapsOfType( NoteData& ndInOut, TapNote::Type typeToRemove );
	void RemoveAllTapsExceptForType( NoteData& ndInOut, TapNote::Type typeToKeep );
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
