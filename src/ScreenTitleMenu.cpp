#include "global.h"
#include "ScreenTitleMenu.h"
#include "ScreenManager.h"
#include "RageUtil.h"
#include "StepMania.h"
#include "PrefsManager.h"
#include "RageLog.h"
#include "AnnouncerManager.h"
#include "GameState.h"
#include "GameManager.h"
#include "ThemeManager.h"
#include "GameSoundManager.h"
#include "CodeDetector.h"
#include "RageTextureManager.h"
#include "LightsManager.h"
#include "Game.h"
#include "InputMapper.h"
#include "ProfileManager.h"
#include "CharacterManager.h"

#define COIN_MODE_CHANGE_SCREEN		THEME->GetMetric (m_sName,"CoinModeChangeScreen")

REGISTER_SCREEN_CLASS( ScreenTitleMenu );
ScreenTitleMenu::ScreenTitleMenu( CString sScreenName ) : 
	ScreenSelectMaster( sScreenName )
{
	LOG->Trace( "ScreenTitleMenu::ScreenTitleMenu()" );

	/* XXX We really need two common calls: 1, something run when exiting from gameplay
	 * (to do this reset), and 2, something run when entering gameplay, to apply default
	 * options.  Having special cases in attract screens and the title menu to reset
	 * things stinks ... */
	GAMESTATE->Reset();

	// TRICKY: Do this after GameState::Reset.
	LIGHTSMAN->SetLightsMode( LIGHTSMODE_JOINING );

	this->SubscribeToMessage( PREFSMAN->m_CoinMode.GetName()+"Changed" );
}

void ScreenTitleMenu::Init()
{
	ScreenSelectMaster::Init();

	m_soundSelectPressed.Load( THEME->GetPathS(m_sName,"select down"), true );

	this->SortByDrawOrder();

	CHARMAN->DemandGraphics();

	SOUND->PlayOnceFromAnnouncer( "title menu game name" );
}

ScreenTitleMenu::~ScreenTitleMenu()
{
	LOG->Trace( "ScreenTitleMenu::~ScreenTitleMenu()" );
	
	CHARMAN->UndemandGraphics();
}

void ScreenTitleMenu::Input( const DeviceInput& DeviceI, const InputEventType type, const GameInput &GameI, const MenuInput &MenuI, const StyleInput &StyleI )
{
	LOG->Trace( "ScreenTitleMenu::Input( %d-%d )", DeviceI.device, DeviceI.button );	// debugging gameport joystick problem

	if( m_In.IsTransitioning() || m_Cancel.IsTransitioning() ) /* not m_Out */
		return;

	if( type == IET_FIRST_PRESS )
	{
		//
		// detect codes
		//
		if( CodeDetector::EnteredCode(GameI.controller,CODE_NEXT_THEME) ||
			CodeDetector::EnteredCode(GameI.controller,CODE_NEXT_THEME2) )
		{
			THEME->NextTheme();
			ApplyGraphicOptions();	// update window title and icon
			SCREENMAN->SystemMessage( "Theme: "+THEME->GetCurThemeName() );
			SCREENMAN->SetNewScreen( m_sName );
			TEXTUREMAN->DoDelayedDelete();
		}
		if( CodeDetector::EnteredCode(GameI.controller,CODE_NEXT_ANNOUNCER) ||
			CodeDetector::EnteredCode(GameI.controller,CODE_NEXT_ANNOUNCER2) )
		{
			ANNOUNCER->NextAnnouncer();
			CString sName = ANNOUNCER->GetCurAnnouncerName();
			if( sName=="" ) sName = "(none)";
			SCREENMAN->SystemMessage( "Announcer: "+sName );
			SCREENMAN->SetNewScreen( m_sName );
		}
		if( CodeDetector::EnteredCode(GameI.controller,CODE_NEXT_GAME) ||
			CodeDetector::EnteredCode(GameI.controller,CODE_NEXT_GAME2) )
		{
			vector<const Game*> vGames;
			GAMEMAN->GetEnabledGames( vGames );
			ASSERT( !vGames.empty() );
			vector<const Game*>::iterator iter = find(vGames.begin(),vGames.end(),GAMESTATE->m_pCurGame);
			ASSERT( iter != vGames.end() );

			iter++;	// move to the next game

			// wrap
			if( iter == vGames.end() )
				iter = vGames.begin();

			GAMESTATE->m_pCurGame = *iter;

			/* Reload the theme if it's changed, but don't back to the initial screen. */
			ResetGame();

			SCREENMAN->SystemMessage( CString("Game: ") + GAMESTATE->GetCurrentGame()->m_szName );
			SCREENMAN->SetNewScreen( m_sName );
		}
	}

	ScreenSelectMaster::Input( DeviceI, type, GameI, MenuI, StyleI );
}

void ScreenTitleMenu::HandleMessage( const CString& sMessage )
{
	if( sMessage == PREFSMAN->m_CoinMode.GetName()+"Changed" )
	{
		/* If the CoinMode was changed, we need to reload this screen
		 * so that the right m_aGameCommands will show */
		SCREENMAN->SetNewScreen( COIN_MODE_CHANGE_SCREEN );
	}
}


/*
 * (c) 2001-2004 Chris Danford
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
