/*
 * NoteData is organized by:
 *  track - corresponds to different columns of notes on the screen
 *  row/index - corresponds to subdivisions of beats
 */

#include "global.h"
#include "NoteData.h"
#include "RageUtil.h"
#include "RageLog.h"


NoteData::NoteData()
{
	Init();
}

void NoteData::Init()
{
	ClearAll();
	m_TapNotes.clear();
}

NoteData::~NoteData()
{
}

int NoteData::GetNumTracks() const
{
	return m_TapNotes.size();
}

void NoteData::SetNumTracks( int iNewNumTracks )
{
	ASSERT( iNewNumTracks > 0 );

	m_TapNotes.resize( iNewNumTracks );

	/* Remove all hold notes that are out of bounds. */
	// Iterate backwards so that we can delete.
	for( int h = m_HoldNotes.size()-1; h >= 0; --h )
		if( m_HoldNotes[h].iTrack >= iNewNumTracks )
			m_HoldNotes.erase( m_HoldNotes.begin()+h );
}


/* Clear [rowBegin,rowEnd]; that is, including rowEnd. */
void NoteData::ClearRange( int rowBegin, int rowEnd )
{
	this->ConvertHoldNotesTo4s();
	for( int t=0; t<GetNumTracks(); t++ )
	{
		FOREACH_NONEMPTY_ROW_IN_TRACK_RANGE( *this, t, r, rowBegin, rowEnd )
			SetTapNote(t, r, TAP_EMPTY);
	}
	this->Convert4sToHoldNotes();
}

void NoteData::ClearAll()
{
	for( int t=0; t<GetNumTracks(); t++ )
		m_TapNotes[t].clear();
	m_HoldNotes.clear();
}

/* Copy a range from pFrom to this.  (Note that this does *not* overlay;
 * all data in the range is overwritten.) */
void NoteData::CopyRange( const NoteData& from, int rowFromBegin, int rowFromEnd, int rowToBegin )
{
	ASSERT( from.GetNumTracks() == GetNumTracks() );

	NoteData From, To;
	From.To4s( from );
	To.To4s( *this );

	// copy recorded TapNotes
	
	for( int t=0; t<GetNumTracks(); t++ )
	{
		FOREACH_NONEMPTY_ROW_IN_TRACK_RANGE( From, t, iFrom, rowFromBegin, rowFromEnd )
		{
			int iTo = rowToBegin + iFrom - rowFromBegin;
			const TapNote &tn = From.GetTapNote( t, iFrom );
			To.SetTapNote( t, iTo, tn );
		}
	}

	this->From4s( To );
}

void NoteData::Config( const NoteData& from )
{
	SetNumTracks( from.GetNumTracks() );
}

void NoteData::CopyAll( const NoteData& from )
{
	Config(from);
	ClearAll();

	for( int c=0; c<GetNumTracks(); c++ )
		m_TapNotes[c] = from.m_TapNotes[c];
	m_HoldNotes = from.m_HoldNotes;
}

bool NoteData::IsRowEmpty( int row ) const
{
	for( int t=0; t<GetNumTracks(); t++ )
		if( GetTapNoteX(t, row).type != TapNote::empty )
			return false;
	return true;
}

bool NoteData::IsRangeEmpty( int track, int rowBegin, int rowEnd ) const
{
	ASSERT( track < GetNumTracks() );

	FOREACH_NONEMPTY_ROW_IN_TRACK_RANGE( *this, track, r, rowBegin, rowEnd )
		if( GetTapNoteX(track,r).type != TapNote::empty )
			return false;
	return true;
}

int NoteData::GetNumTapNonEmptyTracks( int row ) const
{
	int iNum = 0;
	for( int t=0; t<GetNumTracks(); t++ )
		if( GetTapNote(t, row).type != TapNote::empty )
			iNum++;
	return iNum;
}

void NoteData::GetTapNonEmptyTracks( int row, set<int>& addTo ) const
{
	for( int t=0; t<GetNumTracks(); t++ )
		if( GetTapNote(t, row).type != TapNote::empty )
			addTo.insert(t);
}

bool NoteData::GetTapFirstNonEmptyTrack( int row, int &iNonEmptyTrackOut ) const
{
	for( int t=0; t<GetNumTracks(); t++ )
	{
		if( GetTapNoteX( t, row ).type != TapNote::empty )
		{
			iNonEmptyTrackOut = t;
			return true;
		}
	}
	return false;
}

bool NoteData::GetTapFirstEmptyTrack( int row, int &iEmptyTrackOut ) const
{
	for( int t=0; t<GetNumTracks(); t++ )
	{
		if( GetTapNoteX( t, row ).type == TapNote::empty )
		{
			iEmptyTrackOut = t;
			return true;
		}
	}
	return false;
}

bool NoteData::GetTapLastEmptyTrack( int row, int &iEmptyTrackOut ) const
{
	for( int t=GetNumTracks()-1; t>=0; t-- )
	{
		if( GetTapNoteX( t, row ).type == TapNote::empty )
		{
			iEmptyTrackOut = t;
			return true;
		}
	}
	return false;
}

int NoteData::GetNumTracksWithTap( int row ) const
{
	int iNum = 0;
	for( int t=0; t<GetNumTracks(); t++ )
	{
		const TapNote &tn = GetTapNoteX( t, row );
		if( tn.type == TapNote::tap )
			iNum++;
	}
	return iNum;
}

int NoteData::GetNumTracksWithTapOrHoldHead( int row ) const
{
	int iNum = 0;
	for( int t=0; t<GetNumTracks(); t++ )
	{
		const TapNote &tn = GetTapNoteX( t, row );
		if( tn.type == TapNote::tap || tn.type == TapNote::hold_head )
			iNum++;
	}
	return iNum;
}

int NoteData::GetFirstTrackWithTap( int row ) const
{
	for( int t=0; t<GetNumTracks(); t++ )
	{
		const TapNote &tn = GetTapNoteX( t, row );
		if( tn.type == TapNote::tap )
			return t;
	}
	return -1;
}

int NoteData::GetFirstTrackWithTapOrHoldHead( int row ) const
{
	for( int t=0; t<GetNumTracks(); t++ )
	{
		const TapNote &tn = GetTapNoteX( t, row );
		if( tn.type == TapNote::tap || tn.type == TapNote::hold_head )
			return t;
	}
	return -1;
}

void NoteData::AddHoldNote( HoldNote add )
{
	ASSERT( add.iStartRow>=0 && add.iEndRow>=0 );

	// look for other hold notes that overlap and merge them
	// XXX: this is done implicitly with 4s, but 4s uses this function.
	// Rework this later.
	for( int i=0; i<GetNumHoldNotes(); i++ )	// for each HoldNote
	{
		HoldNote &other = GetHoldNote(i);
		if( add.iTrack == other.iTrack  &&		// the tracks correspond
			add.RangeOverlaps(other) ) // they overlap
		{
			add.iStartRow = min(add.iStartRow, other.iStartRow);
			add.iEndRow = max(add.iEndRow, other.iEndRow);

			// delete this HoldNote
			RemoveHoldNote( i );
			--i;
		}
	}

	int iAddStartIndex = add.iStartRow;
	int iAddEndIndex = add.iEndRow;

	// delete TapNotes under this HoldNote
	FOREACH_NONEMPTY_ROW_IN_TRACK_RANGE( *this, add.iTrack, i, iAddStartIndex+1, iAddEndIndex )
		SetTapNote( add.iTrack, i, TAP_EMPTY );

	// add a tap note at the start of this hold
	SetTapNote( add.iTrack, iAddStartIndex, TAP_ORIGINAL_HOLD_HEAD );		// Hold begin marker.  Don't draw this, but do grade it.

	m_HoldNotes.push_back(add);
}

void NoteData::RemoveHoldNote( int iHoldIndex )
{
	ASSERT( iHoldIndex >= 0  &&  iHoldIndex < GetNumHoldNotes() );

	HoldNote& hn = GetHoldNote(iHoldIndex);

	const int iHoldStartIndex = hn.iStartRow;

	// delete a tap note at the start of this hold
	SetTapNote(hn.iTrack, iHoldStartIndex, TAP_EMPTY);

	// remove from list
	m_HoldNotes.erase(m_HoldNotes.begin()+iHoldIndex, m_HoldNotes.begin()+iHoldIndex+1);
}

/* Get a hold note with the same track and end row as hn. */
int NoteData::GetMatchingHoldNote( const HoldNote &hn ) const
{
	for( int i=0; i<GetNumHoldNotes(); i++ )	// for each HoldNote
	{
		const HoldNote &ret = GetHoldNote(i);
		if( ret.iTrack == hn.iTrack && ret.iEndRow == hn.iEndRow )
			return i;
	}
	FAIL_M( ssprintf("%i..%i, %i", hn.iStartRow, hn.iEndRow, hn.iTrack) );
}


void NoteData::SetTapAttackNote( int track, int row, CString sModifiers, float fDurationSeconds )
{
	TapNote tn(
		TapNote::attack, 
		TapNote::original, 
		sModifiers,
		fDurationSeconds, 
		false,
		0 );
	SetTapNote( track, row, tn );
}

int NoteData::GetFirstRow() const
{ 
	int iEarliestRowFoundSoFar = -1;
	
	for( int t=0; t < GetNumTracks(); t++ )
	{
		const TrackMap &trackMap = m_TapNotes[t];
		TrackMap::const_iterator iter = trackMap.begin();
		if( iter == trackMap.end() )	// trackMap is empty
			continue;
		if( iEarliestRowFoundSoFar == -1 )
			iEarliestRowFoundSoFar = iter->first;
		else
			iEarliestRowFoundSoFar = min( iEarliestRowFoundSoFar, iter->first );
	}

	for( int i=0; i<GetNumHoldNotes(); i++ )
	{
		if( iEarliestRowFoundSoFar == -1 ||
			GetHoldNote(i).iStartRow < iEarliestRowFoundSoFar )
			iEarliestRowFoundSoFar = GetHoldNote(i).iStartRow;
	}

	if( iEarliestRowFoundSoFar == -1 )	// there are no notes
		return 0;

	return iEarliestRowFoundSoFar;
}

int NoteData::GetLastRow() const
{ 
	int iOldestRowFoundSoFar = 0;
	
	for( int t=0; t < GetNumTracks(); t++ )
	{
		const TrackMap &trackMap = m_TapNotes[t];
		const TrackMap::const_reverse_iterator  iter = trackMap.rbegin();
		if( iter == trackMap.rend() )	// trackMap is empty
			continue;
		iOldestRowFoundSoFar = max( iOldestRowFoundSoFar, iter->first );
	}

	for( int i=0; i<GetNumHoldNotes(); i++ )
	{
		if( GetHoldNote(i).iEndRow > iOldestRowFoundSoFar )
			iOldestRowFoundSoFar = GetHoldNote(i).iEndRow;
	}

	return iOldestRowFoundSoFar;
}

int NoteData::GetNumTapNotes( float fStartBeat, float fEndBeat ) const
{
	if( fEndBeat == -1 )
		fEndBeat = 999999;

	int iNumNotes = 0;

	int iStartIndex = BeatToNoteRow( fStartBeat );
	int iEndIndex = BeatToNoteRow( fEndBeat );

	/* Clamp to known-good ranges. */
	iStartIndex = max( iStartIndex, 0 );
	iEndIndex = min( iEndIndex, GetLastRow() );
	
	for( int t=0; t<GetNumTracks(); t++ )
	{
		FOREACH_NONEMPTY_ROW_IN_TRACK_RANGE( *this, t, r, iStartIndex, iEndIndex )
		{
			const TapNote &tn = GetTapNoteX(t, r);
			if( tn.type != TapNote::empty  &&  tn.type != TapNote::mine )
				iNumNotes++;
		}
	}
	
	return iNumNotes;
}

int NoteData::GetNumRowsWithTap( float fStartBeat, float fEndBeat ) const
{
	if( fEndBeat == -1 ) 
		fEndBeat = GetLastBeat();

	int iNumNotes = 0;

	int iStartIndex = BeatToNoteRow( fStartBeat );
	int iEndIndex = BeatToNoteRow( fEndBeat );
	
	FOREACH_NONEMPTY_ROW_ALL_TRACKS_RANGE( *this, r, iStartIndex, iEndIndex )
		if( IsThereATapAtRow(r) )
			iNumNotes++;
	
	return iNumNotes;
}

int NoteData::GetNumMines( float fStartBeat, float fEndBeat ) const
{
	if( fEndBeat == -1 )
		fEndBeat = 999999;

	int iNumMines = 0;

	int iStartIndex = BeatToNoteRow( fStartBeat );
	int iEndIndex = BeatToNoteRow( fEndBeat );

	/* Clamp to known-good ranges. */
	iStartIndex = max( iStartIndex, 0 );
	iEndIndex = min( iEndIndex, GetLastRow() );
	
	for( int t=0; t<GetNumTracks(); t++ )
	{
		FOREACH_NONEMPTY_ROW_IN_TRACK_RANGE( *this, t, r, iStartIndex, iEndIndex )
			if( GetTapNoteX(t, r).type == TapNote::mine )
				iNumMines++;
	}
	
	return iNumMines;
}

int NoteData::GetNumRowsWithTapOrHoldHead( float fStartBeat, float fEndBeat ) const
{
	if( fEndBeat == -1 )
		fEndBeat = GetLastBeat();

	int iNumNotes = 0;

	int iStartIndex = BeatToNoteRow( fStartBeat );
	int iEndIndex = BeatToNoteRow( fEndBeat );
	
	FOREACH_NONEMPTY_ROW_ALL_TRACKS_RANGE( *this, r, iStartIndex, iEndIndex )
		if( IsThereATapOrHoldHeadAtRow(r) )
			iNumNotes++;
	
	return iNumNotes;
}

int NoteData::RowNeedsHands( const int row ) const
{
	int iNumNotesThisIndex = 0;
	for( int t=0; t<GetNumTracks(); t++ )
	{
		const TapNote &tn = GetTapNoteX(t, row);
		switch( tn.type )
		{
		case TapNote::mine:
		case TapNote::empty:
		case TapNote::hold_tail:
			continue;	// skip these types - they don't count
		}
		++iNumNotesThisIndex;
	}

	/* We must have at least one non-hold-body at this row to count it. */
	if( !iNumNotesThisIndex )
		return false;

	if( iNumNotesThisIndex < 3 )
	{
		/* We have at least one, but not enough.  Count holds. */
		for( int j=0; j<GetNumHoldNotes(); j++ )
		{
			const HoldNote &hn = GetHoldNote(j);
			if( hn.iStartRow+1 <= row && row <= hn.iEndRow )
				++iNumNotesThisIndex;
		}
	}

	return iNumNotesThisIndex >= 3;
}

int NoteData::GetNumHands( float fStartBeat, float fEndBeat ) const
{
	/* Count the number of times you have to use your hands.  This includes
	 * three taps at the same time, a tap while two hold notes are being held,
	 * etc.  Only count rows that have at least one tap note (hold heads count).
	 * Otherwise, every row of hold notes counts, so three simultaneous hold
	 * notes will count as hundreds of "hands". */
	if( fEndBeat == -1 )
		fEndBeat = GetLastBeat();

	int iStartIndex = BeatToNoteRow( fStartBeat );
	int iEndIndex = BeatToNoteRow( fEndBeat );

	/* Clamp to known-good ranges. */
	iStartIndex = max( iStartIndex, 0 );
	iEndIndex = min( iEndIndex, GetLastRow() );

	int iNum = 0;
	FOREACH_NONEMPTY_ROW_ALL_TRACKS_RANGE( *this, r, iStartIndex, iEndIndex )
	{
		if( !RowNeedsHands(r) )
			continue;

		iNum++;
	}

	return iNum;
}

int NoteData::GetNumN( int iMinTaps, float fStartBeat, float fEndBeat ) const
{
	if( fEndBeat == -1 )
		fEndBeat = GetLastBeat();

	int iStartIndex = BeatToNoteRow( fStartBeat );
	int iEndIndex = BeatToNoteRow( fEndBeat );

	/* Clamp to known-good ranges. */
	iStartIndex = max( iStartIndex, 0 );
	iEndIndex = min( iEndIndex, GetLastRow() );

	int iNum = 0;
	FOREACH_NONEMPTY_ROW_ALL_TRACKS_RANGE( *this, r, iStartIndex, iEndIndex )
	{
		int iNumNotesThisIndex = 0;
		for( int t=0; t<GetNumTracks(); t++ )
		{
			const TapNote &tn = GetTapNoteX(t, r);
			if( tn.type != TapNote::mine  &&  tn.type != TapNote::empty )	// mines don't count
				iNumNotesThisIndex++;
		}
		if( iNumNotesThisIndex >= iMinTaps )
			iNum++;
	}
	
	return iNum;
}

int NoteData::GetNumHoldNotes( float fStartBeat, float fEndBeat ) const
{
	if( fEndBeat == -1 )
		fEndBeat = GetLastBeat();

	int iStartIndex = BeatToNoteRow( fStartBeat );
	int iEndIndex = BeatToNoteRow( fEndBeat );

	int iNumHolds = 0;
	for( int i=0; i<GetNumHoldNotes(); i++ )
	{
		const HoldNote &hn = GetHoldNote(i);
		if( iStartIndex <= hn.iStartRow &&  hn.iEndRow <= iEndIndex )
			iNumHolds++;
	}
	return iNumHolds;
}

void NoteData::Convert2sAnd3sToHoldNotes()
{
	// Any note will end a hold (not just a TAP_HOLD_TAIL).  This makes parsing DWIs much easier.
	// Plus, allowing tap notes in the middle of a hold doesn't make sense!

	for( int t=0; t<GetNumTracks(); t++ )	// foreach column
	{
		FOREACH_NONEMPTY_ROW_IN_TRACK( *this, t, r )
		{
			if( GetTapNote(t,r).type != TapNote::hold_head )
				continue;	// skip

			SetTapNote(t, r, TAP_EMPTY);	// clear the hold head marker

			// search for end of HoldNote
			FOREACH_NONEMPTY_ROW_IN_TRACK_RANGE( *this, t, j, r+1, 999999 )
			{
				// End hold on the next note we see.  This should be a hold_tail if the 
				// data is in a consistent state, but doesn't have to be.
				if( GetTapNote(t, j).type == TapNote::empty )
					continue;

				SetTapNote(t, j, TAP_EMPTY);

				AddHoldNote( HoldNote(t, r, j) );
				break;	// done searching for the end of this hold
			}
		}
	}
}


/* "102000301" ==
 * "104444001" */
void NoteData::ConvertHoldNotesTo2sAnd3s()
{
	// copy HoldNotes into the new structure, but expand them into 2s and 3s
	for( int i=0; i<GetNumHoldNotes(); i++ ) 
	{
		const HoldNote &hn = GetHoldNote(i);
		
		/* If they're the same, then they got clamped together, so just ignore it. */
		if( hn.iStartRow != hn.iEndRow )
		{
			SetTapNote( hn.iTrack, hn.iStartRow, TAP_ORIGINAL_HOLD_HEAD );
			SetTapNote( hn.iTrack, hn.iEndRow, TAP_ORIGINAL_HOLD_TAIL );
		}
	}
	m_HoldNotes.clear();
}


void NoteData::To2sAnd3s( const NoteData& from )
{
	CopyAll( from );
	ConvertHoldNotesTo2sAnd3s();
}

void NoteData::From2sAnd3s( const NoteData& from )
{
	CopyAll( from );
	Convert2sAnd3sToHoldNotes();
}

void NoteData::To4s( const NoteData& from )
{
	CopyAll( from );
	ConvertHoldNotesTo4s();
}

void NoteData::From4s( const NoteData& from )
{
	CopyAll( from );
	Convert4sToHoldNotes();
}

/* "104444001" ==
 * "102000301"
 *
 * "4441" basically means "hold for three rows then hold for another tap";
 * since taps don't really have a length, it's equivalent to "4440".
 * So, make sure the character after a 4 is always a 0. */
void NoteData::Convert4sToHoldNotes()
{
	for( int t=0; t<GetNumTracks(); t++ )	// foreach column
	{
		FOREACH_NONEMPTY_ROW_IN_TRACK( *this, t, r )
		{
			if( GetTapNote(t, r).type == TapNote::hold )	// this is a HoldNote body
			{
				HoldNote hn( t, r, 0 );
				// search for end of HoldNote
				do {
					SetTapNote( t, r, TAP_EMPTY );
					r++;
				} while( GetTapNote(t, r).type == TapNote::hold );
				SetTapNote( t, r, TAP_EMPTY );

				hn.iEndRow = r;
				AddHoldNote( hn );
			}
		}
	}
}

void NoteData::ConvertHoldNotesTo4s()
{
	// copy HoldNotes into the new structure, but expand them into 4s
	for( int i=0; i<GetNumHoldNotes(); i++ ) 
	{
		const HoldNote &hn = GetHoldNote(i);
		for( int j = hn.iStartRow; j < hn.iEndRow; ++j)
			SetTapNote(hn.iTrack, j, TAP_ORIGINAL_HOLD);
	}
	m_HoldNotes.clear();
}

// -1 for iOriginalTracksToTakeFrom means no track
void NoteData::LoadTransformed( const NoteData& original, int iNewNumTracks, const int iOriginalTrackToTakeFrom[] )
{
	// reset all notes
	Init();
	
	NoteData Original;
	Original.To4s( original );

	Config( Original );
	SetNumTracks( iNewNumTracks );

	// copy tracks
	for( int t=0; t<GetNumTracks(); t++ )
	{
		const int iOriginalTrack = iOriginalTrackToTakeFrom[t];
		ASSERT_M( iOriginalTrack < Original.GetNumTracks(), ssprintf("from %i >= %i (to %i)", 
			iOriginalTrack, Original.GetNumTracks(), iOriginalTrackToTakeFrom[t]));

		if( iOriginalTrack == -1 )
			continue;
		m_TapNotes[t] = Original.m_TapNotes[iOriginalTrack];
	}

	Convert4sToHoldNotes();
}

void NoteData::PadTapNotes( int rows )
{
	// Nothing to do for a track map.
}

void NoteData::MoveTapNoteTrack( int dest, int src )
{
	if(dest == src) return;
	m_TapNotes[dest] = m_TapNotes[src];
	m_TapNotes[src].clear();
}

void NoteData::SetTapNote( int track, int row, const TapNote& t )
{
	DEBUG_ASSERT( track>=0 && track<GetNumTracks() );
	
	if( row < 0 )
		return;

	// There's no point in inserting empty notes into the map.
	// Any blank space in the map is defined to be empty.
	// If we're trying to insert an empty at a spot where
	// another note already exists, then we're really deleting
	// from the map.
	if( t == TAP_EMPTY )
	{
		TrackMap &trackMap = m_TapNotes[track];
		// remove the element at this position (if any).  This will return either 0 or 1.
		trackMap.erase( row );
	}
	else
	{
		PadTapNotes(row);
		m_TapNotes[track][row] = t;
	}
}

void NoteData::ReserveRows( int row )
{
	// Nothing to do for a track map.
}

void NoteData::EliminateAllButOneTap( int row )
{
	if(row < 0) return;

	PadTapNotes(row);

	int track;
	for(track = 0; track < GetNumTracks(); ++track)
	{
		if( m_TapNotes[track][row].type == TapNote::tap )
			break;
	}

	track++;

	for( ; track < GetNumTracks(); ++track)
	{
		if( m_TapNotes[track][row].type == TapNote::tap )
			m_TapNotes[track][row] = TAP_EMPTY;
	}
}

void NoteData::GetTracksHeldAtRow( int row, set<int>& addTo )
{
	for( unsigned i=0; i<m_HoldNotes.size(); i++ )
	{
		const HoldNote& hn = m_HoldNotes[i];
		if( hn.RowIsInRange(row) )
			addTo.insert( hn.iTrack );
	}
}

int NoteData::GetNumTracksHeldAtRow( int row )
{
	static set<int> viTracks;
	viTracks.clear();
	GetTracksHeldAtRow( row, viTracks );
	return viTracks.size();
}

bool NoteData::GetNextTapNoteRowForTrack( int track, int &rowInOut ) const
{
	const TrackMap &mapTrack = m_TapNotes[track];

	// lower_bound and upper_bound have the same effect here because duplicate 
	// keys aren't allowed.
	TrackMap::const_iterator iter = mapTrack.lower_bound( rowInOut+1 );	// "find the first note for which row+1 < key == false"
	if( iter != mapTrack.end() )
	{
		ASSERT( iter->first > rowInOut );
		rowInOut = iter->first;
		return true;
	}
	else
	{
		return false;
	}
}

bool NoteData::GetNextTapNoteRowForAllTracks( int &rowInOut ) const
{
	int iClosestNextRow = 999999;
	bool bAnyHaveNextNote = false;
	for( int t=0; t<GetNumTracks(); t++ )
	{
		int iNewRowThisTrack = rowInOut;
		if( GetNextTapNoteRowForTrack( t, iNewRowThisTrack ) )
		{
			bAnyHaveNextNote = true;
			iClosestNextRow = min( iClosestNextRow, iNewRowThisTrack );
		}
	}

	if( bAnyHaveNextNote )
	{
		rowInOut = iClosestNextRow;
		return true;
	}
	else
	{
		return false;
	}
}

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
