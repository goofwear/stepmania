#include "global.h"

#include "ActorUtil.h"
#include "BeginnerHelper.h"
#include "GameState.h"
#include "PrefsManager.h"
#include "RageLog.h"
#include "RageDisplay.h"
#include "ThemeManager.h"
#include "Steps.h"

// "PLAYER_X" offsets are relative to the pad.. ex: Setting this to 10, and the HELPER to 300, will put the dancer at 310
#define PLAYER_X( px )		THEME->GetMetricF("BeginnerHelper",ssprintf("Player%d_X",px+1))
#define PLAYER_ANGLE		THEME->GetMetricF("BeginnerHelper","PlayerAngle")
#define DANCEPAD_ANGLE		THEME->GetMetricF("BeginnerHelper","DancePadAngle")

// STEPCIRCLE commands are relative to the HELPER position
#define STEPCIRCLE_LEFT_COMMAND( pz )	THEME->GetMetric ("BeginnerHelper",ssprintf("StepCircleP%d_LEFT_COMMAND",pz+1))
#define STEPCIRCLE_DOWN_COMMAND( pz )	THEME->GetMetric ("BeginnerHelper",ssprintf("StepCircleP%d_DOWN_COMMAND",pz+1))
#define STEPCIRCLE_UP_COMMAND( pz )	THEME->GetMetric ("BeginnerHelper",ssprintf("StepCircleP%d_UP_COMMAND",pz+1))
#define STEPCIRCLE_RIGHT_COMMAND( pz )	THEME->GetMetric ("BeginnerHelper",ssprintf("StepCircleP%d_RIGHT_COMMAND",pz+1))

// "HELPER" offsets effect the pad/dancer as a whole.. Their relative Y cooridinates are hard-coded for eachother.
#define HELPER_X			THEME->GetMetricF("BeginnerHelper","HelperX")
#define HELPER_Y			THEME->GetMetricF("BeginnerHelper","HelperY")


BeginnerHelper::BeginnerHelper()
{
	LOG->Trace("BeginnerHelper::BeginnerHelper()");
	m_bFlashEnabled = false;
	m_bInitialized = false;
	m_fLastBeatChecked = 0;
	this->AddChild(&m_sBackground);
}

BeginnerHelper::~BeginnerHelper()
{
	LOG->Trace("BeginnerHelper::~BeginnerHelper()");
}

void BeginnerHelper::ShowStepCircle( int pn, int CSTEP )
{
	/*for( int ps=0; ps<NUM_PLAYERS; ps++ )
	{
		m_sStepCircle[ps].SetXY( (HELPER_X + PLAYER_X(ps)), (HELPER_Y + 10) );
		switch( CSTEP )
		{
			case ST_LEFT: m_sStepCircle[ps].SetRotationZ(270); break;
			case ST_DOWN: m_sStepCircle[ps].SetRotationZ(180); break;
			case ST_UP: break;
			case ST_RIGHT: m_sStepCircle[ps].SetRotationZ(90); break;
			default: break;
		}
		
		m_sStepCircle[ps].SetZoom(1);
		m_sStepCircle[ps].SetEffectNone();
		m_sStepCircle[ps].StopTweening();
		m_sStepCircle[ps].BeginTweening((GAMESTATE->m_fCurBPS/4), TWEEN_LINEAR);
		m_sStepCircle[ps].SetZoom(0);
	}*/
}

void BeginnerHelper::SetFlash( CString sFilename, float fX, float fY )
{
	m_sFlash.Load( sFilename );
	m_sFlash.SetX(fX);
	m_sFlash.SetY(fY);
}

void BeginnerHelper::AddPlayer( int pn, NoteData *pNotes )
{
	ASSERT( !m_bInitialized );
	ASSERT( pNotes != NULL );
	ASSERT( pn >= 0 && pn < NUM_PLAYERS);

	m_NoteData[pn].CopyAll( pNotes );
}

bool BeginnerHelper::CanUse()
{
	return ((GAMESTATE->m_CurGame == GAME_DANCE) &&
			(DoesFileExist("Characters" SLASH "DancePad-DDR.txt")) &&
			(DoesFileExist("Characters" SLASH "DancePads-DDR.txt")) );
}

bool BeginnerHelper::Initialize( int iDancePadType )
{
	ASSERT( !m_bInitialized );

	if (!CanUse())		// if we can't be used, bail now.
		return false;

	// Load the StepCircle, Background, and flash animation
	m_sBackground.Load( THEME->GetPathToG("BeginnerHelper background") );
	m_sBackground.SetXY( CENTER_X, CENTER_Y);
	
	m_sFlash.Load( THEME->GetPathToG("BeginnerHelper flash") );
	m_sFlash.SetXY( CENTER_X, CENTER_Y );
	m_sFlash.SetDiffuseAlpha(0);

	/*for( int sc=0; sc<NUM_PLAYERS*2; sc++ )
	{
		m_sStepCircle[sc].Load( THEME->GetPathToG("BeginnerHelper stepcircle") );
		m_sStepCircle[sc].SetZoom(0);	// Hide until needed.
		this->AddChild(&m_sStepCircle[sc]);
	}*/

	// Load the DancePad
	switch(iDancePadType)
	{
		case 0: break; // No pad
		case 1:
			if (!DoesFileExist("Characters" SLASH "DancePad-DDR.txt") )
				return false;		// can't initialize without the required pad model. bail
			m_mDancePad.LoadMilkshapeAscii( "Characters" SLASH "DancePad-DDR.txt" );
			break;
		case 2:
			if (!DoesFileExist("Characters" SLASH "DancePads-DDR.txt") )
				return false;		// can't initialize without the required pad model. bail
			m_mDancePad.LoadMilkshapeAscii( "Characters" SLASH "DancePads-DDR.txt" );
			break;
	}
	m_mDancePad.SetHorizAlign( align_left );
	m_mDancePad.SetRotationX( DANCEPAD_ANGLE );
	m_mDancePad.SetX(HELPER_X);
	m_mDancePad.SetY(HELPER_Y);
	m_mDancePad.SetZoom( 23 );	// Pad should always be 3 units bigger in zoom than the dancer.

	for( int pl=0; pl<NUM_PLAYERS; pl++ )	// Load players
	{
		if( !GAMESTATE->IsHumanPlayer(pl))
			continue;

		// if there is no character set, try loading a random one.
		if( GAMESTATE->m_pCurCharacters[pl] == NULL )
		{
			GAMESTATE->m_pCurCharacters[pl] = GAMESTATE->GetRandomCharacter();
		}

		if( GAMESTATE->m_pCurNotes[pl]->GetDifficulty() == DIFFICULTY_BEGINNER && GAMESTATE->m_pCurCharacters[pl] != NULL )
		{
			// Load textures
			m_mDancer[pl].SetHorizAlign( align_left );
			m_mDancer[pl].LoadMilkshapeAscii( GAMESTATE->m_pCurCharacters[pl]->GetModelPath() );

			// Load needed animations
			m_mDancer[pl].LoadMilkshapeAsciiBones( "Step-LEFT","Characters" SLASH "BeginnerHelper_step-left.bones.txt" );
			m_mDancer[pl].LoadMilkshapeAsciiBones( "Step-DOWN","Characters" SLASH "BeginnerHelper_step-down.bones.txt" );
			m_mDancer[pl].LoadMilkshapeAsciiBones( "Step-UP","Characters" SLASH "BeginnerHelper_step-up.bones.txt" );
			m_mDancer[pl].LoadMilkshapeAsciiBones( "Step-RIGHT","Characters" SLASH "BeginnerHelper_step-right.bones.txt" );
			m_mDancer[pl].LoadMilkshapeAsciiBones( "Step-JUMPLR","Characters" SLASH "BeginnerHelper_step-jumplr.bones.txt" );
			/*m_mDancer[pl].LoadMilkshapeAsciiBones( "Step-JUMPUD","Characters\\BeginnerHelper_step-jumpud.bones.txt" );*/
			m_mDancer[pl].LoadMilkshapeAsciiBones( "rest",GAMESTATE->m_pCurCharacters[0]->GetRestAnimationPath() );
			m_mDancer[pl].SetDefaultAnimation( "rest" );
			m_mDancer[pl].PlayAnimation( "rest" );
			m_mDancer[pl].SetRotationX( PLAYER_ANGLE );
			m_mDancer[pl].SetX( HELPER_X + PLAYER_X(pl) );
			m_mDancer[pl].SetY( HELPER_Y + 10 );
			m_mDancer[pl].SetZoom( 20 );
			m_mDancer[pl].m_bRevertToDefaultAnimation = true;		// Stay bouncing after a step has finished animating.
		}
	}

	return (m_bInitialized = true);
}

void BeginnerHelper::FlashOnce()
{
	m_sFlash.SetDiffuseAlpha(1);
	m_sFlash.SetEffectNone();
	m_sFlash.StopTweening();
	m_sFlash.BeginTweening( 1/GAMESTATE->m_fCurBPS * 0.5);
	m_sFlash.SetDiffuseAlpha(0);
}

void BeginnerHelper::DrawPrimitives()
{
	if(!m_bInitialized)
		return;

	ActorFrame::DrawPrimitives();

	m_sFlash.Draw();

	DISPLAY->SetLighting( true );
	DISPLAY->SetLightDirectional( 
		0, 
		RageColor(0.5,0.5,0.5,1), 
		RageColor(1,1,1,1),
		RageColor(0,0,0,1),
		RageVector3(0, 0, 1) );

	m_mDancePad.Draw();
	for( int scd=0; scd<NUM_PLAYERS*2; scd++ )
		m_sStepCircle[scd].Draw();		// Should be drawn before the dancer, but after the pad, so it is drawn over the pad and under the dancer.
	
	for( int pn=0; pn<NUM_PLAYERS; pn++ )	// Draw each dancer
		if( GAMESTATE->IsHumanPlayer(pn) )
			m_mDancer[pn].Draw();

	DISPLAY->SetLightOff( 0 );
	DISPLAY->SetLighting( false );
}

void BeginnerHelper::Step( int pn, int CSTEP )
{
	LOG->Trace( "BeginnerHelper::Step()" );
	// First make sure this player is on beginner mode and enabled... The difficulty check may be redundant, tho.
	if( (GAMESTATE->IsHumanPlayer(pn)) && (GAMESTATE->m_pCurNotes[pn]->GetDifficulty() == DIFFICULTY_BEGINNER) )
	{
		ShowStepCircle( pn, CSTEP);
		m_mDancer[pn].StopTweening();
		m_mDancer[pn].SetRotationY(0);	// Make sure we're not still inside of a JUMPUD tween.
		switch( CSTEP )
		{
		case ST_LEFT:	m_mDancer[pn].PlayAnimation( "Step-LEFT" ); break;
		case ST_RIGHT:	m_mDancer[pn].PlayAnimation( "Step-RIGHT" ); break;
		case ST_UP:	m_mDancer[pn].PlayAnimation( "Step-UP" ); break;
		case ST_DOWN:	m_mDancer[pn].PlayAnimation( "Step-DOWN" ); break;
		case ST_JUMPLR: m_mDancer[pn].PlayAnimation( "Step-JUMPLR" ); break;
		case ST_JUMPUD:
			// Until I can get an UP+DOWN jump animation, this will have to do.
			m_mDancer[pn].PlayAnimation( "Step-JUMPLR" );
			
			m_mDancer[pn].StopTweening();
			m_mDancer[pn].BeginTweening( GAMESTATE->m_fCurBPS /8, TWEEN_LINEAR );
			m_mDancer[pn].SetRotationY( 90 );
			m_mDancer[pn].BeginTweening( (1/GAMESTATE->m_fCurBPS) ); //sleep between jump-frames
			m_mDancer[pn].BeginTweening( GAMESTATE->m_fCurBPS /6, TWEEN_LINEAR );
			m_mDancer[pn].SetRotationY( 0 );
			break;
		}
	}
}

void BeginnerHelper::Update( float fDeltaTime )
{
	if(!m_bInitialized)
		return;

	// the beat we want to check on this update
	float fCurBeat = GAMESTATE->m_fSongBeat + 0.5f;

	for(int pn = 0; pn < NUM_PLAYERS; pn++ )
	{
		if( !( GAMESTATE->IsHumanPlayer(pn) && GAMESTATE->m_pCurNotes[pn]->GetDifficulty() == DIFFICULTY_BEGINNER) )
			continue;	// skip

		if( (m_NoteData[pn].IsThereATapAtRow( BeatToNoteRowNotRounded((GAMESTATE->m_fSongBeat+0.01f)) ) ) )
			FlashOnce();
		for(float fBeat=m_fLastBeatChecked; fBeat<fCurBeat; fBeat+=0.01f)
		{
			if((m_NoteData[pn].IsThereATapAtRow( BeatToNoteRowNotRounded(fBeat))))
			{
				int iStep = 0;
				for( int k=0; k<m_NoteData[pn].GetNumTracks(); k++ )
					if( m_NoteData[pn].GetTapNote(k, BeatToNoteRowNotRounded(fBeat) ) == TAP_TAP )
					{
						switch(k)
						{
						case 0: iStep += 6; break;
						case 1: iStep += 3; break;
						case 2: iStep += 8; break;
						case 3: iStep += 4; break;
						};
					}
				Step( pn, iStep );
			}
		}
	}
	m_fLastBeatChecked = fCurBeat;

	ActorFrame::Update( fDeltaTime );
	m_mDancePad.Update( fDeltaTime );
	m_sFlash.Update( fDeltaTime );

	float beat = fDeltaTime * GAMESTATE->m_fCurBPS;

	for( int pu=0; pu<NUM_PLAYERS; pu++ )
	{
		if(!GAMESTATE->IsHumanPlayer(pu))
			continue;

		m_mDancer[pu].Update( beat );	//Update dancers
		for( int scu=0; scu<2; scu++ )
			m_sStepCircle[pu+scu].Update( beat );
	}
}
