#include "global.h"

#if !defined(WITHOUT_NETWORKING)
#include "NetworkSyncManager.h"
#include "ScreenNetworkOptions.h"
#include "RageLog.h"
#include "GameSoundManager.h"
#include "ThemeManager.h"
#include "PrefsManager.h"
#include "ScreenManager.h"
#include "ScreenTextEntry.h"
#include "ScreenPrompt.h"
#include "NetworkSyncServer.h"
#include "LocalizedString.h"
#include "OptionRowHandler.h"

static LocalizedString CLIENT_CONNECT	( "ScreenNetworkOptions", "Connect" );
static LocalizedString CLIENT_DISCONNECT	( "ScreenNetworkOptions", "Disconnect" );
static LocalizedString SERVER_START	( "ScreenNetworkOptions", "Start" );
static LocalizedString SERVER_STOP	( "ScreenNetworkOptions", "Stop" );
static LocalizedString SCORE_ON	( "ScreenNetworkOptions", "ScoreOn" );
static LocalizedString SCORE_OFF	( "ScreenNetworkOptions", "ScoreOff" );
static LocalizedString SERVER_STARTED	( "ScreenNetworkOptions", "Server started." );
static LocalizedString SERVER_FAILED	( "ScreenNetworkOptions", "Server failed: %s Code:%d" );

static LocalizedString DISCONNECTED				( "ScreenNetworkOptions", "Disconnected from server." );
static LocalizedString SERVER_STOPPED			( "ScreenNetworkOptions", "Server stopped." );
static LocalizedString ENTER_NETWORK_ADDRESS	("ScreenNetworkOptions","Enter a network address.");
static LocalizedString CONNECT_TO_YOURSELF		("ScreenNetworkOptions","Use 127.0.0.1 to connect to yourself.");
static LocalizedString ENTER_A_SERVER_NAME		("ScreenNetworkOptions","Enter a server name.");

enum {
	PO_CONNECTION,
	PO_SERVER,
	PO_SCOREBOARD,
	PO_SERVERS,
	NUM_NETWORK_OPTIONS_LINES
};

enum {
	NO_STOP_SERVER=0,
	NO_START_SERVER
};

enum
{
	NO_SCOREBOARD_OFF=0,
	NO_SCOREBOARD_ON
};

AutoScreenMessage( SM_DoneConnecting )
AutoScreenMessage( SM_ServerNameEnter )

Preference<RString> g_sLastServer( "LastConnectedServer",	"" );

REGISTER_SCREEN_CLASS( ScreenNetworkOptions );

void ScreenNetworkOptions::Init()
{
	ScreenOptions::Init();

	vector<OptionRowHandler*> vHands;
	{
		OptionRowHandler *pHand = OptionRowHandlerUtil::MakeNull();
		vHands.push_back( pHand );
		pHand->m_Def.m_sName = "Connection";
		pHand->m_Def.m_bOneChoiceForAllPlayers = true;
		if ( NSMAN->useSMserver )
			pHand->m_Def.m_vsChoices.push_back(CLIENT_DISCONNECT);
		else
			pHand->m_Def.m_vsChoices.push_back(CLIENT_CONNECT);
	}
	{
		OptionRowHandler *pHand = OptionRowHandlerUtil::MakeNull();
		vHands.push_back( pHand );
		pHand->m_Def.m_bAllowThemeItems = false;
		pHand->m_Def.m_bOneChoiceForAllPlayers = true;
		pHand->m_Def.m_sName = "Server";
		pHand->m_Def.m_vsChoices.push_back(SERVER_START);
		pHand->m_Def.m_vsChoices.push_back(SERVER_STOP);
	}
	{
		OptionRowHandler *pHand = OptionRowHandlerUtil::MakeNull();
		vHands.push_back( pHand );
		pHand->m_Def.m_sName = "Scoreboard";
		pHand->m_Def.m_vsChoices.clear();
		pHand->m_Def.m_bOneChoiceForAllPlayers = true;
		pHand->m_Def.m_vsChoices.push_back(SCORE_OFF);
		pHand->m_Def.m_vsChoices.push_back(SCORE_ON);
	}
	{

		// Get info on all received servers from NSMAN.
		AllServers.clear();
		NSMAN->GetListOfLANServers( AllServers );
		if( !AllServers.empty() )
		{
			OptionRowHandler *pHand = OptionRowHandlerUtil::MakeNull();
			pHand->m_Def.m_sName = "Servers";
			for( unsigned int j = 0; j < AllServers.size(); j++ )
				pHand->m_Def.m_vsChoices.push_back( AllServers[j].Name );
			vHands.push_back( pHand );
		}
	}

	InitMenu( vHands );

	m_pRows[PO_SCOREBOARD]->SetOneSharedSelection(PREFSMAN->m_bEnableScoreboard);
}

void ScreenNetworkOptions::HandleScreenMessage( const ScreenMessage SM )
{
	if( SM == SM_DoneConnecting )
	{
		if( !ScreenTextEntry::s_bCancelledLast )
		{
			RString sNewName = ScreenTextEntry::s_sLastAnswer;
			NSMAN->PostStartUp(sNewName);
			NSMAN->DisplayStartupStatus();
			UpdateConnectStatus( );
			g_sLastServer.Set( ScreenTextEntry::s_sLastAnswer );
		}
	}
	else if( SM == SM_ServerNameEnter )
	{
		if( !ScreenTextEntry::s_bCancelledLast )
		{
			if ( NSMAN->LANserver == NULL)
				NSMAN->LANserver = new StepManiaLanServer;
			NSMAN->LANserver->servername = ScreenTextEntry::s_sLastAnswer;
			if (NSMAN->LANserver->ServerStart())
			{
				NSMAN->isLanServer = true;
				SCREENMAN->SystemMessage( SERVER_STARTED );
			}
			else
			{
				SCREENMAN->SystemMessage( ssprintf(SERVER_FAILED.GetValue(),NSMAN->LANserver->lastError.c_str(),NSMAN->LANserver->lastErrorCode) );
			}
		}
	}

	ScreenOptions::HandleScreenMessage( SM );
}

void ScreenNetworkOptions::MenuStart( const InputEventPlus &input )
{
#if defined( WITHOUT_NETWORKING )
#else
	switch( GetCurrentRow() )
	{
	case PO_CONNECTION:
		if ( !NSMAN->useSMserver )
		{
			ScreenTextEntry::TextEntry( SM_DoneConnecting, ENTER_NETWORK_ADDRESS.GetValue()+"\n\n"+CONNECT_TO_YOURSELF.GetValue(), g_sLastServer, 128 );
		}
		else
		{
			NSMAN->CloseConnection();
			SCREENMAN->SystemMessage( DISCONNECTED );
			UpdateConnectStatus( );
		}
		break;
	case PO_SERVER:
		switch( m_pRows[GetCurrentRow()]->GetOneSharedSelection() )
		{
		case NO_START_SERVER:
			if (!NSMAN->isLanServer)
			{
				ScreenTextEntry::TextEntry( SM_ServerNameEnter, ENTER_A_SERVER_NAME, "", 128);
			}
			break;
		case NO_STOP_SERVER:
			if ( NSMAN->LANserver != NULL )
				NSMAN->LANserver->ServerStop();
			SCREENMAN->SystemMessage( SERVER_STOPPED );
			NSMAN->isLanServer = false;
			break;
		}
		break;
	case PO_SCOREBOARD:
		if (m_pRows[PO_SCOREBOARD]->GetOneSharedSelection() == NO_SCOREBOARD_ON)
			PREFSMAN->m_bEnableScoreboard.Set(true);
		else
			PREFSMAN->m_bEnableScoreboard.Set(false);
		break;
	case PO_SERVERS:
		if ( !AllServers.empty() )
		{
			string sNewName = AllServers[m_pRows[GetCurrentRow()]->GetOneSharedSelection()].Address;
			NSMAN->PostStartUp(sNewName);
			NSMAN->DisplayStartupStatus();
			UpdateConnectStatus( );
		}
		else
		{
			//If the server list is empty, keep passing the message on so exit works
			ScreenOptions::MenuStart( input );
		}
		break;
	default:
		ScreenOptions::MenuStart( input );
	}
#endif
}

void ScreenNetworkOptions::ImportOptions( int iRow, const vector<PlayerNumber> &vpns ) { }
void ScreenNetworkOptions::ExportOptions( int iRow, const vector<PlayerNumber> &vpns ) { }

void ScreenNetworkOptions::UpdateConnectStatus( )
{
	SCREENMAN->SetNewScreen( m_sName );
}
#endif

/*
 * (c) 2004 Charles Lohr, Josh Allen
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
