#include "global.h"
#include "TimingData.h"
#include "PrefsManager.h"
#include "RageUtil.h"
#include "RageLog.h"
#include "NoteTypes.h"
#include <float.h>

const float BPMSegment::INFINITE_BPM = 9999999.f;
const float BPMSegment::INFINITE_BPS = INFINITE_BPM / 60.f;

void BPMSegment::SetBPM( float f )
{
	m_fBPS = f / 60.0f;
}

float BPMSegment::GetBPM() const
{
	return m_fBPS * 60.0f;
}

TimingData::TimingData()
{
	m_fBeat0OffsetInSeconds = 0;
}

static int CompareBPMSegments(const BPMSegment &seg1, const BPMSegment &seg2)
{
	return seg1.m_iStartIndex < seg2.m_iStartIndex;
}
void SortBPMSegmentsArray( vector<BPMSegment> &arrayBPMSegments )
{
	sort( arrayBPMSegments.begin(), arrayBPMSegments.end(), CompareBPMSegments );
}

static int CompareStopSegments(const StopSegment &seg1, const StopSegment &seg2)
{
	return seg1.m_iStartRow < seg2.m_iStartRow;
}
void SortStopSegmentsArray( vector<StopSegment> &arrayStopSegments )
{
	sort( arrayStopSegments.begin(), arrayStopSegments.end(), CompareStopSegments );
}

void TimingData::GetActualBPM( float &fMinBPMOut, float &fMaxBPMOut ) const
{
	fMinBPMOut = FLT_MAX;
	fMaxBPMOut = 0;
	for( unsigned i=0; i<m_BPMSegments.size(); i++ ) 
	{
		const BPMSegment &seg = m_BPMSegments[i];
		fMaxBPMOut = max( seg.m_fBPS * 60.0f, fMaxBPMOut );
		fMinBPMOut = min( seg.m_fBPS * 60.0f, fMinBPMOut );
	}
}


void TimingData::AddBPMSegment( const BPMSegment &seg )
{
	m_BPMSegments.push_back( seg );
	SortBPMSegmentsArray( m_BPMSegments );
}

void TimingData::AddStopSegment( const StopSegment &seg )
{
	m_StopSegments.push_back( seg );
	SortStopSegmentsArray( m_StopSegments );
}

/* Change an existing BPM segment, merge identical segments together or insert a new one. */
void TimingData::SetBPMAtRow( int iNoteRow, float fBPM )
{
	float fBPS = fBPM / 60.0f;
	unsigned i;
	for( i=0; i<m_BPMSegments.size(); i++ )
		if( m_BPMSegments[i].m_iStartIndex >= iNoteRow )
			break;

	if( i == m_BPMSegments.size() || m_BPMSegments[i].m_iStartIndex != iNoteRow )
	{
		// There is no BPMSegment at the specified beat.  If the BPM being set differs
		// from the last BPMSegment's BPM, create a new BPMSegment.
		if( i == 0 || fabsf(m_BPMSegments[i-1].m_fBPS - fBPS) > 1e-5f )
			AddBPMSegment( BPMSegment(iNoteRow, fBPM) );
	}
	else	// BPMSegment being modified is m_BPMSegments[i]
	{
		if( i > 0  &&  fabsf(m_BPMSegments[i-1].m_fBPS - fBPS) < 1e-5f )
			m_BPMSegments.erase( m_BPMSegments.begin()+i, m_BPMSegments.begin()+i+1 );
		else
			m_BPMSegments[i].m_fBPS = fBPS;
	}
}

void TimingData::SetStopAtRow( int iRow, float fSeconds )
{
	unsigned i;
	for( i=0; i<m_StopSegments.size(); i++ )
		if( m_StopSegments[i].m_iStartRow == iRow )
			break;

	if( i == m_StopSegments.size() )	// there is no BPMSegment at the current beat
	{
		// create a new StopSegment
		if( fSeconds > 0 )
			AddStopSegment( StopSegment(iRow, fSeconds) );
	}
	else	// StopSegment being modified is m_StopSegments[i]
	{
		if( fSeconds > 0 )
			m_StopSegments[i].m_fStopSeconds = fSeconds;
		else
			m_StopSegments.erase( m_StopSegments.begin()+i, m_StopSegments.begin()+i+1 );
	}
}

float TimingData::GetStopAtRow( int iNoteRow ) const
{
	for( unsigned i=0; i<m_StopSegments.size(); i++ )
	{
		if( m_StopSegments[i].m_iStartRow == iNoteRow )
			return m_StopSegments[i].m_fStopSeconds;
	}

	return 0;
}

/* Multiply the BPM in the range [fStartBeat,fEndBeat) by fFactor. */
void TimingData::MultiplyBPMInBeatRange( int iStartIndex, int iEndIndex, float fFactor )
{
	/* Change all other BPM segments in this range. */
	for( unsigned i=0; i<m_BPMSegments.size(); i++ )
	{
		const int iStartIndexThisSegment = m_BPMSegments[i].m_iStartIndex;
		const bool bIsLastBPMSegment = i==m_BPMSegments.size()-1;
		const int iStartIndexNextSegment = bIsLastBPMSegment ? INT_MAX : m_BPMSegments[i+1].m_iStartIndex;

		if( iStartIndexThisSegment <= iStartIndex && iStartIndexNextSegment <= iStartIndex )
			continue;

		/* If this BPM segment crosses the beginning of the range, split it into two. */
		if( iStartIndexThisSegment < iStartIndex && iStartIndexNextSegment > iStartIndex )
		{
			BPMSegment b = m_BPMSegments[i];
			b.m_iStartIndex = iStartIndexNextSegment;
			m_BPMSegments.insert( m_BPMSegments.begin()+i+1, b );

			/* Don't apply the BPM change to the first half of the segment we just split,
			 * since it lies outside the range. */
			continue;
		}

		/* If this BPM segment crosses the end of the range, split it into two. */
		if( iStartIndexThisSegment < iEndIndex && iStartIndexNextSegment > iEndIndex )
		{
			BPMSegment b = m_BPMSegments[i];
			b.m_iStartIndex = iEndIndex;
			m_BPMSegments.insert( m_BPMSegments.begin()+i+1, b );
		}
		else if( iStartIndexNextSegment > iEndIndex )
			continue;

		m_BPMSegments[i].m_fBPS = m_BPMSegments[i].m_fBPS * fFactor;
	}
}

float TimingData::GetBPMAtBeat( float fBeat ) const
{
	int iIndex = BeatToNoteRow( fBeat );
	unsigned i;
	for( i=0; i<m_BPMSegments.size()-1; i++ )
		if( m_BPMSegments[i+1].m_iStartIndex > iIndex )
			break;
	return m_BPMSegments[i].GetBPM();
}

int TimingData::GetBPMSegmentIndexAtBeat( float fBeat )
{
	int iIndex = BeatToNoteRow( fBeat );
	int i;
	for( i=0; i<(int)(m_BPMSegments.size())-1; i++ )
		if( m_BPMSegments[i+1].m_iStartIndex > iIndex )
			break;
	return i;
}

BPMSegment& TimingData::GetBPMSegmentAtBeat( float fBeat )
{
	static BPMSegment empty;
	if( m_BPMSegments.empty() )
	{
		empty = BPMSegment();
		return empty;
	}
	
	int i = GetBPMSegmentIndexAtBeat( fBeat );
	return m_BPMSegments[i];
}

void TimingData::GetBeatAndBPSFromElapsedTime( float fElapsedTime, float &fBeatOut, float &fBPSOut, bool &bFreezeOut ) const
{
	fElapsedTime += PREFSMAN->m_fGlobalOffsetSeconds;

	GetBeatAndBPSFromElapsedTimeNoOffset( fElapsedTime, fBeatOut, fBPSOut, bFreezeOut );
}

struct UnreachableNoteInfo
{
	int iStartRow;
	float fLengthSeconds;
};

void TimingData::GetBeatAndBPSFromElapsedTimeNoOffset( float fElapsedTime, float &fBeatOut, float &fBPSOut, bool &bFreezeOut ) const
{
//	LOG->Trace( "GetBeatAndBPSFromElapsedTime( fElapsedTime = %f )", fElapsedTime );

	fElapsedTime += m_fBeat0OffsetInSeconds;

	for( unsigned i=0; i<m_BPMSegments.size(); i++ ) // foreach BPMSegment
	{
		const int iStartRowThisSegment = m_BPMSegments[i].m_iStartIndex;
		const float fStartBeatThisSegment = NoteRowToBeat( iStartRowThisSegment );
		const bool bIsFirstBPMSegment = i==0;
		const bool bIsLastBPMSegment = i==m_BPMSegments.size()-1;
		const int iStartRowNextSegment = bIsLastBPMSegment ? MAX_NOTE_ROW : m_BPMSegments[i+1].m_iStartIndex; 
		const float fStartBeatNextSegment = NoteRowToBeat( iStartRowNextSegment );
		const float fBPS = m_BPMSegments[i].m_fBPS;

		FOREACH_CONST( StopSegment, m_StopSegments, ss )
		{
			if( !bIsFirstBPMSegment && iStartRowThisSegment >= ss->m_iStartRow )
				continue;
			if( !bIsLastBPMSegment && ss->m_iStartRow > iStartRowNextSegment )
				continue;

			// If we get here, this StopSegment lies within the current BPMSegment

			const int iRowsSinceStartOfSegment = ss->m_iStartRow - iStartRowThisSegment;
			const float fBeatsSinceStartOfSegment = NoteRowToBeat(iRowsSinceStartOfSegment);
			const float fFreezeStartSecond = fBeatsSinceStartOfSegment / fBPS;
			
			if( fFreezeStartSecond >= fElapsedTime )
				break;

			// the freeze segment is <= current time
			fElapsedTime -= ss->m_fStopSeconds;

			if( fFreezeStartSecond >= fElapsedTime )
			{
				/* The time lies within the stop. */
				fBeatOut = NoteRowToBeat(ss->m_iStartRow);
				fBPSOut = fBPS;
				bFreezeOut = true;
				return;
			}
		}

		const float fBeatsInThisSegment = fStartBeatNextSegment - fStartBeatThisSegment;
		const float fSecondsInThisSegment =  fBeatsInThisSegment / fBPS;
		if( bIsLastBPMSegment || fElapsedTime <= fSecondsInThisSegment )
		{
			// this BPMSegment IS the current segment
			fBeatOut = fStartBeatThisSegment + fElapsedTime*fBPS;
			fBPSOut = fBPS;
			bFreezeOut = false;
			return;
		}

		// this BPMSegment is NOT the current segment
		fElapsedTime -= fSecondsInThisSegment;
	}
}

void TimingData::GetUnreachableSegments( vector<UnreachableSegment> &vUnreachable ) const
{
	FOREACH_CONST( BPMSegment, m_BPMSegments, seg )
	{
		bool bIsLast = seg == m_BPMSegments.end();
		if( bIsLast )
			continue;
		
		bool bIsInfiniteBPM = seg->m_fBPS >= BPMSegment::INFINITE_BPS;
		if( !bIsInfiniteBPM )
			continue;
			
		int iSegmentEnd = (seg + 1)->m_iStartIndex;
		
		UnreachableSegment us;
		us.iNoteRowStart = seg->m_iStartIndex;
		us.iNoteRowEnd = iSegmentEnd;
		vUnreachable.push_back( us );
	}
}

float TimingData::GetElapsedTimeFromBeat( float fBeat ) const
{
	return TimingData::GetElapsedTimeFromBeatNoOffset( fBeat ) - PREFSMAN->m_fGlobalOffsetSeconds;
}

float TimingData::GetElapsedTimeFromBeatNoOffset( float fBeat ) const
{
	float fElapsedTime = 0;
	fElapsedTime -= m_fBeat0OffsetInSeconds;

	int iRow = BeatToNoteRow(fBeat);
	for( unsigned j=0; j<m_StopSegments.size(); j++ )	// foreach freeze
	{
		/* The exact beat of a stop comes before the stop, not after, so use >=, not >. */
		if( m_StopSegments[j].m_iStartRow >= iRow )
			break;
		fElapsedTime += m_StopSegments[j].m_fStopSeconds;
	}

	for( unsigned i=0; i<m_BPMSegments.size(); i++ ) // foreach BPMSegment
	{
		const bool bIsLastBPMSegment = i==m_BPMSegments.size()-1;
		const float fBPS = m_BPMSegments[i].m_fBPS;

		if( bIsLastBPMSegment )
		{
			fElapsedTime += NoteRowToBeat( iRow ) / fBPS;
		}
		else
		{
			const int iStartIndexThisSegment = m_BPMSegments[i].m_iStartIndex;
			const int iStartIndexNextSegment = m_BPMSegments[i+1].m_iStartIndex; 
			const int iRowsInThisSegment = min( iStartIndexNextSegment - iStartIndexThisSegment, iRow );
			fElapsedTime += NoteRowToBeat( iRowsInThisSegment ) / fBPS;
			iRow -= iRowsInThisSegment;
		}
		
		if( iRow <= 0 )
			return fElapsedTime;
	}

	return fElapsedTime;
}

void TimingData::ScaleRegion( float fScale, int iStartIndex, int iEndIndex )
{
	ASSERT( fScale > 0 );
	ASSERT( iStartIndex >= 0 );
	ASSERT( iStartIndex < iEndIndex );

	for ( unsigned i = 0; i < m_BPMSegments.size(); i++ )
	{
		const int iSegStart = m_BPMSegments[i].m_iStartIndex;
		if( iSegStart < iStartIndex )
			continue;
		else if( iSegStart > iEndIndex )
			m_BPMSegments[i].m_iStartIndex += lrintf( (iEndIndex - iStartIndex) * (fScale - 1) );
		else
			m_BPMSegments[i].m_iStartIndex = lrintf( (iSegStart - iStartIndex) * fScale ) + iStartIndex;
	}

	for( unsigned i = 0; i < m_StopSegments.size(); i++ )
	{
		const int iSegStartRow = m_StopSegments[i].m_iStartRow;
		if( iSegStartRow < iStartIndex )
			continue;
		else if( iSegStartRow > iEndIndex )
			m_StopSegments[i].m_iStartRow += lrintf((iEndIndex - iStartIndex) * (fScale - 1));
		else
			m_StopSegments[i].m_iStartRow = lrintf((iSegStartRow - iStartIndex) * fScale) + iStartIndex;
	}
}

void TimingData::InsertRows( int iStartRow, int iRowsToAdd )
{
	for( unsigned i = 0; i < m_BPMSegments.size(); i++ )
	{
		BPMSegment &bpm = m_BPMSegments[i];
		if( bpm.m_iStartIndex < iStartRow )
			continue;
		bpm.m_iStartIndex += iRowsToAdd;
	}

	for( unsigned i = 0; i < m_StopSegments.size(); i++ )
	{
		StopSegment &stop = m_StopSegments[i];
		if( stop.m_iStartRow < iStartRow )
			continue;
		stop.m_iStartRow += iRowsToAdd;
	}

	if( iStartRow == 0 )
	{
		/* If we're shifting up at the beginning, we just shifted up the first BPMSegment.  That
		 * segment must always begin at 0. */
		ASSERT( m_BPMSegments.size() > 0 );
		m_BPMSegments[0].m_iStartIndex = 0;
	}
}

/* Delete BPMChanges and StopSegments in [iStartRow,iRowsToDelete), and shift down. */
void TimingData::DeleteRows( int iStartRow, int iRowsToDelete )
{
	/* Remember the BPM at the end of the region being deleted. */
	float fNewBPM = this->GetBPMAtBeat( NoteRowToBeat(iStartRow+iRowsToDelete) );

	/* We're moving rows up.  Delete any BPM changes and stops in the region being
	 * deleted. */
	for( unsigned i = 0; i < m_BPMSegments.size(); i++ )
	{
		BPMSegment &bpm = m_BPMSegments[i];

		/* Before deleted region: */
		if( bpm.m_iStartIndex < iStartRow )
			continue;

		/* Inside deleted region: */
		if( bpm.m_iStartIndex < iStartRow+iRowsToDelete )
		{
			m_BPMSegments.erase( m_BPMSegments.begin()+i, m_BPMSegments.begin()+i+1 );
			--i;
			continue;
		}

		/* After deleted region: */
		bpm.m_iStartIndex -= iRowsToDelete;
	}

	for( unsigned i = 0; i < m_StopSegments.size(); i++ )
	{
		StopSegment &stop = m_StopSegments[i];

		/* Before deleted region: */
		if( stop.m_iStartRow < iStartRow )
			continue;

		/* Inside deleted region: */
		if( stop.m_iStartRow < iStartRow+iRowsToDelete )
		{
			m_StopSegments.erase( m_StopSegments.begin()+i, m_StopSegments.begin()+i+1 );
			--i;
			continue;
		}

		/* After deleted region: */
		stop.m_iStartRow -= iRowsToDelete;
	}

	this->SetBPMAtRow( iStartRow, fNewBPM );
}

bool TimingData::HasBpmChanges() const
{
	return m_BPMSegments.size()>1;
}

bool TimingData::HasStops() const
{
	return m_StopSegments.size()>0;
}

void TimingData::FixNegativeBpmsAndNegativeStops()
{
	// probe to find unreachable segments.
	vector<UnreachableSegment> vUnreachable;
	{
		vector<int> viUnreachableStartRows;
		FOREACH_CONST( BPMSegment, m_BPMSegments, seg )
		{
			if( seg->m_fBPS >= 0 )
				continue;
			viUnreachableStartRows.push_back( seg->m_iStartIndex );
		}
		FOREACH_CONST( StopSegment, m_StopSegments, seg )
		{
			if( seg->m_fStopSeconds >= 0 )
				continue;
			viUnreachableStartRows.push_back( seg->m_iStartRow );
		}

		FOREACH_CONST( int, viUnreachableStartRows, iStartRow )
		{
			// Negative segments are the beginning of a discontinuity.  If we map the start beat of the segment to seconds and back to beats, we'll get the end of the segment.
			float fStartBeat = NoteRowToBeat(*iStartRow);
			float fStartSecs = GetElapsedTimeFromBeatNoOffset( fStartBeat );
			float fEndBeat = GetBeatFromElapsedTime( fStartSecs+0.0001f );
			int iRowEnd = BeatToNoteRow( fEndBeat );

			UnreachableSegment us;
			us.iNoteRowStart = *iStartRow;
			us.iNoteRowEnd = iRowEnd;
			vUnreachable.push_back( us );
		}

		// TODO: collapse overlapping segments
	}

	FOREACH_CONST( UnreachableSegment, vUnreachable, us )
	{
		// save the BPM at the end of the unreachable segment
		float fUnreachableEndBeat = NoteRowToBeat( us->iNoteRowEnd );
		float fAfterUnreachableBPM = this->GetBPMAtBeat(fUnreachableEndBeat);

		// clobber all BPMSegments and StopSegments that lie within the unreachable segment
		for( int i=m_BPMSegments.size()-1; i>=0; i-- )
		{
			bool bStartsInsideUnreachable = m_BPMSegments[i].m_iStartIndex >= us->iNoteRowStart && m_BPMSegments[i].m_iStartIndex < us->iNoteRowEnd;
			if( bStartsInsideUnreachable )
				m_BPMSegments.erase( m_BPMSegments.begin() + i );
		}
		for( int i=m_StopSegments.size()-1; i>=0; i-- )
		{
			bool bInsideUnreachable = m_StopSegments[i].m_iStartRow >= us->iNoteRowStart && m_StopSegments[i].m_iStartRow < us->iNoteRowEnd;
			if( bInsideUnreachable )
				m_StopSegments.erase( m_StopSegments.begin() + i );
		}

		// insert the infinite BPM segment to warp
		{
			BPMSegment seg;
			seg.m_iStartIndex = us->iNoteRowStart;
			seg.m_fBPS = BPMSegment::INFINITE_BPS;
			this->AddBPMSegment( seg );
		}

		// set the BPM at the end of the unreachable segment to stop the warp
		{
			BPMSegment seg;
			seg.m_iStartIndex = BeatToNoteRow(fUnreachableEndBeat);
			seg.m_fBPS = fAfterUnreachableBPM / 60;
			this->AddBPMSegment( seg );
		}
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