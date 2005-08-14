#include "global.h"

#include "ScreenOptionsEditProfile.h"
#include "ScreenManager.h"
#include "ProfileManager.h"
#include "ScreenTextEntry.h"
#include "ScreenPrompt.h"
#include "RageUtil.h"
#include "GameState.h"
#include "Profile.h"
#include "Character.h"

enum EditProfileRow
{
	ROW_CHARACTER,
};

REGISTER_SCREEN_CLASS( ScreenOptionsEditProfile );
ScreenOptionsEditProfile::ScreenOptionsEditProfile( CString sName ) : 
	ScreenOptions( sName )
{

}

void ScreenOptionsEditProfile::Init()
{
	ScreenOptions::Init();
}

void ScreenOptionsEditProfile::BeginScreen()
{
	m_Original = *GAMESTATE->GetEditLocalProfile();

	vector<OptionRowDefinition> vDefs;
	vector<OptionRowHandler*> vHands;

	OptionRowDefinition def;
	def.m_layoutType = LAYOUT_SHOW_ONE_IN_ROW;
	def.m_bOneChoiceForAllPlayers = true;
	def.m_bAllowThemeItems = false;
	def.m_bAllowThemeTitles = false;
	def.m_bAllowExplanation = false;
	def.m_bExportOnChange = true;
	
	Profile *pProfile = PROFILEMAN->GetLocalProfile( GAMESTATE->m_sEditLocalProfileID );
	ASSERT( pProfile );

	{
		def.m_sName = "Character";
		def.m_vsChoices.clear();
		vector<Character*> vpCharacters;
		GAMESTATE->GetCharacters( vpCharacters );
		FOREACH_CONST( Character*, vpCharacters, c )
			def.m_vsChoices.push_back( (*c)->GetDisplayName() );
		vDefs.push_back( def );
		vHands.push_back( NULL );
	}

	InitMenu( vDefs, vHands );

	ScreenOptions::BeginScreen();
}

ScreenOptionsEditProfile::~ScreenOptionsEditProfile()
{

}

void ScreenOptionsEditProfile::ImportOptions( int iRow, const vector<PlayerNumber> &vpns )
{
	Profile *pProfile = PROFILEMAN->GetLocalProfile( GAMESTATE->m_sEditLocalProfileID );
	ASSERT( pProfile );
	OptionRow &row = *m_pRows[iRow];

	switch( iRow )
	{
	case ROW_CHARACTER:
		row.SetOneSharedSelectionIfPresent( pProfile->m_sCharacterID );
		break;
	}
}

void ScreenOptionsEditProfile::ExportOptions( int iRow, const vector<PlayerNumber> &vpns )
{
	Profile *pProfile = PROFILEMAN->GetLocalProfile( GAMESTATE->m_sEditLocalProfileID );
	ASSERT( pProfile );
	OptionRow &row = *m_pRows[iRow];
	int iIndex = row.GetOneSharedSelection( true );
	CString sValue;
	if( iIndex >= 0 )
		sValue = row.GetRowDef().m_vsChoices[ iIndex ];

	switch( iRow )
	{
	case ROW_CHARACTER:
		pProfile->m_sCharacterID = sValue;
		break;
	}
}

void ScreenOptionsEditProfile::GoToNextScreen()
{
}

void ScreenOptionsEditProfile::GoToPrevScreen()
{
}

void ScreenOptionsEditProfile::HandleScreenMessage( const ScreenMessage SM )
{
	if( SM == SM_GoToNextScreen )
	{
		PROFILEMAN->SaveLocalProfile( GAMESTATE->m_sEditLocalProfileID );
	}
	else if( SM == SM_GoToPrevScreen )
	{
		*GAMESTATE->GetEditLocalProfile() = m_Original;
	}

	ScreenOptions::HandleScreenMessage( SM );
}

void ScreenOptionsEditProfile::AfterChangeValueInRow( int iRow, PlayerNumber pn )
{
	ScreenOptions::AfterChangeValueInRow( iRow, pn );

	// cause the overlay to reload
	GAMESTATE->m_sEditLocalProfileID.Set( GAMESTATE->m_sEditLocalProfileID );
}

void ScreenOptionsEditProfile::ProcessMenuStart( PlayerNumber pn, const InputEventType type )
{
	int iRow = GetCurrentRow();;
	OptionRow &row = *m_pRows[iRow];

	switch( iRow )
	{
	case ROW_CHARACTER:
		{
		}
		break;
	default:
		ScreenOptions::ProcessMenuStart( pn, type );
		break;
	}
}


/*
 * (c) 2003-2004 Chris Danford
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
