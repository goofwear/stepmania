#include "global.h"
/*
-----------------------------------------------------------------------------
 Class: BGAnimation

 Desc: Particles used initially for background effects

 Copyright (c) 2001-2002 by the person(s) listed below.  All rights reserved.
	Ben Nordstrom
	Chris Danford
-----------------------------------------------------------------------------
*/

#include "BGAnimation.h"
#include "PrefsManager.h"
#include "GameState.h"
#include "IniFile.h"
#include "BGAnimationLayer.h"
#include "RageUtil.h"
#include "song.h"
#include "ThemeManager.h"
#include "RageFile.h"

const int MAX_LAYERS = 1000;

BGAnimation::BGAnimation()
{
	m_fLengthSeconds = 10;
}

BGAnimation::~BGAnimation()
{
	Unload();
}

void BGAnimation::Unload()
{
    for( unsigned i=0; i<m_Layers.size(); i++ )
		delete m_Layers[i];
	m_Layers.clear();
}

void BGAnimation::LoadFromStaticGraphic( CString sPath )
{
	Unload();

	BGAnimationLayer* pLayer = new BGAnimationLayer;
	pLayer->LoadFromStaticGraphic( sPath );
	m_Layers.push_back( pLayer );
}

void AddLayersFromAniDir( CString sAniDir, vector<BGAnimationLayer*> &layersAddTo )
{
	if( sAniDir.empty() )
		 return;

	if( sAniDir.Right(1) != SLASH )
		sAniDir += SLASH;

	RAGE_ASSERT_M( IsADirectory(sAniDir), sAniDir + " isn't a directory" );

	CString sPathToIni = sAniDir + "BGAnimation.ini";

	IniFile ini(sPathToIni);
	ini.ReadFile();

	int i;
	for( i=0; i<MAX_LAYERS; i++ )
	{
		CString sLayer = ssprintf("Layer%d",i+1);
		const IniFile::key* pKey = ini.GetKey( sLayer );
		if( pKey == NULL )
			continue;	// skip

		CString sImportDir;
		if( ini.GetValue(sLayer, "Import", sImportDir) )
		{
			// import a whole BGAnimation
			sImportDir = sAniDir + sImportDir;
			CollapsePath( sImportDir );
			AddLayersFromAniDir( sImportDir, layersAddTo );
		}
		else
		{
			// import a single layer
			BGAnimationLayer* pLayer = new BGAnimationLayer;
			pLayer->LoadFromIni( sAniDir, sLayer );
			layersAddTo.push_back( pLayer );
		}
	}

}

void BGAnimation::LoadFromAniDir( CString sAniDir )
{
	Unload();

	if( sAniDir.empty() )
		 return;

	if( sAniDir.Right(1) != SLASH )
		sAniDir += SLASH;

	RAGE_ASSERT_M( IsADirectory(sAniDir), sAniDir + " isn't a directory" );

	CString sPathToIni = sAniDir + "BGAnimation.ini";

	if( DoesFileExist(sPathToIni) )
	{
		// This is a new style BGAnimation (using .ini)
		AddLayersFromAniDir( sAniDir, m_Layers );	// TODO: Check for circular load

		IniFile ini(sPathToIni);
		ini.ReadFile();
		if( !ini.GetValue( "BGAnimation", "LengthSeconds", m_fLengthSeconds ) )
		{
			m_fLengthSeconds = 0;
			for( int i=0; (unsigned)i < m_Layers.size(); i++ )
				m_fLengthSeconds = max(m_fLengthSeconds, m_Layers[i]->GetMaxTweenTimeLeft());
		}
	}
	else
	{
		// This is an old style BGAnimation (not using .ini)

		// loading a directory of layers
		CStringArray asImagePaths;
		ASSERT( sAniDir != "" );

		GetDirListing( sAniDir+"*.png", asImagePaths, false, true );
		GetDirListing( sAniDir+"*.jpg", asImagePaths, false, true );
		GetDirListing( sAniDir+"*.gif", asImagePaths, false, true );
		GetDirListing( sAniDir+"*.avi", asImagePaths, false, true );
		GetDirListing( sAniDir+"*.mpg", asImagePaths, false, true );
		GetDirListing( sAniDir+"*.mpeg", asImagePaths, false, true );
		GetDirListing( sAniDir+"*.sprite", asImagePaths, false, true );

		SortCStringArray( asImagePaths );

		for( unsigned i=0; i<asImagePaths.size(); i++ )
		{
			const CString sPath = asImagePaths[i];
			if( Basename(sPath).Left(1) == "_" )
				continue;	// don't directly load files starting with an underscore
			BGAnimationLayer* pLayer = new BGAnimationLayer;
			pLayer->LoadFromAniLayerFile( asImagePaths[i] );
			m_Layers.push_back( pLayer );
		}
	}
}

void BGAnimation::LoadFromMovie( CString sMoviePath )
{
	Unload();

	BGAnimationLayer* pLayer = new BGAnimationLayer;
	pLayer->LoadFromMovie( sMoviePath );
	m_Layers.push_back( pLayer );
}

void BGAnimation::LoadFromVisualization( CString sVisPath )
{
	Unload();
	BGAnimationLayer* pLayer;
	
	const Song* pSong = GAMESTATE->m_pCurSong;
	CString sSongBGPath = pSong && pSong->HasBackground() ? pSong->GetBackgroundPath() : THEME->GetPathToG("Common fallback background");

	pLayer = new BGAnimationLayer;
	pLayer->LoadFromStaticGraphic( sSongBGPath );
	m_Layers.push_back( pLayer );

	pLayer = new BGAnimationLayer;
	pLayer->LoadFromVisualization( sVisPath );
	m_Layers.push_back( pLayer );	
}


void BGAnimation::Update( float fDeltaTime )
{
	for( unsigned i=0; i<m_Layers.size(); i++ )
		m_Layers[i]->Update( fDeltaTime );
}

void BGAnimation::DrawPrimitives()
{
	for( unsigned i=0; i<m_Layers.size(); i++ )
		m_Layers[i]->Draw();
}
	
void BGAnimation::GainingFocus( float fRate, bool bRewindMovie, bool bLoop )
{
	for( unsigned i=0; i<m_Layers.size(); i++ )
		m_Layers[i]->GainingFocus( fRate, bRewindMovie, bLoop );

	SetDiffuse( RageColor(1,1,1,1) );
}

void BGAnimation::LosingFocus()
{
	for( unsigned i=0; i<m_Layers.size(); i++ )
		m_Layers[i]->LosingFocus();
}

void BGAnimation::SetDiffuse( const RageColor &c )
{
	for( unsigned i=0; i<m_Layers.size(); i++ ) 
		m_Layers[i]->SetDiffuse(c);
}

void BGAnimation::PlayOffCommand()
{
	for( unsigned i=0; i<m_Layers.size(); i++ ) 
		m_Layers[i]->PlayOffCommand();
}
