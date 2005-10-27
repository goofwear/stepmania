#include "global.h"
#include "ScreenProfileOptions.h"
#include "RageLog.h"
#include "ProfileManager.h"
#include "GameSoundManager.h"
#include "ThemeManager.h"
#include "PrefsManager.h"
#include "ScreenManager.h"
#include "ScreenTextEntry.h"
#include "ScreenPrompt.h"
#include "GameState.h"
#include "Profile.h"


enum {
	PO_PLAYER1,
	PO_PLAYER2,
	PO_CREATE_NEW,
	PO_DELETE_,
	PO_RENAME_,
	PO_OS_MOUNT_1,
	PO_OS_MOUNT_2,
	NUM_PROFILE_OPTIONS_LINES
};

OptionRowDefinition g_ProfileOptionsLines[NUM_PROFILE_OPTIONS_LINES] = {
	OptionRowDefinition( "Player1Profile",	true ),
	OptionRowDefinition( "Player2Profile",	true ),
	OptionRowDefinition( "CreateNew",		true, "PRESS START" ),
	OptionRowDefinition( "Delete",			true ),
	OptionRowDefinition( "Rename",			true ),
	OptionRowDefinition( "OsMountPlayer1",	true, "" ),
	OptionRowDefinition( "OsMountPlayer2",	true, "" ),
};

AutoScreenMessage( SM_DoneCreating )
AutoScreenMessage( SM_DoneRenaming )
AutoScreenMessage( SM_DoneDeleting )

REGISTER_SCREEN_CLASS( ScreenProfileOptions );
ScreenProfileOptions::ScreenProfileOptions( CString sClassName ) : ScreenOptions( sClassName )
{
	LOG->Trace( "ScreenProfileOptions::ScreenProfileOptions()" );
}

void ScreenProfileOptions::Init()
{
	ScreenOptions::Init();

	g_ProfileOptionsLines[PO_PLAYER1].m_vsChoices.clear();
	g_ProfileOptionsLines[PO_PLAYER1].m_vsChoices.push_back( "-NONE-" );
	PROFILEMAN->GetLocalProfileDisplayNames( g_ProfileOptionsLines[PO_PLAYER1].m_vsChoices );

	g_ProfileOptionsLines[PO_PLAYER2].m_vsChoices.clear();
	g_ProfileOptionsLines[PO_PLAYER2].m_vsChoices.push_back( "-NONE-" );
	PROFILEMAN->GetLocalProfileDisplayNames( g_ProfileOptionsLines[PO_PLAYER2].m_vsChoices );

	g_ProfileOptionsLines[PO_DELETE_].m_vsChoices.clear();
	g_ProfileOptionsLines[PO_DELETE_].m_vsChoices.push_back( "-NONE-" );
	PROFILEMAN->GetLocalProfileDisplayNames( g_ProfileOptionsLines[PO_DELETE_].m_vsChoices );

	g_ProfileOptionsLines[PO_RENAME_].m_vsChoices.clear();
	g_ProfileOptionsLines[PO_RENAME_].m_vsChoices.push_back( "-NONE-" );
	PROFILEMAN->GetLocalProfileDisplayNames( g_ProfileOptionsLines[PO_RENAME_].m_vsChoices );

	if( PREFSMAN->m_sMemoryCardOsMountPoint[PLAYER_1].Get().empty() )
		g_ProfileOptionsLines[PO_OS_MOUNT_1].m_vsChoices[0] = "-NOT SET IN INI-";
	else
		g_ProfileOptionsLines[PO_OS_MOUNT_1].m_vsChoices[0] = PREFSMAN->m_sMemoryCardOsMountPoint[PLAYER_1].Get();

	if( PREFSMAN->m_sMemoryCardOsMountPoint[PLAYER_2].Get().empty() )
		g_ProfileOptionsLines[PO_OS_MOUNT_2].m_vsChoices[0] = "-NOT SET IN INI-";
	else
		g_ProfileOptionsLines[PO_OS_MOUNT_2].m_vsChoices[0] = PREFSMAN->m_sMemoryCardOsMountPoint[PLAYER_2].Get();

	//Enable all lines for all players
	for ( unsigned int i = 0; i < NUM_PROFILE_OPTIONS_LINES; i++ )
		FOREACH_PlayerNumber( pn )
			g_ProfileOptionsLines[i].m_vEnabledForPlayers.insert( pn );

	vector<OptionRowDefinition> vDefs( &g_ProfileOptionsLines[0], &g_ProfileOptionsLines[ARRAYSIZE(g_ProfileOptionsLines)] );
	vector<OptionRowHandler*> vHands( vDefs.size(), NULL );
	InitMenu( vDefs, vHands );
}

void ScreenProfileOptions::ImportOptions( int iRow, const vector<PlayerNumber> &vpns )
{
	switch( iRow )
	{
	case PO_PLAYER1:
	case PO_PLAYER2:
		{
			PlayerNumber pn = (PlayerNumber)(iRow - PO_PLAYER1);
			vector<CString> vsProfiles;
			PROFILEMAN->GetLocalProfileIDs( vsProfiles );

			CStringArray::iterator iter = find( 
				vsProfiles.begin(),
				vsProfiles.end(),
				PREFSMAN->m_sMemoryCardOsMountPoint[pn].Get() );
			if( iter != vsProfiles.end() )
				m_pRows[iRow]->SetOneSharedSelection( iter - vsProfiles.begin() + 1 );
		}
		break;
	}
}

void ScreenProfileOptions::ExportOptions( int iRow, const vector<PlayerNumber> &vpns )
{
	switch( iRow )
	{
	case PO_PLAYER1:
	case PO_PLAYER2:
		{
			PlayerNumber pn = (PlayerNumber)(iRow - PO_PLAYER1);
			vector<CString> vsProfiles;
			PROFILEMAN->GetLocalProfileIDs( vsProfiles );

			if( m_pRows[iRow]->GetOneSharedSelection() > 0 )
				PREFSMAN->m_sMemoryCardOsMountPoint[pn].Set( vsProfiles[m_pRows[iRow]->GetOneSharedSelection()-1] );
			else
				PREFSMAN->m_sMemoryCardOsMountPoint[pn].Set( "" );
		}
		break;
	}
}

void ScreenProfileOptions::HandleScreenMessage( const ScreenMessage SM )
{
	if( SM == SM_GoToNextScreen )
	{
		PREFSMAN->SavePrefsToDisk();
		SCREENMAN->SetNewScreen( "ScreenOptionsService" );
		return;
	}
	else if( SM == SM_GoToPrevScreen )
	{
		SCREENMAN->SetNewScreen( "ScreenOptionsService" );
		return;
	}
	else if( SM == SM_DoneCreating )
	{
		if( !ScreenTextEntry::s_bCancelledLast && ScreenTextEntry::s_sLastAnswer != "" )
		{
			CString sNewName = ScreenTextEntry::s_sLastAnswer;
			CString s;
			bool bResult = PROFILEMAN->CreateLocalProfile( sNewName, s );
//			bool bResult = PROFILEMAN->CreateLocalProfile( sNewName, GAMESTATE->m_sLastSelectedProfileID );
			if( bResult )
				SCREENMAN->SetNewScreen( m_sName );	// reload
			else
				ScreenPrompt::Prompt( SM_None, ssprintf("Error creating profile '%s'.", sNewName.c_str()) );
		}
	}
	else if( SM == SM_DoneRenaming )
	{
		if( !ScreenTextEntry::s_bCancelledLast )
		{
			CString sProfileID = GetSelectedProfileID();
			CString sName = GetSelectedProfileName();
			CString sNewName = ScreenTextEntry::s_sLastAnswer;
			bool bResult = PROFILEMAN->RenameLocalProfile( sProfileID, sNewName );
			if( bResult )
				SCREENMAN->SetNewScreen( m_sName );	// reload
			else
				ScreenPrompt::Prompt( SM_None, ssprintf("Error renaming profile %s '%s'\nto '%s'.", sProfileID.c_str(), sName.c_str(), sNewName.c_str()) );
		}
	}
	else if ( SM == SM_DoneDeleting )
	{
		if( ScreenPrompt::s_LastAnswer == ANSWER_YES )
		{
			CString sProfileID = GetSelectedProfileID();
			CString sName = GetSelectedProfileName();
			bool bResult = PROFILEMAN->DeleteLocalProfile( sProfileID );
			if( bResult )
				SCREENMAN->SetNewScreen( m_sName );	// reload
			else
				ScreenPrompt::Prompt( SM_None, ssprintf("Error deleting profile %s '%s'.", sName.c_str(), sProfileID.c_str()) );
		}
	}

	ScreenOptions::HandleScreenMessage( SM );
}


void ScreenProfileOptions::MenuStart( const InputEventPlus &input )
{
	switch( GetCurrentRow() )
	{
	case PO_CREATE_NEW:
		ScreenTextEntry::TextEntry( SM_DoneCreating, "Enter a profile name", "", 12 );
		break;
	case PO_DELETE_:
	{
		const CString sProfileID = GetSelectedProfileID();
		const CString sName = GetSelectedProfileName();

		if( sProfileID=="" )
			SCREENMAN->PlayInvalidSound();
		else
			ScreenPrompt::Prompt( SM_DoneDeleting, ssprintf("Delete profile %s '%s'?",sProfileID.c_str(),sName.c_str()), PROMPT_YES_NO, ANSWER_NO );
		break;
	}
	case PO_RENAME_:
	{
		const CString sProfileID = GetSelectedProfileID();
		const CString sName = GetSelectedProfileName();

		if( sProfileID=="" )
			SCREENMAN->PlayInvalidSound();
		else
			ScreenTextEntry::TextEntry( SM_DoneRenaming, ssprintf("Rename profile %s '%s'",sProfileID.c_str(),sName.c_str()), sName, 12 );
		break;
	}
	default:
		ScreenOptions::MenuStart( input );
	}
}

CString ScreenProfileOptions::GetSelectedProfileID()
{
	vector<CString> vsProfiles;
	PROFILEMAN->GetLocalProfileIDs( vsProfiles );

	const OptionRow &row = *m_pRows[GetCurrentRow()];
	const int Selection = row.GetOneSharedSelection();
	if( !Selection )
		return CString();
	return vsProfiles[ Selection-1 ];
}

CString ScreenProfileOptions::GetSelectedProfileName()
{
	const OptionRow &row = *m_pRows[GetCurrentRow()];
	const int Selection = row.GetOneSharedSelection();
	if( !Selection )
		return CString();
	return g_ProfileOptionsLines[PO_PLAYER1].m_vsChoices[ Selection ];
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
