#include "global.h"
#include "MemoryCardManager.h"
#include "arch/MemoryCard/MemoryCardDriver.h"	// for UsbStorageDevice
#include "ScreenManager.h"
#include "ThemeManager.h"
#include "PrefsManager.h"
#include "RageLog.h"
#include "ScreenManager.h"
#include "ProfileManager.h"

MemoryCardManager*	MEMCARDMAN = NULL;	// global and accessable from anywhere in our program


MemoryCardManager::MemoryCardManager()
{
	m_pDriver = MakeMemoryCardDriver();
	m_bCardsLocked = false;
	FOREACH_PlayerNumber( p )
	{
		m_bTooLate[p] = false;
		m_bWriteError[p] = false;
	}
	m_pDriver->GetStorageDevices( m_vStorageDevices );

	m_bDontPlaySoundsOnce = false;

	m_soundReady.Load( THEME->GetPathToS("MemoryCardManager ready") );
	m_soundError.Load( THEME->GetPathToS("MemoryCardManager error") );
	m_soundTooLate.Load( THEME->GetPathToS("MemoryCardManager too late") );
	m_soundDisconnect.Load( THEME->GetPathToS("MemoryCardManager disconnect") );

	UpdateUnassignedCards();
}

MemoryCardManager::~MemoryCardManager()
{
	delete m_pDriver;
}

void MemoryCardManager::Update( float fDelta )
{
	if( m_pDriver->StorageDevicesChanged() )
	{
		vector<UsbStorageDevice> vOld = m_vStorageDevices;	// copy
		m_pDriver->GetStorageDevices( m_vStorageDevices );
		vector<UsbStorageDevice> &vNew = m_vStorageDevices;
		vector<UsbStorageDevice> vConnects;	// fill these in below
		vector<UsbStorageDevice> vDisconnects;	// fill these in below


		// check for disconnects
		{
			for( unsigned i=0; i<vOld.size(); i++ )
			{
				const UsbStorageDevice &old = vOld[i];
				if( find(vNew.begin(),vNew.end(),old) == vNew.end() )	// didn't find
				{
					LOG->Trace( ssprintf("Disconnected bus %d port %d device %d path %s", old.iBus, old.iPort, old.iLevel, old.sOsMountDir.c_str()) );
					vDisconnects.push_back( old );
				}
			}
		}

		// check for connects
		{
			for( unsigned i=0; i<vNew.size(); i++ )
			{
				const UsbStorageDevice &newd = vNew[i];
				if( find(vOld.begin(),vOld.end(),newd) == vOld.end() )	// didn't find
				{
					LOG->Trace( ssprintf("Connected bus %d port %d device %d path %s", newd.iBus, newd.iPort, newd.iLevel, newd.sOsMountDir.c_str()) );
					vConnects.push_back( newd );
				}
			}
		}

		// unassign cards that were disconnected
		{
			FOREACH_PlayerNumber( p )
			{
				if( m_Device[p].IsBlank() )	// not assigned a card
					continue;
				
				if( find(vDisconnects.begin(),vDisconnects.end(),m_Device[p]) != vDisconnects.end() )	// assigned card was disconnected
				{
					m_pDriver->Unmount(&m_Device[p], MEM_CARD_MOUNT_POINT[p]);

					m_Device[p].MakeBlank();
					m_soundDisconnect.Play();

					if( PROFILEMAN->ProfileWasLoadedFromMemoryCard((PlayerNumber)p) )
						PROFILEMAN->UnloadProfile( (PlayerNumber)p );
				}
			}
		}

		UpdateUnassignedCards();

		SCREENMAN->RefreshCreditsMessages();
	}
}

MemoryCardState MemoryCardManager::GetCardState( PlayerNumber pn )
{
	if( m_Device[pn].IsBlank() )
		return MEMORY_CARD_STATE_NO_CARD;
	else if( m_bTooLate[pn] )
		return MEMORY_CARD_STATE_TOO_LATE;
	else if( m_bWriteError[pn] )
		return MEMORY_CARD_STATE_WRITE_ERROR;
	else
		return MEMORY_CARD_STATE_READY;
}

/*
CString MemoryCardManager::GetOsMountDir( PlayerNumber pn )
{
	if( m_Device[pn].IsBlank() )
		return "";
	else
	{
		CString sDir = m_Device[pn].sOsMountDir;
		FixSlashesInPlace( sDir );
		if( !sDir.empty() && sDir.Right(1)!="/" )
			sDir += '/';
		return sDir;
	}
}
*/

void MemoryCardManager::LockCards( bool bLock )
{
	m_bCardsLocked = bLock;

	if( !bLock )
	{
		// clear too late flag
		FOREACH_PlayerNumber( p )
			m_bTooLate[p] = false;
	}

	if( m_bCardsLocked )
		m_pDriver->SetMountThreadState( MemoryCardDriver::detect_and_dont_mount );
	else
		m_pDriver->SetMountThreadState( MemoryCardDriver::detect_and_mount );
}

void MemoryCardManager::UpdateUnassignedCards()
{
	LOG->Trace( "UpdateUnassignedCards" );

	// make a list of unassigned
	vector<UsbStorageDevice> vUnassignedDevices = m_vStorageDevices;	// copy

	// remove cards that are already assigned
	{
		FOREACH_PlayerNumber( p )
		{
			if( m_Device[p].IsBlank() )	// no card assigned to this player
				continue;

			vector<UsbStorageDevice>::iterator it = find(vUnassignedDevices.begin(),vUnassignedDevices.end(),m_Device[p]);
			if( it != vUnassignedDevices.end() )
			{
				// TRICKY: Update m_Device with the latest info we received from the 
				// driver.  It may contain updated info like WriteTest results and a
				// sName.
				m_Device[p] = *it;

				vUnassignedDevices.erase( it );
			}
		}
	}

	// try to assign each device to a player
	{
		FOREACH_PlayerNumber( p )
		{
			LOG->Trace( "Looking for a card for Player %d", p+1 );

			if( !m_Device[p].IsBlank() )	// they already have an assigned card
			{
				LOG->Trace( "Player %d already has a card: '%s'", p+1, m_Device[p].sOsMountDir.c_str() );
				continue;	// skip
			}

			unsigned i;

			// search for card dir match
			if( !PREFSMAN->m_sMemoryCardOsMountPoint[p].empty() )
			{
				for( i=0; i<vUnassignedDevices.size(); i++ )
				{
					UsbStorageDevice &usbd = vUnassignedDevices[i];
					if( usbd.sOsMountDir.CompareNoCase(PREFSMAN->m_sMemoryCardOsMountPoint[p]) == 0 )	// match
					{
						LOG->Trace( "dir match:  iScsiIndex: %d, iBus: %d, iLevel: %d, iPort: %d",
							usbd.iScsiIndex, usbd.iBus, usbd.iLevel, usbd.iPort );
						goto match;
					}
				}
			}

			// search for USB bus match
			for( i=0; i<vUnassignedDevices.size(); i++ )
			{
				UsbStorageDevice &usbd = vUnassignedDevices[i];

				if( PREFSMAN->m_iMemoryCardUsbBus[p] != -1 && 
					PREFSMAN->m_iMemoryCardUsbBus[p] != usbd.iBus )
					continue;	// not a match

				if( PREFSMAN->m_iMemoryCardUsbPort[p] != -1 && 
					PREFSMAN->m_iMemoryCardUsbPort[p] != usbd.iPort )
					continue;	// not a match

				if( PREFSMAN->m_iMemoryCardUsbLevel[p] != -1 && 
					PREFSMAN->m_iMemoryCardUsbLevel[p] != usbd.iLevel )
					continue;	// not a match

				LOG->Trace( "bus/port match:  iScsiIndex: %d, iBus: %d, iLevel: %d, iPort: %d",
					usbd.iScsiIndex, usbd.iBus, usbd.iLevel, usbd.iPort );
				goto match;
			}
			
			LOG->Trace( "Player %d memory card: none", p+1 );
			continue;	// no matches
match:
			m_Device[p] = vUnassignedDevices[i];	// save a copy
			LOG->Trace( "Player %d memory card: '%s'", p+1, m_Device[p].sOsMountDir.c_str() );
			vUnassignedDevices.erase( vUnassignedDevices.begin()+i );	// remove the device so we don't match it for another player
			m_bTooLate[p] = m_bCardsLocked;
			m_bWriteError[p] = false;
			if( !m_bCardsLocked )
			{
				if( !m_pDriver->MountAndTestWrite(&m_Device[p], MEM_CARD_MOUNT_POINT[p]) )
				{
					m_bWriteError[p] = true;
				}
			}

			if( !m_bDontPlaySoundsOnce )
			{
				// play sound
				if( m_bWriteError[p] )
					m_soundError.Play();
				else if( m_bTooLate[p] )
					m_soundTooLate.Play();
				else
					m_soundReady.Play();
			}
		}
	}

	m_bDontPlaySoundsOnce = false;
}

void MemoryCardManager::FlushAndReset()
{
	FOREACH_PlayerNumber( p )
	{
		if( m_Device[p].IsBlank() )	// no card assigned
			continue;	// skip
		if( m_bWriteError[p] || m_bTooLate[p] )
			continue;	// skip
		m_pDriver->Flush(&m_Device[p]);
	}

	m_pDriver->ResetUsbStorage();	// forces cards to be re-detected

	m_bDontPlaySoundsOnce = true;
}

bool MemoryCardManager::PathIsMemCard( CString sDir ) const
{
	FOREACH_PlayerNumber( p )
		if( !sDir.Left(MEM_CARD_MOUNT_POINT[p].size()).CompareNoCase( MEM_CARD_MOUNT_POINT[p] ) )
			return true;
	return false;
}

CString MemoryCardManager::GetName( PlayerNumber pn ) const
{
	return m_Device[pn].sName;
}

void MemoryCardManager::PauseMountingThread()
{
	m_pDriver->SetMountThreadState( MemoryCardDriver::paused );
}

void MemoryCardManager::UnPauseMountingThread()
{
	m_pDriver->SetMountThreadState( 
		m_bCardsLocked ? 
		MemoryCardDriver::detect_and_dont_mount : 
		MemoryCardDriver::detect_and_mount );
}

bool IsAnyPlayerUsingMemoryCard()
{
	FOREACH_HumanPlayer( pn )
		if( MEMCARDMAN->GetCardState(pn) == MEMORY_CARD_STATE_READY )
			return true;
	return false;
}

#include "LuaFunctions.h"
LuaFunction_NoArgs( IsAnyPlayerUsingMemoryCard,		IsAnyPlayerUsingMemoryCard() )

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
