#include "global.h"
/*
-----------------------------------------------------------------------------
 Class: NoteDataUtil

 Desc: See header.

 Copyright (c) 2001-2002 by the person(s) listed below.  All rights reserved.
	Chris Danford
-----------------------------------------------------------------------------
*/

#include "NoteDataUtil.h"
#include "RageUtil.h"
#include "RageLog.h"
#include "PlayerOptions.h"

NoteType NoteDataUtil::GetSmallestNoteTypeForMeasure( const NoteData &n, int iMeasureIndex )
{
	const int iMeasureStartIndex = iMeasureIndex * ROWS_PER_MEASURE;
	const int iMeasureLastIndex = (iMeasureIndex+1) * ROWS_PER_MEASURE - 1;

	// probe to find the smallest note type
	NoteType nt;
	for( nt=(NoteType)0; nt<NUM_NOTE_TYPES; nt=NoteType(nt+1) )		// for each NoteType, largest to largest
	{
		float fBeatSpacing = NoteTypeToBeat( nt );
		int iRowSpacing = int(roundf( fBeatSpacing * ROWS_PER_BEAT ));

		bool bFoundSmallerNote = false;
		for( int i=iMeasureStartIndex; i<=iMeasureLastIndex; i++ )	// for each index in this measure
		{
			if( i % iRowSpacing == 0 )
				continue;	// skip
			
			if( !n.IsRowEmpty(i) )
			{
				bFoundSmallerNote = true;
				break;
			}
		}

		if( bFoundSmallerNote )
			continue;	// searching the next NoteType
		else
			break;	// stop searching.  We found the smallest NoteType
	}

	if( nt == NUM_NOTE_TYPES )	// we didn't find one
		return NOTE_TYPE_INVALID;	// well-formed notes created in the editor should never get here
	else
		return nt;
}

void NoteDataUtil::LoadFromSMNoteDataString( NoteData &out, CString sSMNoteData )
{
	/* Clear notes, but keep the same number of tracks. */
	int iNumTracks = out.GetNumTracks();
	out.Init();
	out.SetNumTracks( iNumTracks );

	// strip comments out of sSMNoteData
	while( sSMNoteData.Find("//") != -1 )
	{
		int iIndexCommentStart = sSMNoteData.Find("//");
		int iIndexCommentEnd = sSMNoteData.Find("\n", iIndexCommentStart);
		if( iIndexCommentEnd == -1 )	// comment doesn't have an end?
			sSMNoteData.erase( iIndexCommentStart, 2 );
		else
			sSMNoteData.erase( iIndexCommentStart, iIndexCommentEnd-iIndexCommentStart );
	}

	CStringArray asMeasures;
	split( sSMNoteData, ",", asMeasures, true );	// ignore empty is important
	for( unsigned m=0; m<asMeasures.size(); m++ )	// foreach measure
	{
		CString &sMeasureString = asMeasures[m];
		TrimLeft(sMeasureString);
		TrimRight(sMeasureString);

		CStringArray asMeasureLines;
		split( sMeasureString, "\n", asMeasureLines, true );	// ignore empty is important

		//ASSERT( asMeasureLines.size() == 4  ||
		//	    asMeasureLines.size() == 8  ||
		//	    asMeasureLines.size() == 12  ||
		//	    asMeasureLines.size() == 16 );


		for( unsigned l=0; l<asMeasureLines.size(); l++ )
		{
			CString &sMeasureLine = asMeasureLines[l];
			TrimLeft(sMeasureLine);
			TrimRight(sMeasureLine);

			const float fPercentIntoMeasure = l/(float)asMeasureLines.size();
			const float fBeat = (m + fPercentIntoMeasure) * BEATS_PER_MEASURE;
			const int iIndex = BeatToNoteRow( fBeat );

//			if( m_iNumTracks != sMeasureLine.GetLength() )
//				RageException::Throw( "Actual number of note columns (%d) is different from the StepsType (%d).", m_iNumTracks, sMeasureLine.GetLength() );

			for( int c=0; c<min(sMeasureLine.GetLength(),out.GetNumTracks()); c++ )
			{
				TapNote t;
				switch(sMeasureLine[c])
				{
				case '0': t = TAP_EMPTY; break;
				case '1': t = TAP_TAP; break;
				case '2': t = TAP_HOLD_HEAD; break;
				case '3': t = TAP_HOLD_TAIL; break;
				case 'm':
				case 'M': t = TAP_MINE; break;
				
				default: 
					/* Invalid data.  We don't want to assert, since there might
					 * simply be invalid data in an .SM, and we don't want to die
					 * due to invalid data.  We should probably check for this when
					 * we load SM data for the first time ... */
					// ASSERT(0); 
					t = TAP_EMPTY; break;
				}
				out.SetTapNote(c, iIndex, t);
			}
		}
	}
	out.Convert2sAnd3sToHoldNotes();
}

CString NoteDataUtil::GetSMNoteDataString(NoteData &in)
{
	in.ConvertHoldNotesTo2sAnd3s();

	float fLastBeat = in.GetLastBeat();
	int iLastMeasure = int( fLastBeat/BEATS_PER_MEASURE );

	CString sRet = "\n"; /* data begins on a new line when written to disk */

	for( int m=0; m<=iLastMeasure; m++ )	// foreach measure
	{
		NoteType nt = GetSmallestNoteTypeForMeasure( in, m );
		int iRowSpacing;
		if( nt == NOTE_TYPE_INVALID )
			iRowSpacing = 1;
		else
			iRowSpacing = int(roundf( NoteTypeToBeat(nt) * ROWS_PER_BEAT ));

		sRet += ssprintf("  // measure %d\n", m+1);

		const int iMeasureStartRow = m * ROWS_PER_MEASURE;
		const int iMeasureLastRow = (m+1) * ROWS_PER_MEASURE - 1;

		for( int r=iMeasureStartRow; r<=iMeasureLastRow; r+=iRowSpacing )
		{
			for( int t=0; t<in.GetNumTracks(); t++ ) {
				char c;
				switch(in.GetTapNote(t, r)) {
				case TAP_EMPTY: c = '0'; break;
				case TAP_TAP:   c = '1'; break;
				case TAP_HOLD_HEAD: c = '2'; break;
				case TAP_HOLD_TAIL: c = '3'; break;
				case TAP_MINE:	c = 'M'; break;
				default: ASSERT(0); c = '0'; break;
				}
				sRet.append(1, c);
			}
			
			sRet.append(1, '\n');
		}

		sRet.append(1, ',');
	}

	in.Convert2sAnd3sToHoldNotes();

	return sRet;
}

float NoteDataUtil::GetRadarValue( const NoteData &in, RadarCategory rv, float fSongSeconds )
{
	switch( rv )
	{
	case RADAR_STREAM:	return GetStreamRadarValue( in, fSongSeconds );
	case RADAR_VOLTAGE:	return GetVoltageRadarValue( in, fSongSeconds );
	case RADAR_AIR:		return GetAirRadarValue( in, fSongSeconds );
	case RADAR_FREEZE:	return GetFreezeRadarValue( in, fSongSeconds );
	case RADAR_CHAOS:	return GetChaosRadarValue( in, fSongSeconds );
	default:	ASSERT(0);  return 0;
	}
}

float NoteDataUtil::GetStreamRadarValue( const NoteData &in, float fSongSeconds )
{
	if( !fSongSeconds )
		return 0.0f;
	// density of steps
	int iNumNotes = in.GetNumTapNotes() + in.GetNumHoldNotes();
	float fNotesPerSecond = iNumNotes/fSongSeconds;
	float fReturn = fNotesPerSecond / 7;
	return min( fReturn, 1.0f );
}

float NoteDataUtil::GetVoltageRadarValue( const NoteData &in, float fSongSeconds )
{
	if( !fSongSeconds )
		return 0.0f;

	const float fLastBeat = in.GetLastBeat();
	const float fAvgBPS = fLastBeat / fSongSeconds;

	// peak density of steps
	float fMaxDensitySoFar = 0;

	const int BEAT_WINDOW = 8;

	for( int i=0; i<=int(fLastBeat); i+=BEAT_WINDOW )
	{
		int iNumNotesThisWindow = in.GetNumTapNotes((float)i,(float)i+BEAT_WINDOW) + in.GetNumHoldNotes((float)i,(float)i+BEAT_WINDOW);
		float fDensityThisWindow = iNumNotesThisWindow/(float)BEAT_WINDOW;
		fMaxDensitySoFar = max( fMaxDensitySoFar, fDensityThisWindow );
	}

	float fReturn = fMaxDensitySoFar*fAvgBPS/10;
	return min( fReturn, 1.0f );
}

float NoteDataUtil::GetAirRadarValue( const NoteData &in, float fSongSeconds )
{
	if( !fSongSeconds )
		return 0.0f;
	// number of doubles
	int iNumDoubles = in.GetNumDoubles();
	float fReturn = iNumDoubles / fSongSeconds;
	return min( fReturn, 1.0f );
}

float NoteDataUtil::GetFreezeRadarValue( const NoteData &in, float fSongSeconds )
{
	if( !fSongSeconds )
		return 0.0f;
	// number of hold steps
	float fReturn = in.GetNumHoldNotes() / fSongSeconds;
	return min( fReturn, 1.0f );
}

float NoteDataUtil::GetChaosRadarValue( const NoteData &in, float fSongSeconds )
{
	if( !fSongSeconds )
		return 0.0f;
	// count number of triplets or 16ths
	int iNumChaosNotes = 0;
	int rows = in.GetLastRow();
	for( int r=0; r<=rows; r++ )
	{
		if( !in.IsRowEmpty(r)  &&  GetNoteType(r) >= NOTE_TYPE_12TH )
			iNumChaosNotes++;
	}

	float fReturn = iNumChaosNotes / fSongSeconds * 0.5f;
	return min( fReturn, 1.0f );
}

void NoteDataUtil::RemoveHoldNotes(NoteData &in)
{
	vector<int> tracks, rows;

	// turn all the HoldNotes into TapNotes
	for( int i=0; i<in.GetNumHoldNotes(); i++ )
	{
		const HoldNote &hn = in.GetHoldNote(i);
		
		tracks.push_back(hn.iTrack);
		rows.push_back(BeatToNoteRow(hn.fStartBeat));
	}

	// Remove all HoldNotes
	while(in.GetNumHoldNotes())
		in.RemoveHoldNote(in.GetNumHoldNotes()-1);

	for(unsigned j = 0; j < tracks.size(); ++j)
		in.SetTapNote(tracks[j], rows[j], TAP_TAP);
}


void NoteDataUtil::Turn( NoteData &in, StepsType st, TurnType tt )
{
	int iTakeFromTrack[MAX_NOTE_TRACKS];	// New track "t" will take from old track iTakeFromTrack[t]

	int t;

	switch( tt )
	{
	case left:
	case right:
		// Is there a way to do this withoutn handling each StepsType? -Chris
		// Identity transform for ones not handled below.  What should we do here?
		for( t = 0; t < MAX_NOTE_TRACKS; ++t )
			iTakeFromTrack[t] = t;
		switch( st )
		{
		case STEPS_TYPE_DANCE_SINGLE:
		case STEPS_TYPE_DANCE_DOUBLE:
		case STEPS_TYPE_DANCE_COUPLE:
			iTakeFromTrack[0] = 2;
			iTakeFromTrack[1] = 0;
			iTakeFromTrack[2] = 3;
			iTakeFromTrack[3] = 1;
			iTakeFromTrack[4] = 6;
			iTakeFromTrack[5] = 4;
			iTakeFromTrack[6] = 7;
			iTakeFromTrack[7] = 5;
			break;
		case STEPS_TYPE_DANCE_SOLO:
			iTakeFromTrack[0] = 5;
			iTakeFromTrack[1] = 4;
			iTakeFromTrack[2] = 0;
			iTakeFromTrack[3] = 3;
			iTakeFromTrack[4] = 1;
			iTakeFromTrack[5] = 2;
			break;
		case STEPS_TYPE_PUMP_SINGLE:
		case STEPS_TYPE_PUMP_COUPLE:
			iTakeFromTrack[0] = 3;
			iTakeFromTrack[1] = 4;
			iTakeFromTrack[2] = 2;
			iTakeFromTrack[3] = 0;
			iTakeFromTrack[4] = 1;
			iTakeFromTrack[5] = 8;
			iTakeFromTrack[6] = 9;
			iTakeFromTrack[7] = 7;
			iTakeFromTrack[8] = 5;
			iTakeFromTrack[9] = 6;
			break;
		case STEPS_TYPE_PUMP_HALFDOUBLE:
			iTakeFromTrack[0] = 2;
			iTakeFromTrack[1] = 0;
			iTakeFromTrack[2] = 1;
			iTakeFromTrack[3] = 3;
			iTakeFromTrack[4] = 4;
			iTakeFromTrack[5] = 5;
			break;
		case STEPS_TYPE_PUMP_DOUBLE:
			iTakeFromTrack[0] = 8;
			iTakeFromTrack[1] = 9;
			iTakeFromTrack[2] = 7;
			iTakeFromTrack[3] = 5;
			iTakeFromTrack[4] = 6;
			iTakeFromTrack[5] = 3;
			iTakeFromTrack[6] = 4;
			iTakeFromTrack[7] = 2;
			iTakeFromTrack[8] = 0;
			iTakeFromTrack[9] = 1;
			break;
		default: break;
		}

		if( tt == right )
		{
			/* Invert. */
			int iTrack[MAX_NOTE_TRACKS];
			memcpy( iTrack, iTakeFromTrack, sizeof(iTrack) );
			for( t = 0; t < MAX_NOTE_TRACKS; ++t )
			{
				const int to = iTrack[t];
				iTakeFromTrack[to] = t;
			}
		}

		break;
	case mirror:
		for( t=0; t<in.GetNumTracks(); t++ )
			iTakeFromTrack[t] = in.GetNumTracks()-t-1;
		break;
	case shuffle:
	case super_shuffle:		// use shuffle code to mix up HoldNotes without creating impossible patterns
		{
			vector<int> aiTracksLeftToMap;
			for( t=0; t<in.GetNumTracks(); t++ )
				aiTracksLeftToMap.push_back( t );
			
			for( t=0; t<in.GetNumTracks(); t++ )
			{
				int iRandTrackIndex = rand()%aiTracksLeftToMap.size();
				int iRandTrack = aiTracksLeftToMap[iRandTrackIndex];
				aiTracksLeftToMap.erase( aiTracksLeftToMap.begin()+iRandTrackIndex,
										 aiTracksLeftToMap.begin()+iRandTrackIndex+1 );
				iTakeFromTrack[t] = iRandTrack;
			}
		}
		break;
	default:
		ASSERT(0);
	}

	NoteData tempNoteData;	// write into here as we tranform
	tempNoteData.Config(in);

	in.ConvertHoldNotesTo2sAnd3s();

	// transform notes
	int max_row = in.GetLastRow();
	for( t=0; t<in.GetNumTracks(); t++ )
		for( int r=0; r<=max_row; r++ ) 			
			tempNoteData.SetTapNote(t, r, in.GetTapNote(iTakeFromTrack[t], r));

	in.CopyAll( &tempNoteData );		// copy note data from newData back into this
	in.Convert2sAnd3sToHoldNotes();

	if( tt == super_shuffle )
		SuperShuffleTaps( in );
}

void NoteDataUtil::SuperShuffleTaps( NoteData &in )
{
	// We already did the normal shuffling code above, which did a good job
	// of shuffling HoldNotes without creating impossible patterns.
	// Now, go in and shuffle the TapNotes per-row.
	in.ConvertHoldNotesTo4s();

	int max_row = in.GetLastRow();
	for( int r=0; r<=max_row; r++ )
	{
		for( int t1=0; t1<in.GetNumTracks(); t1++ )
		{
			TapNote tn1 = in.GetTapNote(t1, r);
			if( tn1!=TAP_HOLD )	// a tap that is not part of a hold
			{
				// probe for a spot to swap with
				while( 1 )
				{
					int t2 = rand() % in.GetNumTracks();
					TapNote tn2 = in.GetTapNote(t2, r);
					if( tn2!=TAP_HOLD )	// a tap that is not part of a hold
					{
						// swap
						in.SetTapNote(t1, r, tn2);
						in.SetTapNote(t2, r, tn1);
						break;
					}
				}
			}
		}
	}
	in.Convert4sToHoldNotes();
}

void NoteDataUtil::Backwards( NoteData &in )
{
	in.ConvertHoldNotesTo4s();
	int max_row = in.GetLastRow();
	for( int r=0; r<=max_row/2; r++ )
	{
		int iRowEarlier = r;
		int iRowLater = max_row-r;

		for( int t=0; t<in.GetNumTracks(); t++ )
		{
			TapNote tnEarlier = in.GetTapNote(t, iRowEarlier);
			TapNote tnLater = in.GetTapNote(t, iRowLater);
			in.SetTapNote(t, iRowEarlier, tnLater);
			in.SetTapNote(t, iRowLater, tnEarlier);
		}
	}
	in.Convert4sToHoldNotes();
}

void NoteDataUtil::SwapSides( NoteData &in )
{
	in.ConvertHoldNotesTo4s();
	int max_row = in.GetLastRow();
	for( int r=0; r<=max_row; r++ )
	{
		for( int t=0; t<in.GetNumTracks()/2; t++ )
		{
			int iTrackEarlier = t;
			int iTrackLater = t + in.GetNumTracks()/2 + in.GetNumTracks()%2;

			// swap
			TapNote tnEarlier = in.GetTapNote(iTrackEarlier, r);
			TapNote tnLater = in.GetTapNote(iTrackLater, r);
			in.SetTapNote(iTrackEarlier, r, tnLater);
			in.SetTapNote(iTrackLater, r, tnEarlier);
		}
	}
	in.Convert4sToHoldNotes();
}

void NoteDataUtil::Little(NoteData &in, float fStartBeat, float fEndBeat)
{
	int i;

	// filter out all non-quarter notes
	int max_row = in.GetLastRow();
	for( i=0; i<=max_row; i+=1 ) 
		if( i % ROWS_PER_BEAT != 0 )
			for( int c=0; c<in.GetNumTracks(); c++ ) 
				in.SetTapNote(c, i, TAP_EMPTY);

	for( i=in.GetNumHoldNotes()-1; i>=0; i-- )
		if( fmodf(in.GetHoldNote(i).fStartBeat,1) != 0 )	// doesn't start on a beat
			in.RemoveHoldNote( i );
}

void NoteDataUtil::Wide( NoteData &in, float fStartBeat, float fEndBeat )
{
	// Make all all quarter notes into jumps.

	in.ConvertHoldNotesTo4s();


	int first_row = 0;
	if( fStartBeat != -1 )
		first_row = BeatToNoteRow(fStartBeat);

	int last_row = in.GetLastRow();
	if( fEndBeat != -1 )
		last_row = BeatToNoteRow(fEndBeat);
	
	for( int i=first_row; i<last_row; i+=ROWS_PER_BEAT*2 ) // every even beat
	{
		if( in.GetNumTapNonEmptyTracks(i) != 1 )
			continue;	// skip.  Don't place during holds

		if( in.GetNumTracksWithTap(i) != 1 )
			continue;	// skip

		bool bSpaceAroundIsEmpty = true;	// no other notes with a 1/8th of this row
		for( int j=i-ROWS_PER_BEAT/2+1; j<i+ROWS_PER_BEAT/2-1; j++ )
			if( j!=i  &&  in.GetNumTapNonEmptyTracks(j) > 0 )
			{
				bSpaceAroundIsEmpty = false;
				break;
			}
				
		if( !bSpaceAroundIsEmpty )
			continue;	// skip

		// add a note determinitsitcally
		int iBeat = (int)roundf( NoteRowToBeat(i) );
		int iTrackOfNote = in.GetFirstTrackWithTap(i);
		int iTrackToAdd = iTrackOfNote + (iBeat%5)-2;	// won't be more than 2 tracks away from the existing note
		CLAMP( iTrackToAdd, 0, in.GetNumTracks()-1 );
		if( iTrackToAdd == iTrackOfNote )
			iTrackToAdd++;
		CLAMP( iTrackToAdd, 0, in.GetNumTracks()-1 );
		if( iTrackToAdd == iTrackOfNote )
			iTrackToAdd--;
		CLAMP( iTrackToAdd, 0, in.GetNumTracks()-1 );

		if( in.GetTapNote(iTrackToAdd, i) != TAP_EMPTY )
		{
			iTrackToAdd = (iTrackToAdd+1) % in.GetNumTracks();
		}
		in.SetTapNote(iTrackToAdd, i, TAP_ADDITION);
	}

	in.Convert4sToHoldNotes();
}

void NoteDataUtil::Big( NoteData &in, float fStartBeat, float fEndBeat )
{
	InsertIntelligentTaps(in,1.0f,0.5f,false,fStartBeat,fEndBeat);	// add 8ths between 4ths
}

void NoteDataUtil::Quick( NoteData &in, float fStartBeat, float fEndBeat )
{
	InsertIntelligentTaps(in,0.5f,0.25f,false,fStartBeat,fEndBeat);	// add 16ths between 8ths
}

void NoteDataUtil::Skippy( NoteData &in, float fStartBeat, float fEndBeat )
{
	InsertIntelligentTaps(in,1.0f,0.75f,true,fStartBeat,fEndBeat);	// add 16ths between 4ths
}

void NoteDataUtil::InsertIntelligentTaps( NoteData &in, float fBeatInterval, float fInsertBeatOffset, bool bSkippy, float fStartBeat, float fEndBeat )
{
	ASSERT( fInsertBeatOffset <= fBeatInterval );

	in.ConvertHoldNotesTo4s();

	// Insert a beat in the middle of every fBeatInterval.

	int first_row = 0;
	if( fStartBeat != -1 )
		first_row = BeatToNoteRow(fStartBeat);

	int last_row = in.GetLastRow();
	if( fEndBeat != -1 )
		last_row = BeatToNoteRow(fEndBeat);

	int rows_per_interval = BeatToNoteRow( fBeatInterval );
	int insert_row_offset = BeatToNoteRow( fInsertBeatOffset );
	for( int i=first_row; i<last_row; i+=rows_per_interval ) 
	{
		int iRowEarlier = i;
		int iRowLater = i + rows_per_interval;
		int iRowToAdd = i + insert_row_offset;
		if( in.GetNumTapNonEmptyTracks(iRowEarlier)!=1 || in.GetNumTracksWithTap(iRowEarlier)!=1 )
			continue;
		if( in.GetNumTapNonEmptyTracks(iRowLater)!=1 || in.GetNumTracksWithTap(iRowLater)!=1 )
			continue;
		// there is a 4th and 8th note surrounding iRowBetween
		
		// don't insert a new note if there's already one within this interval
		bool bNoteInMiddle = false;
		for( int j=iRowEarlier+1; j<=iRowLater-1; j++ )
			if( !in.IsRowEmpty(j) )
			{
				bNoteInMiddle = true;
				break;
			}
		if( bNoteInMiddle )
			continue;

		// add a note determinitsitcally somewhere on a track different from the two surrounding notes
		int iTrackOfNoteEarlier = in.GetFirstNonEmptyTrack(iRowEarlier);
		int iTrackOfNoteLater = in.GetFirstNonEmptyTrack(iRowLater);
		int iTrackOfNoteToAdd = 0;
		if( bSkippy  &&
			iTrackOfNoteEarlier != iTrackOfNoteLater )	// Don't make skips on the same note
		{
			iTrackOfNoteToAdd = iTrackOfNoteEarlier;
		}
		else
		{
			// choose a randomish track
			if( abs(iTrackOfNoteEarlier-iTrackOfNoteLater) == 2 )
				iTrackOfNoteToAdd = (iTrackOfNoteEarlier+iTrackOfNoteLater)/2;
			else
			{
				for( int t=min(iTrackOfNoteEarlier,iTrackOfNoteLater)-1; t<=max(iTrackOfNoteEarlier,iTrackOfNoteLater)+1; t++ )
				{
					iTrackOfNoteToAdd = t;
					CLAMP( iTrackOfNoteToAdd, 0, in.GetNumTracks()-1 );
					if( iTrackOfNoteToAdd!=iTrackOfNoteEarlier && iTrackOfNoteToAdd!=iTrackOfNoteLater )
						break;
				}
			}
		}

		in.SetTapNote(iTrackOfNoteToAdd, iRowToAdd, TAP_ADDITION);
	}

	in.Convert4sToHoldNotes();
}

void NoteDataUtil::Mines( NoteData &in, float fStartBeat, float fEndBeat )
{
	int first_row = 0;
	if( fStartBeat != -1 )
		first_row = BeatToNoteRow(fStartBeat);

	int last_row = in.GetLastRow();
	if( fEndBeat != -1 )
		last_row = BeatToNoteRow(fEndBeat);

	//
	// Change whole rows at a time to be tap notes.  Otherwise, it causes
	// major problems for our scoring system. -Chris
	//

	int iRowCount = 0;
	for( int r=first_row; r<=last_row; r++ )
	{
		if( !in.IsRowEmpty(r) )
		{
			iRowCount++;

			// place every 6 or 7 rows
			if( (iRowCount>=7) || (iRowCount>=6 && rand()%2) )
			{
				for( int t=0; t<in.GetNumTracks(); t++ )
					if( in.GetTapNote(t,r) == TAP_TAP )
						in.SetTapNote(t,r,TAP_MINE);
				
				iRowCount = 0;
			}
		}
	}

	// Place mines right after hold so players must lift their foot.
	for( int i=0; i<in.GetNumHoldNotes(); i++ )
	{
		HoldNote &hn = in.GetHoldNote(i);
		float fHoldEndBeat = hn.fEndBeat;
		int iMineRow = BeatToNoteRow( fHoldEndBeat+0.5f );
		if( iMineRow < first_row || iMineRow > last_row )
			continue;
		
		// Add a mine right after the hold end.h
		in.SetTapNote(hn.iTrack,iMineRow,TAP_MINE);

		// Convert all other notes in this row to mines.
		for( int t=0; t<in.GetNumTracks(); t++ )
			if( in.GetTapNote(t,iMineRow) == TAP_TAP )
				in.SetTapNote(t,iMineRow,TAP_MINE);

		iRowCount = 0;
	}
}


void NoteDataUtil::SnapToNearestNoteType( NoteData &in, NoteType nt1, NoteType nt2, float fBeginBeat, float fEndBeat )
{
	// nt2 is optional and should be -1 if it is not used

	float fSnapInterval1, fSnapInterval2;
	switch( nt1 )
	{
		case NOTE_TYPE_4TH:		fSnapInterval1 = 1/1.0f;	break;
		case NOTE_TYPE_8TH:		fSnapInterval1 = 1/2.0f;	break;
		case NOTE_TYPE_12TH:	fSnapInterval1 = 1/3.0f;	break;
		case NOTE_TYPE_16TH:	fSnapInterval1 = 1/4.0f;	break;
		case NOTE_TYPE_24TH:	fSnapInterval1 = 1/6.0f;	break;
		case NOTE_TYPE_32ND:	fSnapInterval1 = 1/8.0f;	break;
		default:	ASSERT( false );						return;
	}

	switch( nt2 )
	{
		case NOTE_TYPE_4TH:		fSnapInterval2 = 1/1.0f;	break;
		case NOTE_TYPE_8TH:		fSnapInterval2 = 1/2.0f;	break;
		case NOTE_TYPE_12TH:	fSnapInterval2 = 1/3.0f;	break;
		case NOTE_TYPE_16TH:	fSnapInterval2 = 1/4.0f;	break;
		case NOTE_TYPE_24TH:	fSnapInterval2 = 1/6.0f;	break;
		case NOTE_TYPE_32ND:	fSnapInterval2 = 1/8.0f;	break;
		case -1:				fSnapInterval2 = 10000;		break;	// nothing will ever snap to this.  That's what we want!
		default:	ASSERT( false );						return;
	}

	int iNoteIndexBegin = BeatToNoteRow( fBeginBeat );
	int iNoteIndexEnd = BeatToNoteRow( fEndBeat );

	in.ConvertHoldNotesTo2sAnd3s();

	// iterate over all TapNotes in the interval and snap them
	for( int i=iNoteIndexBegin; i<=iNoteIndexEnd; i++ )
	{
		LOG->Trace("index %i", i);
		int iOldIndex = i;
		float fOldBeat = NoteRowToBeat( iOldIndex );
		float fNewBeat1 = froundf( fOldBeat, fSnapInterval1 );
		float fNewBeat2 = froundf( fOldBeat, fSnapInterval2 );

		LOG->Trace("oldbeat %.2f, nb1 %.2f, nb2 %.2f", fOldBeat, fNewBeat1, fNewBeat2);
		bool bNewBeat1IsCloser = fabsf(fNewBeat1-fOldBeat) < fabsf(fNewBeat2-fOldBeat);
		float fNewBeat = bNewBeat1IsCloser ? fNewBeat1 : fNewBeat2;
		int iNewIndex = BeatToNoteRow( fNewBeat );
		LOG->Trace("closer %i, newbeat %.2f, index %i", 
			bNewBeat1IsCloser, fNewBeat, iNewIndex);

		for( int c=0; c<in.GetNumTracks(); c++ )
		{
			if( iOldIndex == iNewIndex )
				continue;

			TapNote note = in.GetTapNote(c, iOldIndex);
			if( note == TAP_EMPTY )
				continue;

			in.SetTapNote(c, iOldIndex, TAP_EMPTY);

			const TapNote oldnote = in.GetTapNote(c, iNewIndex);
			if( note == TAP_TAP &&
				(oldnote == TAP_HOLD_HEAD || oldnote == TAP_HOLD_TAIL) )
				continue; // HoldNotes override TapNotes

			/* If two hold note boundaries are getting snapped together,
			 * merge them. */
			if( (note == TAP_HOLD_HEAD && oldnote == TAP_HOLD_TAIL) ||
				(note == TAP_HOLD_TAIL && oldnote == TAP_HOLD_HEAD))
				note = TAP_EMPTY;
			
			in.SetTapNote(c, iNewIndex, note );
		}
	}

	in.Convert2sAnd3sToHoldNotes();
}


void NoteDataUtil::CopyLeftToRight( NoteData &in )
{
	in.ConvertHoldNotesTo4s();
	int max_row = in.GetLastRow();
	for( int r=0; r<=max_row; r++ )
	{
		for( int t=0; t<in.GetNumTracks()/2; t++ )
		{
			int iTrackEarlier = t;
			int iTrackLater = in.GetNumTracks()-1-t;

			// swap
			TapNote tnEarlier = in.GetTapNote(iTrackEarlier, r);
//			TapNote tnLater = in.GetTapNote(iTrackLater, r);
//			in.SetTapNote(iTrackEarlier, r, tnLater);
			in.SetTapNote(iTrackLater, r, tnEarlier);
		}
	}
	in.Convert4sToHoldNotes();
}

void NoteDataUtil::CopyRightToLeft( NoteData &in )
{
	in.ConvertHoldNotesTo4s();
	int max_row = in.GetLastRow();
	for( int r=0; r<=max_row; r++ )
	{
		for( int t=0; t<in.GetNumTracks()/2; t++ )
		{
			int iTrackEarlier = t;
			int iTrackLater = in.GetNumTracks()-1-t;

			// swap
//			TapNote tnEarlier = in.GetTapNote(iTrackEarlier, r);
			TapNote tnLater = in.GetTapNote(iTrackLater, r);
			in.SetTapNote(iTrackEarlier, r, tnLater);
//			in.SetTapNote(iTrackLater, r, tnEarlier);
		}
	}
	in.Convert4sToHoldNotes();
}

void NoteDataUtil::ClearLeft( NoteData &in )
{
	in.ConvertHoldNotesTo4s();
	int max_row = in.GetLastRow();
	for( int r=0; r<=max_row; r++ )
		for( int t=0; t<in.GetNumTracks()/2; t++ )
			in.SetTapNote(t, r, TAP_EMPTY);
	in.Convert4sToHoldNotes();
}

void NoteDataUtil::ClearRight( NoteData &in )
{
	in.ConvertHoldNotesTo4s();
	int max_row = in.GetLastRow();
	for( int r=0; r<=max_row; r++ )
		for( int t=(in.GetNumTracks()+1)/2; t<in.GetNumTracks(); t++ )
			in.SetTapNote(t, r, TAP_EMPTY);
	in.Convert4sToHoldNotes();
}

void NoteDataUtil::CollapseToOne( NoteData &in )
{
	in.ConvertHoldNotesTo2sAnd3s();
	int max_row = in.GetLastRow();
	for( int r=0; r<=max_row; r++ )
		for( int t=0; t<in.GetNumTracks(); t++ )
			if( in.GetTapNote(t,r) != TAP_EMPTY )
			{
				TapNote tn = in.GetTapNote(t,r);
				in.SetTapNote(t, r, TAP_EMPTY);
				in.SetTapNote(0, r, tn);
			}
	in.Convert2sAnd3sToHoldNotes();
}

void NoteDataUtil::ShiftLeft( NoteData &in )
{
	in.ConvertHoldNotesTo4s();
	int max_row = in.GetLastRow();
	for( int r=0; r<=max_row; r++ )
	{
		for( int t=0; t<in.GetNumTracks()-1; t++ )	// in.GetNumTracks()-1 times
		{
			int iTrackEarlier = t;
			int iTrackLater = (t+1) % in.GetNumTracks();

			// swap
			TapNote tnEarlier = in.GetTapNote(iTrackEarlier, r);
			TapNote tnLater = in.GetTapNote(iTrackLater, r);
			in.SetTapNote(iTrackEarlier, r, tnLater);
			in.SetTapNote(iTrackLater, r, tnEarlier);
		}
	}
	in.Convert4sToHoldNotes();
}

void NoteDataUtil::ShiftRight( NoteData &in )
{
	in.ConvertHoldNotesTo4s();
	int max_row = in.GetLastRow();
	for( int r=0; r<=max_row; r++ )
	{
		for( int t=in.GetNumTracks()-1; t>0; t-- )	// in.GetNumTracks()-1 times
		{
			int iTrackEarlier = t;
			int iTrackLater = (t+1) % in.GetNumTracks();

			// swap
			TapNote tnEarlier = in.GetTapNote(iTrackEarlier, r);
			TapNote tnLater = in.GetTapNote(iTrackLater, r);
			in.SetTapNote(iTrackEarlier, r, tnLater);
			in.SetTapNote(iTrackLater, r, tnEarlier);
		}
	}
	in.Convert4sToHoldNotes();
}


struct ValidRow
{
	StepsType st;
	bool bValidMask[MAX_NOTE_TRACKS];
};
#define T true
#define f false
const ValidRow g_ValidRows[] = 
{
	{ STEPS_TYPE_DANCE_DOUBLE, { T,T,T,T,f,f,f,f } },
	{ STEPS_TYPE_DANCE_DOUBLE, { f,T,T,T,T,f,f,f } },
	{ STEPS_TYPE_DANCE_DOUBLE, { f,f,f,T,T,T,T,f } },
	{ STEPS_TYPE_DANCE_DOUBLE, { f,f,f,f,T,T,T,T } },
};

void NoteDataUtil::FixImpossibleRows( NoteData &in, StepsType st )
{
	vector<const ValidRow*> vpValidRowsToCheck;
	for( int i=0; i<ARRAYSIZE(g_ValidRows); i++ )
	{
		if( g_ValidRows[i].st == st )
			vpValidRowsToCheck.push_back( &g_ValidRows[i] );
	}

	// bail early if there's nothing to validate against
	if( vpValidRowsToCheck.empty() )
		return;

	// each row must pass at least one valid mask
	for( int r=0; r<=in.GetLastRow(); r++ )
	{
		// only check rows with jumps
		if( in.GetNumTapNonEmptyTracks(r) < 2 )
			continue;

		bool bPassedOneMask = false;
		for( unsigned i=0; i<vpValidRowsToCheck.size(); i++ )
		{
			const ValidRow &vr = *vpValidRowsToCheck[i];
			if( NoteDataUtil::RowPassesValidMask(in,r,vr.bValidMask) )
			{
				bPassedOneMask = true;
				break;
			}
			else
				int skjdksjd = 0;
		}

		if( !bPassedOneMask )
			in.EliminateAllButOneTap(r);
	}
}

bool NoteDataUtil::RowPassesValidMask( NoteData &in, int row, const bool bValidMask[] )
{
	for( int t=0; t<in.GetNumTracks(); t++ )
	{
		if( !bValidMask[t] && in.GetTapNote(t,row) != TAP_EMPTY )
			return false;
	}

	return true;
}

void NoteDataUtil::ConvertAdditionsToRegular( NoteData &in )
{
	for( int r=0; r<=in.GetLastRow(); r++ )
		for( int t=0; t<in.GetNumTracks(); t++ )
			if( in.GetTapNote(t,r) == TAP_ADDITION )
				in.SetTapNote(t,r,TAP_TAP);
}

void NoteDataUtil::TransformNoteData( NoteData &nd, PlayerOptions &po, StepsType st )
{
	if( !po.m_bHoldNotes )
		RemoveHoldNotes( nd );

	switch( po.m_Turn )
	{
	case PlayerOptions::TURN_NONE:																			break;
	case PlayerOptions::TURN_MIRROR:		NoteDataUtil::Turn( nd, st, NoteDataUtil::mirror );			break;
	case PlayerOptions::TURN_LEFT:			NoteDataUtil::Turn( nd, st, NoteDataUtil::left );			break;
	case PlayerOptions::TURN_RIGHT:			NoteDataUtil::Turn( nd, st, NoteDataUtil::right );			break;
	case PlayerOptions::TURN_SHUFFLE:		NoteDataUtil::Turn( nd, st, NoteDataUtil::shuffle );			break;
	case PlayerOptions::TURN_SUPER_SHUFFLE:	NoteDataUtil::Turn( nd, st, NoteDataUtil::super_shuffle );	break;
	default:		ASSERT(0);
	}

	switch( po.m_Transform )
	{
	case PlayerOptions::TRANSFORM_NONE:											break;
	case PlayerOptions::TRANSFORM_LITTLE:		NoteDataUtil::Little(nd);	break;
	case PlayerOptions::TRANSFORM_WIDE:			NoteDataUtil::Wide(nd);		break;
	case PlayerOptions::TRANSFORM_BIG:			NoteDataUtil::Big(nd);		break;
	case PlayerOptions::TRANSFORM_QUICK:		NoteDataUtil::Quick(nd);		break;
	case PlayerOptions::TRANSFORM_SKIPPY:		NoteDataUtil::Skippy(nd);	break;
	case PlayerOptions::TRANSFORM_MINES:		NoteDataUtil::Mines(nd);		break;
	default:		ASSERT(0);
	}
}

void NoteDataUtil::Scale( NoteData &nd, float fScale )
{
	ASSERT( fScale > 0 );

	NoteData temp;
	temp.CopyAll( &nd );
	nd.ClearAll();

	for( int r=0; r<=temp.GetLastRow(); r++ )
	{
		for( int t=0; t<temp.GetNumTracks(); t++ )
		{
			TapNote tn = temp.GetTapNote( t, r );
			if( tn != TAP_EMPTY )
			{
				temp.SetTapNote( t, r, TAP_EMPTY );

				int new_row = int(r*fScale);
				nd.SetTapNote( t, new_row, tn );
			}
		}
	}
}

void NoteDataUtil::ShiftRows( NoteData &nd, float fStartBeat, float fBeatsToShift )
{
	NoteData temp;
	temp.SetNumTracks( nd.GetNumTracks() );
	int iTakeFromRow=0;
	int iPasteAtRow;

	iTakeFromRow = BeatToNoteRow( fStartBeat );
	iPasteAtRow = BeatToNoteRow( fStartBeat );

	if( fBeatsToShift > 0 )	// add blank rows
		iPasteAtRow += BeatToNoteRow( fBeatsToShift );
	else	// delete rows
		iTakeFromRow += BeatToNoteRow( -fBeatsToShift );

	temp.CopyRange( &nd, iTakeFromRow, nd.GetLastRow() );
	nd.ClearRange( min(iTakeFromRow,iPasteAtRow), nd.GetLastRow()  );
	nd.CopyRange( &temp, 0, temp.GetLastRow(), iPasteAtRow );		
}
