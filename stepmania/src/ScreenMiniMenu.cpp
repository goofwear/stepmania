#include "global.h"
#include "ScreenMiniMenu.h"
#include "PrefsManager.h"
#include "ScreenManager.h"
#include "GameSoundManager.h"
#include "GameConstantsAndTypes.h"
#include "PrefsManager.h"
#include "ThemeManager.h"
#include "Foreach.h"
#include "ScreenDimensions.h"
#include "CommonMetrics.h"
#include "GameState.h"
#include "FontCharAliases.h"

AutoScreenMessage( SM_GoToOK )
AutoScreenMessage( SM_GoToCancel )

int	ScreenMiniMenu::s_iLastRowCode = -1;
vector<int>	ScreenMiniMenu::s_viLastAnswers;

//REGISTER_SCREEN_CLASS( ScreenMiniMenu );
ScreenMiniMenu::ScreenMiniMenu( CString sClassName ) :ScreenOptions( sClassName )
{
}

void ScreenMiniMenu::Init( const Menu* pDef, ScreenMessage SM_SendOnOK, ScreenMessage SM_SendOnCancel )
{
	ScreenOptions::Init();

	m_Background.Load( THEME->GetPathB(m_sName,"background") );
	m_Background->SetDrawOrder( DRAW_ORDER_BEFORE_EVERYTHING );
	this->AddChild( m_Background );
	m_Background->PlayCommand( "On" );

	this->SortByDrawOrder();

	m_bIsTransparent = true;	// draw screens below us

	m_SMSendOnOK = SM_SendOnOK;
	m_SMSendOnCancel = SM_SendOnCancel;


	FOREACH_CONST( MenuRow, pDef->rows, r )
	{
		// Don't add rows that aren't applicable to HomeEditMode.
		if( !HOME_EDIT_MODE || r->bShowInHomeEditMode )
			m_vMenuRows.push_back( *r );
	}

	// Convert from m_vMenuRows to vector<OptionRowDefinition>
	vector<OptionRowDefinition> vDefs;
	vDefs.resize( m_vMenuRows.size() );
	for( unsigned r=0; r<m_vMenuRows.size(); r++ )
	{
		const MenuRow &mr = m_vMenuRows[r];
		OptionRowDefinition &ord = vDefs[r];

		ord.name = mr.sName;
		FontCharAliases::ReplaceMarkers( ord.name );	// Allow special characters
		
		if( mr.bEnabled )
		{
			ord.m_vEnabledForPlayers.clear();
			FOREACH_EnabledPlayer( pn )
				ord.m_vEnabledForPlayers.insert( pn );
		}
		else
		{
			ord.m_vEnabledForPlayers.clear();
		}

		ord.bOneChoiceForAllPlayers = true;
		ord.selectType = SELECT_ONE;
		ord.layoutType = LAYOUT_SHOW_ONE_IN_ROW;
		ord.m_bExportOnChange = false;
			
		ord.choices = mr.choices;

		FOREACH( CString, ord.choices, c )
			FontCharAliases::ReplaceMarkers( *c );	// Allow special characters
	}

	vector<OptionRowHandler*> vHands;
	vHands.resize( vDefs.size(), NULL );

	ScreenOptions::InitMenu( INPUTMODE_SHARE_CURSOR, vDefs, vHands );
}

void ScreenMiniMenu::ImportOptions( int r, PlayerNumber pn )
{
	OptionRow &optrow = *m_Rows[r];
	MenuRow &mr = m_vMenuRows[r];
	if( !mr.choices.empty() )
		optrow.SetOneSharedSelection( mr.iDefaultChoice );
}

void ScreenMiniMenu::ExportOptions( int r, PlayerNumber pn )
{
	if( r == GetCurrentRow() )
		s_iLastRowCode = m_vMenuRows[r].iRowCode;
	s_viLastAnswers.resize( m_vMenuRows.size() );
	s_viLastAnswers[r] = m_Rows[r]->GetOneSharedSelection( true );
}

void ScreenMiniMenu::GoToNextScreen()
{
	SCREENMAN->PopTopScreen( m_SMSendOnOK );
}

void ScreenMiniMenu::GoToPrevScreen()
{
	SCREENMAN->PopTopScreen( m_SMSendOnCancel );
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
