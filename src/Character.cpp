#include "global.h"
/*
-----------------------------------------------------------------------------
 Class: Character

 Desc: See header.

 Copyright (c) 2001-2002 by the person(s) listed below.  All rights reserved.
	Chris Danford
-----------------------------------------------------------------------------
*/

#include "Character.h"
#include "IniFile.h"
#include "RageUtil.h"
#include "arch/arch.h"


bool Character::Load( CString sCharDir )
{
	// Save character directory
	if( sCharDir.Right(1)!=SLASH )
		sCharDir += SLASH;
	m_sCharDir = sCharDir;

	// Save character name
	vector<CString> as;
	split( sCharDir, SLASH, as );
	m_sName = as.back();

	// Save attacks
	IniFile ini;
	ini.SetPath( sCharDir+"character.ini" );
	if( !ini.ReadFile() )
		return false;
	for( int i=0; i<NUM_ATTACK_LEVELS; i++ )
		for( int j=0; j<NUM_ATTACKS_PER_LEVEL; j++ )
			ini.GetValue( "Character", ssprintf("Level%dAttack%d",i+1,j+1), m_sAttacks[i][j] );

	return true;
}


CString GetRandomFileInDir( CString sDir )
{
	CStringArray asFiles;
	GetDirListing( sDir, asFiles, false, true );
	if( asFiles.empty() )
		return "";
	else
		return asFiles[rand()%asFiles.size()];
}


CString Character::GetModelPath() const
{
	CString s = m_sCharDir + "model.txt";
	if( DoesFileExist(s) )
		return s; 
	else 
		return "";
}

CString Character::GetRestAnimationPath() const	{ return DerefRedir(GetRandomFileInDir(m_sCharDir + "Rest" SLASH)); }
CString Character::GetWarmUpAnimationPath() const { return DerefRedir(GetRandomFileInDir(m_sCharDir + "WarmUp" SLASH)); }
CString Character::GetDanceAnimationPath() const { return DerefRedir(GetRandomFileInDir(m_sCharDir + "Dance" SLASH)); }
CString Character::GetTakingABreakPath() const
{
	CStringArray as;
	GetDirListing( m_sCharDir+"break.png", as, false, true );
	GetDirListing( m_sCharDir+"break.jpg", as, false, true );
	GetDirListing( m_sCharDir+"break.gif", as, false, true );
	GetDirListing( m_sCharDir+"break.bmp", as, false, true );
	if( as.empty() )
		return "";
	else
		return as[0];
}

CString Character::GetSongSelectIconPath() const
{
	CStringArray as;
	// first try and find an icon specific to the select music screen
	// so you can have different icons for music select / char select
	GetDirListing( m_sCharDir+"selectmusicicon.png", as, false, true );
	GetDirListing( m_sCharDir+"selectmusicicon.jpg", as, false, true );
	GetDirListing( m_sCharDir+"selectmusicicon.gif", as, false, true );
	GetDirListing( m_sCharDir+"selectmusicicon.bmp", as, false, true );

	if( as.empty() )
	{
		// if that failed, try using the regular icon
		GetDirListing( m_sCharDir+"icon.png", as, false, true );
		GetDirListing( m_sCharDir+"icon.jpg", as, false, true );
		GetDirListing( m_sCharDir+"icon.gif", as, false, true );
		GetDirListing( m_sCharDir+"icon.bmp", as, false, true );
		if( as.empty() )
			return "";
		else
			return as[0];
	}
	else
		return as[0];
}

CString Character::GetCardPath() const
{
	CStringArray as;
	GetDirListing( m_sCharDir+"card.png", as, false, true );
	GetDirListing( m_sCharDir+"card.jpg", as, false, true );
	GetDirListing( m_sCharDir+"card.gif", as, false, true );
	GetDirListing( m_sCharDir+"card.bmp", as, false, true );
	if( as.empty() )
		return "";
	else
		return as[0];
}

CString Character::GetIconPath() const
{
	CStringArray as;
	GetDirListing( m_sCharDir+"icon.png", as, false, true );
	GetDirListing( m_sCharDir+"icon.jpg", as, false, true );
	GetDirListing( m_sCharDir+"icon.gif", as, false, true );
	GetDirListing( m_sCharDir+"icon.bmp", as, false, true );
	if( as.empty() )
		return "";
	else
		return as[0];
}

CString Character::GetHeadPath() const
{
	CStringArray as;
	GetDirListing( m_sCharDir+"head*.png", as, false, true );
	GetDirListing( m_sCharDir+"head*.jpg", as, false, true );
	GetDirListing( m_sCharDir+"head*.gif", as, false, true );
	GetDirListing( m_sCharDir+"head*.bmp", as, false, true );
	if( as.empty() )
		return "";
	else
		return as[0];
}

