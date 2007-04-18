#include "global.h"
#include "ScreenSelect.h"
#include "ScreenManager.h"
#include "GameSoundManager.h"
#include "RageLog.h"
#include "AnnouncerManager.h"
#include "GameState.h"
#include "ThemeManager.h"
#include "GameCommand.h"
#include "LightsManager.h"
#include "InputEventPlus.h"

#define CHOICE_NAMES		THEME->GetMetric (m_sName,"ChoiceNames")
#define CHOICE( s )		THEME->GetMetric (m_sName,ssprintf("Choice%s",s.c_str()))
#define IDLE_TIMEOUT_SCREEN	THEME->GetMetric (m_sName,"IdleTimeoutScreen")
#define UPDATE_ON_MESSAGE	THEME->GetMetric (m_sName,"UpdateOnMessage")

void ScreenSelect::Init()
{
	IDLE_COMMENT_SECONDS.Load( m_sName, "IdleCommentSeconds" );
	IDLE_TIMEOUT_SECONDS.Load( m_sName, "IdleTimeoutSeconds" );
	ALLOW_DISABLED_PLAYER_INPUT.Load( m_sName, "AllowDisabledPlayerInput" );

	ScreenWithMenuElements::Init();

	//
	// Load messages to update on
	//
	split( UPDATE_ON_MESSAGE, ",", m_asSubscribedMessages );
	for( unsigned i = 0; i < m_asSubscribedMessages.size(); ++i )
		MESSAGEMAN->Subscribe( this, m_asSubscribedMessages[i] );

	//
	// Load choices
	//
	{
		// Instead of using NUM_CHOICES, use a comma-separated list of choices.  Each
		// element in the list is a choice name.  This level of indirection 
		// makes it easier to add or remove items without having to change a bunch
		// of indices.
		vector<RString> asChoiceNames;
		split( CHOICE_NAMES, ",", asChoiceNames, true );

		for( unsigned c=0; c<asChoiceNames.size(); c++ )
		{
			RString sChoiceName = asChoiceNames[c];

			GameCommand mc;
			mc.ApplyCommitsScreens( false );
			mc.m_sName = sChoiceName;
			Commands cmd = ParseCommands( CHOICE(sChoiceName) );
			mc.Load( c, cmd );
			m_aGameCommands.push_back( mc );
		}
	}

	if( !m_aGameCommands.size() )
		RageException::Throw( "Screen \"%s\" does not set any choices.", m_sName.c_str() );
}

void ScreenSelect::BeginScreen()
{
	ScreenWithMenuElements::BeginScreen();

	m_timerIdleComment.GetDeltaTime();
	m_timerIdleTimeout.GetDeltaTime();

	// derived classes can override if they want
	LIGHTSMAN->SetLightsMode( LIGHTSMODE_MENU );
}


ScreenSelect::~ScreenSelect()
{
	LOG->Trace( "ScreenSelect::~ScreenSelect()" );
	for( unsigned i = 0; i < m_asSubscribedMessages.size(); ++i )
		MESSAGEMAN->Unsubscribe( this, m_asSubscribedMessages[i] );
}

void ScreenSelect::Update( float fDelta )
{
	if( !IsTransitioning() )
	{
		if( IDLE_COMMENT_SECONDS > 0 && m_timerIdleComment.PeekDeltaTime() >= IDLE_COMMENT_SECONDS )
		{
			SOUND->PlayOnceFromAnnouncer( m_sName+" IdleComment" );
			m_timerIdleComment.GetDeltaTime();
		}
		// don't time out on this screen is coin mode is pay.  
		// If we're here, then there's a credit in the machine.
		if( GAMESTATE->GetCoinMode() != CoinMode_Pay )
		{
			if( IDLE_TIMEOUT_SECONDS > 0 && m_timerIdleTimeout.PeekDeltaTime() >= IDLE_TIMEOUT_SECONDS )
			{
				SCREENMAN->SetNewScreen( IDLE_TIMEOUT_SCREEN );
				m_timerIdleTimeout.GetDeltaTime();
				return;
			}
		}
	}

	ScreenWithMenuElements::Update( fDelta );
}

void ScreenSelect::Input( const InputEventPlus &input )
{
//	LOG->Trace( "ScreenSelect::Input()" );

	/* Reset the announcer timers when a key is pressed. */
	m_timerIdleComment.GetDeltaTime();
	m_timerIdleTimeout.GetDeltaTime();

	/* Choices may change when more coins are inserted. */
	if( input.MenuI == MENU_BUTTON_COIN && input.type == IET_FIRST_PRESS )
		this->UpdateSelectableChoices();

	if( input.MenuI == MENU_BUTTON_START && input.type == IET_FIRST_PRESS && GAMESTATE->JoinInput(input.pn) )
	{
		// HACK: Only play start sound for the 2nd player who joins.  The 
		// start sound for the 1st player will be played by ScreenTitleMenu 
		// when the player makes a selection on the screen.
		if( GAMESTATE->GetNumSidesJoined() > 1 )
			SCREENMAN->PlayStartSound();

		this->UpdateSelectableChoices();
	
		if( !ALLOW_DISABLED_PLAYER_INPUT )
			return;	// don't let the screen handle the MENU_START press
	}

	// block input of disabled players
	if( !ALLOW_DISABLED_PLAYER_INPUT && !GAMESTATE->IsPlayerEnabled(input.pn) )
		return;

	if( IsTransitioning() )
		return;

	ScreenWithMenuElements::Input( input );	// default input handler
}

void ScreenSelect::HandleScreenMessage( const ScreenMessage SM )
{
	if( SM == SM_BeginFadingOut )	/* Screen is starting to tween out. */
	{
		/*
		 * Don't call GameCommand::Apply once per player on screens that 
		 * have a shared selection.  This can cause change messages to be broadcast
		 * multiple times.  Detect whether all players have the same choice, and 
		 * if so, call ApplyToAll instead.
		 * TODO: Think of a better way to handle this.
		 */
		int iMastersIndex = this->GetSelectionIndex( GAMESTATE->m_MasterPlayerNumber );
		bool bAllPlayersChoseTheSame = true;
		FOREACH_HumanPlayer( p )
		{
			if( this->GetSelectionIndex(p) != iMastersIndex )
			{
				bAllPlayersChoseTheSame = false;
				break;
			}
		}

		if( bAllPlayersChoseTheSame )
		{
			const GameCommand &gc = m_aGameCommands[iMastersIndex];
			m_sNextScreen = gc.m_sScreen;
			if( !gc.m_bInvalid )
				gc.ApplyToAllPlayers();
		}
		else
		{
			FOREACH_HumanPlayer( p )
			{
				int iIndex = this->GetSelectionIndex(p);
				const GameCommand &gc = m_aGameCommands[iIndex];
				m_sNextScreen = gc.m_sScreen;
				if( !gc.m_bInvalid )
					gc.Apply( p );
			}
		}

		StopTimer();

		SCREENMAN->RefreshCreditsMessages();

		if( !IsTransitioning() )
			StartTransitioningScreen( SM_GoToNextScreen );
	}

	ScreenWithMenuElements::HandleScreenMessage( SM );
}

void ScreenSelect::HandleMessage( const Message &msg )
{
	if( find(m_asSubscribedMessages.begin(), m_asSubscribedMessages.end(), msg.GetName()) != m_asSubscribedMessages.end() )
		this->UpdateSelectableChoices();

	ScreenWithMenuElements::HandleMessage( msg );
}

void ScreenSelect::MenuBack( const InputEventPlus &input )
{
	Cancel( SM_GoToPrevScreen );
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

