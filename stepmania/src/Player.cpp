#include "stdafx.h"
/*
-----------------------------------------------------------------------------
 Class: Player

 Desc: See header.

 Copyright (c) 2001-2002 by the person(s) listed below.  All rights reserved.
	Chris Danford
-----------------------------------------------------------------------------
*/

#include "GameConstantsAndTypes.h"
#include "Math.h" // for fabs()
#include "Player.h"
#include "RageUtil.h"
#include "PrefsManager.h"
#include "GameConstantsAndTypes.h"
#include "ArrowEffects.h"
#include "GameManager.h"
#include "InputMapper.h"
#include "SongManager.h"
#include "GameState.h"
#include "RageLog.h"


// these two items are in the
const float FRAME_JUDGE_AND_COMBO_Y = CENTER_Y;
const float JUDGEMENT_Y_OFFSET	= -26;
const float COMBO_Y_OFFSET		= +26;

const float FRAME_JUDGE_AND_COMBO_BEAT_TIME = 0.2f;

const float ARROWS_Y			= SCREEN_TOP + ARROW_SIZE * 1.5f;
const float HOLD_JUDGEMENT_Y	= ARROWS_Y + 80;

const float HOLD_ARROW_NG_TIME	=	0.18f;


Player::Player()
{
	m_PlayerNumber = PLAYER_INVALID;

	m_pLifeMeter = NULL;
	m_pScore = NULL;

	this->AddSubActor( &m_GrayArrowRow );
	this->AddSubActor( &m_NoteField );
	this->AddSubActor( &m_GhostArrowRow );

	m_frameJudgement.AddSubActor( &m_Judgement );
	this->AddSubActor( &m_frameJudgement );

	m_frameCombo.AddSubActor( &m_Combo );
	this->AddSubActor( &m_frameCombo );
	
	for( int c=0; c<MAX_NOTE_TRACKS; c++ )
		this->AddSubActor( &m_HoldJudgement[c] );
}


void Player::Load( PlayerNumber pn, NoteData* pNoteData, LifeMeter* pLM, ScoreDisplay* pScore )
{
	//LOG->Trace( "Player::Load()", );

	m_PlayerNumber = pn;
	m_pLifeMeter = pLM;
	m_pScore = pScore;

	StyleDef* pStyleDef = GAMESTATE->GetCurrentStyleDef();

	// copy note data
	this->CopyAll( pNoteData );


	m_iNumTapNotes = pNoteData->GetNumTapNotes();
	m_iTapNotesHit = 0;
	m_iMeter = GAMESTATE->m_pCurNotes[m_PlayerNumber]->m_iMeter;

	// init scoring
	NoteDataWithScoring::Init();
	for( int i=0; i<MAX_TAP_NOTE_ROWS; i++ )
		m_fHoldNoteLife[i] = 1.0f;


	if( m_pScore )
		m_pScore->Init( pn );

	if( !GAMESTATE->m_PlayerOptions[pn].m_bHoldNotes )
		this->RemoveHoldNotes();

	this->Turn( GAMESTATE->m_PlayerOptions[pn].m_TurnType );

	if( GAMESTATE->m_PlayerOptions[pn].m_bLittle )
		this->MakeLittle();

	int iPixelsToDrawBefore = 64;
	int iPixelsToDrawAfter = 320;
	switch( GAMESTATE->m_PlayerOptions[pn].m_EffectType )
	{
	case PlayerOptions::EFFECT_MINI:	iPixelsToDrawBefore *= 2;	iPixelsToDrawAfter *= 2;	break;
	case PlayerOptions::EFFECT_SPACE:	iPixelsToDrawBefore *= 2;	iPixelsToDrawAfter *= 2;	break;
	}

	m_NoteField.Load( (NoteData*)this, pn, iPixelsToDrawBefore, iPixelsToDrawAfter );
	
	m_GrayArrowRow.Load( pn );
	m_GhostArrowRow.Load( pn );

	m_frameJudgement.SetY( FRAME_JUDGE_AND_COMBO_Y );
	m_frameCombo.SetY( FRAME_JUDGE_AND_COMBO_Y );
	m_Combo.SetY( GAMESTATE->m_PlayerOptions[pn].m_bReverseScroll ?  -COMBO_Y_OFFSET : COMBO_Y_OFFSET );
	m_Judgement.SetY( GAMESTATE->m_PlayerOptions[pn].m_bReverseScroll ? -JUDGEMENT_Y_OFFSET : JUDGEMENT_Y_OFFSET );

	for( int c=0; c<pStyleDef->m_iColsPerPlayer; c++ )
		m_HoldJudgement[c].SetY( GAMESTATE->m_PlayerOptions[pn].m_bReverseScroll ? SCREEN_HEIGHT - HOLD_JUDGEMENT_Y : HOLD_JUDGEMENT_Y );
	for( c=0; c<pStyleDef->m_iColsPerPlayer; c++ )
		m_HoldJudgement[c].SetX( (float)pStyleDef->m_ColumnInfo[pn][c].fXOffset );

	m_NoteField.SetY( GAMESTATE->m_PlayerOptions[pn].m_bReverseScroll ? SCREEN_HEIGHT - ARROWS_Y : ARROWS_Y );
	m_GrayArrowRow.SetY( GAMESTATE->m_PlayerOptions[pn].m_bReverseScroll ? SCREEN_HEIGHT - ARROWS_Y : ARROWS_Y );
	m_GhostArrowRow.SetY( GAMESTATE->m_PlayerOptions[pn].m_bReverseScroll ? SCREEN_HEIGHT - ARROWS_Y : ARROWS_Y );

	if( GAMESTATE->m_PlayerOptions[pn].m_EffectType == PlayerOptions::EFFECT_MINI )
	{
		m_NoteField.SetZoom( 0.5f );
		m_GrayArrowRow.SetZoom( 0.5f );
		m_GhostArrowRow.SetZoom( 0.5f );
	}
}

void Player::Update( float fDeltaTime )
{
	//LOG->Trace( "Player::Update(%f)", fDeltaTime );

	const float fSongBeat = GAMESTATE->m_fSongBeat;

	//
	// Check for TapNote misses
	//
	UpdateTapNotesMissedOlderThan( GAMESTATE->m_fSongBeat - GetMaxBeatDifference() );


	//
	// update HoldNotes logic
	//
	for( int i=0; i<m_iNumHoldNotes; i++ )		// for each HoldNote
	{
		HoldNote &hn = m_HoldNotes[i];
		HoldNoteScore &hns = m_HoldNoteScores[i];
		float &fLife = m_fHoldNoteLife[i];

		if( hns != HNS_NONE )	// if this HoldNote already has a result
			continue;	// we don't need to update the logic for this one

		float fStartBeat = NoteRowToBeat( (float)hn.m_iStartIndex );
		float fEndBeat = NoteRowToBeat( (float)hn.m_iEndIndex );

		const StyleInput StyleI( m_PlayerNumber, hn.m_iTrack );
		const GameInput GameI = GAMESTATE->GetCurrentStyleDef()->StyleInputToGameInput( StyleI );


		// update the life
		if( fStartBeat < fSongBeat && fSongBeat < fEndBeat )	// if the song beat is in the range of this hold
		{
			const bool bIsHoldingButton = INPUTMAPPER->IsButtonDown( GameI )  ||  PREFSMAN->m_bAutoPlay;
			// if they got a bad score or haven't stepped on the corresponding tap yet
			const bool bSteppedOnTapNote = m_TapNoteScores[hn.m_iTrack][hn.m_iStartIndex] >= TNS_GREAT;


			if( bIsHoldingButton && bSteppedOnTapNote )
			{
				// Increase life
				fLife += fDeltaTime/HOLD_ARROW_NG_TIME;
				fLife = min( fLife, 1 );	// clamp

				m_NoteField.m_HoldNotes[i].m_iStartIndex = BeatToNoteRow( fSongBeat );	// move the start of this Hold

				m_GhostArrowRow.HoldNote( hn.m_iTrack );		// update the "electric ghost" effect
			}
			else	// !bIsHoldingButton
			{
				if( fSongBeat-fStartBeat > GetMaxBeatDifference() )
				{
					// Decrease life
					fLife -= fDeltaTime/HOLD_ARROW_NG_TIME;
					fLife = max( fLife, 0 );	// clamp
				}
			}
			m_NoteField.SetHoldNoteLife( i, fLife );	// update the NoteField display
		}

		// check for NG
		if( fLife == 0 )	// the player has not pressed the button for a long time!
		{
			hns = HNS_NG;
			HandleNoteScore( hns );
			m_Combo.EndCombo();
			m_HoldJudgement[hn.m_iTrack].SetHoldJudgement( HNS_NG );
		}

		// check for OK
		if( fSongBeat > fEndBeat )	// if this HoldNote is in the past
		{
			// At this point fLife > 0, or else we would have marked it NG above
			fLife = 1;
			hns = HNS_OK;
			HandleNoteScore( hns );
			m_HoldJudgement[hn.m_iTrack].SetHoldJudgement( HNS_OK );
			m_NoteField.SetHoldNoteLife( i, fLife );	// update the NoteField display
		}
	}



	ActorFrame::Update( fDeltaTime );
}

void Player::DrawPrimitives()
{
	m_frameCombo.Draw();	// draw this below everything else

	D3DXMATRIX matOldView, matOldProj;

	if( GAMESTATE->m_PlayerOptions[m_PlayerNumber].m_EffectType == PlayerOptions::EFFECT_SPACE )
	{
		// save old view and projection
		DISPLAY->GetDevice()->GetTransform( D3DTS_VIEW, &matOldView );
		DISPLAY->GetDevice()->GetTransform( D3DTS_PROJECTION, &matOldProj );


		// construct view and project matrix
		D3DXMATRIX matNewView;
		if( GAMESTATE->m_PlayerOptions[m_PlayerNumber].m_bReverseScroll )
			D3DXMatrixLookAtLH( 
				&matNewView, 
				&D3DXVECTOR3( CENTER_X, GetY()-300.0f, 400.0f ),
				&D3DXVECTOR3( CENTER_X, GetY()+100.0f, 0.0f ), 
				&D3DXVECTOR3( 0.0f,     -1.0f,           0.0f ) 
				);
		else
			D3DXMatrixLookAtLH( 
				&matNewView, 
				&D3DXVECTOR3( CENTER_X, GetY()+800.0f, 400.0f ),
				&D3DXVECTOR3( CENTER_X, GetY()+400.0f, 0.0f ), 
				&D3DXVECTOR3( 0.0f,     -1.0f,           0.0f ) 
				);

		DISPLAY->GetDevice()->SetTransform( D3DTS_VIEW, &matNewView );

		D3DXMATRIX matNewProj;
		D3DXMatrixPerspectiveFovLH( &matNewProj, D3DX_PI/4.0f, SCREEN_WIDTH/(float)SCREEN_HEIGHT, 0.0f, 1000.0f );
		DISPLAY->GetDevice()->SetTransform( D3DTS_PROJECTION, &matNewProj );
	}

	m_GrayArrowRow.Draw();
	m_NoteField.Draw();
	m_GhostArrowRow.Draw();

	if( GAMESTATE->m_PlayerOptions[m_PlayerNumber].m_EffectType == PlayerOptions::EFFECT_SPACE )
	{
		// restire old view and projection
		DISPLAY->GetDevice()->SetTransform( D3DTS_VIEW, &matOldView );
		DISPLAY->GetDevice()->SetTransform( D3DTS_PROJECTION, &matOldProj );
	}

	m_frameJudgement.Draw();

	for( int c=0; c<m_iNumTracks; c++ )
		m_HoldJudgement[c].Draw();
}

void Player::Step( int col )
{
	//LOG->Trace( "Player::HandlePlayerStep()" );

	ASSERT( col >= 0  &&  col <= m_iNumTracks );

	const float fSongBeat = GAMESTATE->m_fSongBeat;

	// look for the closest matching step
	int iIndexStartLookingAt = BeatToNoteRow( GAMESTATE->m_fSongBeat );
	int iNumElementsToExamine = BeatToNoteRow( GetMaxBeatDifference() );	// number of elements to examine on either end of iIndexStartLookingAt
	
	//LOG->Trace( "iIndexStartLookingAt = %d, iNumElementsToExamine = %d", iIndexStartLookingAt, iNumElementsToExamine );

	int iIndexOverlappingNote = -1;		// leave as -1 if we don't find any

	// Start at iIndexStartLookingAt and search outward.  The first one note overlaps the player's hit (this is the closest match).
	for( int delta=0; delta <= iNumElementsToExamine; delta++ )
	{
		int iCurrentIndexEarlier = iIndexStartLookingAt - delta;
		int iCurrentIndexLater   = iIndexStartLookingAt + delta;

		// silly check to make sure we don't go out of bounds
		iCurrentIndexEarlier	= clamp( iCurrentIndexEarlier, 0, MAX_TAP_NOTE_ROWS-1 );
		iCurrentIndexLater		= clamp( iCurrentIndexLater,   0, MAX_TAP_NOTE_ROWS-1 );

		////////////////////////////
		// check the step to the left of iIndexStartLookingAt
		////////////////////////////
		//LOG->Trace( "Checking Notes[%d]", iCurrentIndexEarlier );
		if( m_TapNotes[col][iCurrentIndexEarlier] != '0'  &&	// there is a note here
			m_TapNoteScores[col][iCurrentIndexEarlier] == TNS_NONE )	// this note doesn't have a score
		{
			iIndexOverlappingNote = iCurrentIndexEarlier;
			break;
		}


		////////////////////////////
		// check the step to the right of iIndexStartLookingAt
		////////////////////////////
		//LOG->Trace( "Checking Notes[%d]", iCurrentIndexLater );
		if( m_TapNotes[col][iCurrentIndexLater] != '0'  &&	// there is a note here
			m_TapNoteScores[col][iCurrentIndexLater] == TNS_NONE )	// this note doesn't have a score
		{
			iIndexOverlappingNote = iCurrentIndexLater;
			break;
		}
	}

	bool bDestroyedNote = false;

	if( iIndexOverlappingNote != -1 )
	{
		// compute the score for this hit
		const float fStepBeat = NoteRowToBeat( (float)iIndexOverlappingNote );
		const float fBeatsUntilStep = fStepBeat - fSongBeat;
		const float fPercentFromPerfect = fabsf( fBeatsUntilStep / GetMaxBeatDifference() );

		TapNoteScore &score = m_TapNoteScores[col][iIndexOverlappingNote];

		if(		 fPercentFromPerfect < 0.25f )	score = TNS_PERFECT;
		else if( fPercentFromPerfect < 0.50f )	score = TNS_GREAT;
		else if( fPercentFromPerfect < 0.75f )	score = TNS_GOOD;
		else									score = TNS_BOO;

		bDestroyedNote = score >= TNS_GOOD;


		bool bRowDestroyed = true;
		for( int t=0; t<m_iNumTracks; t++ )			// did this complete the elminiation of the row?
		{
			if( m_TapNotes[t][iIndexOverlappingNote] != '0'  &&			// there is a note here
				m_TapNoteScores[t][iIndexOverlappingNote] == TNS_NONE )	// and it doesn't have a score
			{
				bRowDestroyed = false;
				break;	// stop searching
			}
		}
		if( bRowDestroyed )
			OnRowDestroyed( col, iIndexOverlappingNote );
	}

	if( !bDestroyedNote )
		m_GrayArrowRow.Step( col );
}

void Player::OnRowDestroyed( int col, int iIndexThatWasSteppedOn )
{
	//LOG->Trace( "fBeatsUntilStep: %f, fPercentFromPerfect: %f", 
	//		 fBeatsUntilStep, fPercentFromPerfect );
	
	// find the minimum score of the row
	TapNoteScore score = TNS_PERFECT;
	for( int t=0; t<m_iNumTracks; t++ )
		if( m_TapNoteScores[t][iIndexThatWasSteppedOn] >= TNS_BOO )
			score = min( score, m_TapNoteScores[t][iIndexThatWasSteppedOn] );

	// remove this row from the NoteField
	if ( ( score == TNS_PERFECT ) || ( score == TNS_GREAT ) )
		m_NoteField.RemoveTapNoteRow( iIndexThatWasSteppedOn );

	for( int c=0; c<m_iNumTracks; c++ )	// for each column
	{
		if( m_TapNotes[c][iIndexThatWasSteppedOn] != '0' )	// if there is a note in this col
		{
			m_GhostArrowRow.TapNote( c, score, m_Combo.GetCurrentCombo()>100 );	// show the ghost arrow for this column
			
			HandleNoteScore( score );	// update score - called once per note in this row

			// update combo - called once per note in this row
			switch( score )
			{
			case TNS_PERFECT:
			case TNS_GREAT:
				m_Combo.ContinueCombo();
				GAMESTATE->m_iMaxCombo[m_PlayerNumber] = max( GAMESTATE->m_iMaxCombo[m_PlayerNumber], m_Combo.GetCurrentCombo() );
				break;
			case TNS_GOOD:
			case TNS_BOO:
				m_Combo.EndCombo();
				break;
			}
		}
	}

	// update the judgement, score, and life
	m_Judgement.SetJudgement( score );
	if( m_pLifeMeter )
		m_pLifeMeter->ChangeLife( score );

	// zoom the judgement and combo like a heart beat
	float fStartZoom;
	switch( score )
	{
	case TNS_PERFECT:	fStartZoom = 1.3f;	break;
	case TNS_GREAT:		fStartZoom = 1.2f;	break;
	case TNS_GOOD:		fStartZoom = 1.1f;	break;
	case TNS_BOO:		fStartZoom = 1.0f;	break;
	}
	m_frameJudgement.SetZoom( fStartZoom );
	m_frameJudgement.BeginTweening( 0.1f );
	m_frameJudgement.SetTweenZoom( 1 );

	m_frameCombo.SetZoom( fStartZoom );
	m_frameCombo.BeginTweening( 0.1f );
	m_frameCombo.SetTweenZoom( 1 );
}


int Player::UpdateTapNotesMissedOlderThan( float fMissIfOlderThanThisBeat )
{
	//LOG->Trace( "Notes::UpdateTapNotesMissedOlderThan(%f)", fMissIfOlderThanThisBeat );

	int iMissIfOlderThanThisIndex = BeatToNoteRow( fMissIfOlderThanThisBeat );

	int iNumMissesFound = 0;
	// Since this is being called every frame, let's not check the whole array every time.
	// Instead, only check 10 elements back.  Even 10 is overkill.
	int iStartCheckingAt = max( 0, iMissIfOlderThanThisIndex-10 );

	//LOG->Trace( "iStartCheckingAt: %d   iMissIfOlderThanThisIndex:  %d", iStartCheckingAt, iMissIfOlderThanThisIndex );
	for( int t=0; t<m_iNumTracks; t++ )
	{
		for( int r=iStartCheckingAt; r<iMissIfOlderThanThisIndex; r++ )
		{
			bool bFoundAMissInThisRow = false;
			if( m_TapNotes[t][r] != '0'  &&  m_TapNoteScores[t][r] == TNS_NONE )
			{
				m_TapNoteScores[t][r] = TNS_MISS;
				iNumMissesFound++;
				bFoundAMissInThisRow = true;
				HandleNoteScore( TNS_MISS );
			}
			if( bFoundAMissInThisRow )
				if( m_pLifeMeter )
					m_pLifeMeter->ChangeLife( TNS_MISS );
		}
	}

	if( iNumMissesFound > 0 )
	{
		m_Judgement.SetJudgement( TNS_MISS );
		m_Combo.EndCombo();
	}

	return iNumMissesFound;
}


void Player::CrossedRow( int iNoteRow )
{
	if( PREFSMAN->m_bAutoPlay )
	{
		// check to see if there's at the crossed row
		for( int t=0; t<m_iNumTracks; t++ )
		{
			if( m_TapNotes[t][iNoteRow] != '0' )
				this->Step( t );
		}
	}
}



void Player::HandleNoteScore( TapNoteScore score )
{
	// update dance points for Oni lifemeter
	GAMESTATE->m_iActualDancePoints[m_PlayerNumber] += TapNoteScoreToDancePoints( score );
	GAMESTATE->m_TapNoteScores[m_PlayerNumber][score]++;
	if( m_pLifeMeter )
		m_pLifeMeter->OnDancePointsChange();


//A single step's points are calculated as follows: 
//
//Let p = score multiplier (Perfect = 10, Great = 5, other = 0)
//N = total number of steps and freeze steps
//n = number of the current step or freeze step (varies from 1 to N)
//B = Base value of the song (1,000,000 X the number of feet difficulty) - All edit data is rated as 5 feet
//So, the score for one step is: 
//one_step_score = p * (B/S) * n 
//Where S = The sum of all integers from 1 to N (the total number of steps/freeze steps) 
//
//*IMPORTANT* : Double steps (U+L, D+R, etc.) count as two steps instead of one, so if you get a double L+R on the 112th step of a song, you score is calculated with a Perfect/Great/whatever for both the 112th and 113th steps. Got it? Now, through simple algebraic manipulation 
//S = 1+...+N = (1+N)*N/2 (1 through N added together) 
//Okay, time for an example: 
//
//So, for example, suppose we wanted to calculate the step score of a "Great" on the 57th step of a 441 step, 8-foot difficulty song (I'm just making this one up): 
//
//S = (1 + 441)*441 / 2
//= 194,222 / 2
//= 97,461
//StepScore = p * (B/S) * n
//= 5 * (8,000,000 / 97,461) * 57
//= 5 * (82) * 57 (The 82 is rounded down from 82.08411...)
//= 23,370
//Remember this is just the score for the step, not the cumulative score up to the 57th step. Also, please note that I am currently checking into rounding errors with the system and if there are any, how they are resolved in the system. 
//
//Note: if you got all Perfect on this song, you would get (p=10)*B, which is 80,000,000. In fact, the maximum possible score for any song is the number of feet difficulty X 10,000,000. 

	int p;	// score multiplier 
	switch( score )
	{
	case TNS_PERFECT:	p = 10;		break;
	case TNS_GREAT:		p = 5;		break;
	default:			p = 0;		break;
	}
	
	int N = m_iNumTapNotes;
	int n = m_iTapNotesHit+1;
	int B = m_iMeter * 1000000;
	float S = (1+N)*N/2.0f;

	printf( "m_iNumTapNotes %d, m_iTapNotesHit %d\n", m_iNumTapNotes, m_iTapNotesHit );

	float one_step_score = p * (B/S) * n;

	float& fScore = GAMESTATE->m_fScore[m_PlayerNumber];
	ASSERT( fScore >= 0 );

	fScore += one_step_score;

	m_iTapNotesHit++;
	ASSERT( m_iTapNotesHit <= m_iNumTapNotes );

	// HACK:  Correct for rounding errors that cause a 100% perfect score to be slightly off
	if( m_iTapNotesHit == m_iNumTapNotes  &&  
		fabsf( fScore - froundf(fScore,1000000) ) < 50.0f )	// close to a multiple of 1,000,000
		fScore = froundf(fScore,1000000);

	if( m_pScore )
		m_pScore->SetScore( fScore );
}

void Player::HandleNoteScore( HoldNoteScore score )
{
	// update dance points for Oni lifemeter
	GAMESTATE->m_iActualDancePoints[m_PlayerNumber] += HoldNoteScoreToDancePoints( score );
	GAMESTATE->m_HoldNoteScores[m_PlayerNumber][score]++;
	if( m_pLifeMeter )
		m_pLifeMeter->OnDancePointsChange();

	// HoldNoteScores don't effect m_fScore
}
float Player::GetMaxBeatDifference()
{
	return GAMESTATE->m_fCurBPS * PREFSMAN->m_fJudgeWindow * GAMESTATE->m_SongOptions.m_fMusicRate;
}