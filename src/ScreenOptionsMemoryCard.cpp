#include "global.h"
#include "ScreenOptionsMemoryCard.h"
#include "RageLog.h"
#include "arch/MemoryCard/MemoryCardDriver.h"
#include "MemoryCardManager.h"
#include "GameState.h"
#include "ScreenManager.h"
#include "ScreenPrompt.h"
#include "LocalizedString.h"
#include "OptionRowHandler.h"


REGISTER_SCREEN_CLASS( ScreenOptionsMemoryCard );

void ScreenOptionsMemoryCard::Init()
{
	ScreenOptions::Init();

	this->SubscribeToMessage( Message_StorageDevicesChanged );
}

bool ScreenOptionsMemoryCard::UpdateCurrentUsbStorageDevices()
{
	vector<UsbStorageDevice> aOldDevices = m_CurrentUsbStorageDevices;

	const vector<UsbStorageDevice> &aNewDevices = MEMCARDMAN->GetStorageDevices();

	m_CurrentUsbStorageDevices.clear();
	for( size_t i = 0; i < aNewDevices.size(); ++i )
	{
		const UsbStorageDevice &dev = aNewDevices[i];
		if( dev.m_State == UsbStorageDevice::STATE_CHECKING )
			continue;
		m_CurrentUsbStorageDevices.push_back( dev );
	}

	return aOldDevices != m_CurrentUsbStorageDevices;
}

void ScreenOptionsMemoryCard::BeginScreen()
{
	vector<OptionRowHandler*> vHands;
	
	UpdateCurrentUsbStorageDevices();

	FOREACH_CONST( UsbStorageDevice, m_CurrentUsbStorageDevices, iter )
	{
		// TODO: Make these string themable

		vector<RString> vs;
		if( iter->sOsMountDir.empty() )
			vs.push_back( "(no mount dir)" );
		else
			vs.push_back( iter->sOsMountDir );
		if( iter->sVolumeLabel.empty() )
			vs.push_back( "(no label)" );
		else
			vs.push_back( iter->sVolumeLabel );
		if( iter->iVolumeSizeMB == 0 )
			vs.push_back( "size ???" );
		else
			vs.push_back( ssprintf("%dMB",iter->iVolumeSizeMB) );

		vHands.push_back( OptionRowHandlerUtil::MakeNull() );

		OptionRowDefinition &def = vHands.back()->m_Def;
		RString sDescription = join(", ", vs);
		def.m_sName = sDescription;
		def.m_vsChoices.push_back( "" );
		def.m_sExplanationName = "Memory Card";
		def.m_bAllowThemeTitle = false;
		def.m_bOneChoiceForAllPlayers = true;
	}

	if( m_CurrentUsbStorageDevices.empty() )
	{
		OptionRowDefinition def( "No memory cards detected", true, "" );
	}
	
	InitMenu( vHands );

	ScreenOptions::BeginScreen();

	// select the last chosen memory card (if present)
	SelectRowWithMemoryCard( MEMCARDMAN->m_sEditorMemoryCardOsMountPoint.Get() );
}

void ScreenOptionsMemoryCard::HandleScreenMessage( const ScreenMessage SM )
{
	ScreenOptions::HandleScreenMessage( SM );
}

void ScreenOptionsMemoryCard::HandleMessage( const RString& sMessage )
{
	if( sMessage == MessageToString(Message_StorageDevicesChanged) )
	{
		if( !m_Out.IsTransitioning() )
			SCREENMAN->SetNewScreen( this->m_sName );	// reload
	}
}

void ScreenOptionsMemoryCard::ImportOptions( int iRow, const vector<PlayerNumber> &vpns )
{

}

void ScreenOptionsMemoryCard::ExportOptions( int iRow, const vector<PlayerNumber> &vpns )
{
	OptionRow &row = *m_pRows[iRow];
	if( row.GetRowType() == OptionRow::RowType_Exit )
		return;

	PlayerNumber pn = GAMESTATE->m_MasterPlayerNumber;
	if( m_iCurrentRow[pn] == iRow )
	{
		const vector<UsbStorageDevice> &v = m_CurrentUsbStorageDevices;
		if( iRow < int(v.size()) )
		{
			const UsbStorageDevice &dev = v[iRow];
			MEMCARDMAN->m_sEditorMemoryCardOsMountPoint.Set( dev.sOsMountDir );
		}
	}
}

void ScreenOptionsMemoryCard::SelectRowWithMemoryCard( const RString &sOsMountPoint )
{
	if( sOsMountPoint.empty() )
		return;

	const vector<UsbStorageDevice> &v = m_CurrentUsbStorageDevices;
	for( unsigned i=0; i<v.size(); i++ )
	{
		if( v[i].sOsMountDir == sOsMountPoint )
		{
			this->MoveRowAbsolute( PLAYER_1, i, false );
			return;
		}
	}
}

static LocalizedString ERROR_MOUNTING_CARD ("ScreenOptionsMemoryCard", "Error mounting card: %s" );
void ScreenOptionsMemoryCard::ProcessMenuStart( const InputEventPlus &input )
{
	int iCurRow = m_iCurrentRow[GAMESTATE->m_MasterPlayerNumber];

	const vector<UsbStorageDevice> &v = m_CurrentUsbStorageDevices;
	if( iCurRow < int(v.size()) )	// a card
	{
		const vector<UsbStorageDevice> &v = m_CurrentUsbStorageDevices;
		const UsbStorageDevice &dev = v[iCurRow];
		MEMCARDMAN->m_sEditorMemoryCardOsMountPoint.Set( dev.sOsMountDir );
		bool bSuccess = MEMCARDMAN->MountCard( PLAYER_1, dev );
		if( bSuccess )
		{
			SCREENMAN->PlayStartSound();
			this->BeginFadingOut();
		}
		else
		{
			RString s = ssprintf(ERROR_MOUNTING_CARD.GetValue(), MEMCARDMAN->GetCardError(PLAYER_1).c_str() );
			ScreenPrompt::Prompt( SM_None, s );
		}
	}
	else
	{
		SCREENMAN->PlayInvalidSound();
	}
}

/*
 * (c) 2005 Chris Danford
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
