#include "stdafx.h"
/*
-----------------------------------------------------------------------------
 Class: SongManager

 Desc: See header.

 Copyright (c) 2001-2002 by the person(s) listed below.  All rights reserved.
	Chris Danford
-----------------------------------------------------------------------------
*/

//#include <d3dxmath.h>
#include "SongManager.h"
#include "IniFile.h"
#include "RageLog.h"
#include "NotesLoaderDWI.h"

#include "GameState.h"
#include "PrefsManager.h"
#include "RageException.h"
#include "RageTimer.h"

SongManager*	SONGMAN = NULL;	// global and accessable from anywhere in our program


const CString g_sStatisticsFileName = "statistics.ini";

#define NUM_GROUP_COLORS	THEME->GetMetricI("SongManager","NumGroupColors")
#define GROUP_COLOR( i )	THEME->GetMetricC("SongManager",ssprintf("GroupColor%d",i+1))
#define EXTRA_COLOR			THEME->GetMetricC("SongManager","ExtraColor")

D3DXCOLOR g_GroupColors[30];
D3DXCOLOR g_ExtraColor;


SongManager::SongManager( void(*callback)() )
{
	for( int i=0; i<NUM_GROUP_COLORS; i++ )
		g_GroupColors[i] = GROUP_COLOR( i );
	g_ExtraColor = EXTRA_COLOR;

	InitSongArrayFromDisk( callback );
	ReadStatisticsFromDisk();

	InitCoursesFromDisk();
}

SongManager::~SongManager()
{
	SaveStatisticsToDisk();
	FreeSongArray();
}


void SongManager::InitSongArrayFromDisk( void(*callback)() )
{
	LoadStepManiaSongDir( "Songs", callback );

	for( int i=0; i<PREFSMAN->m_asAdditionalSongFolders.GetSize(); i++ )
        LoadStepManiaSongDir( PREFSMAN->m_asAdditionalSongFolders[i], callback );

	if( PREFSMAN->m_DWIPath != "" )
		LoadStepManiaSongDir( PREFSMAN->m_DWIPath + "\\Songs", callback );

	LOG->Trace( "Found %d Songs.", m_pSongs.GetSize() );
}

void SongManager::SanityCheckGroupDir( CString sDir ) const
{
	// Check to see if they put a song directly inside the group folder.
	CStringArray arrayFiles;
	GetDirListing( ssprintf("%s\\*.mp3", sDir), arrayFiles );
	GetDirListing( ssprintf("%s\\*.ogg", sDir), arrayFiles );
	GetDirListing( ssprintf("%s\\*.wav", sDir), arrayFiles );
	if( arrayFiles.GetSize() > 0 )
		throw RageException( 
			ssprintf( "The folder '%s' contains music files.\n\n"
				"This probably means that you have a song outside of a group folder.\n"
				"All song folders must reside in a group folder.  For example, 'Songs\\DDR 4th Mix\\B4U'.\n"
				"See the StepMania readme for more info.",
				ssprintf("%s\\%s", sDir ) )
		);
	
}

void SongManager::AddGroup( CString sDir, CString sGroupDirName )
{
	int j;
	for(j = 0; j < m_arrayGroupNames.GetSize(); ++j)
		if( sGroupDirName == m_arrayGroupNames[j] ) break;

	if( j != m_arrayGroupNames.GetSize() )
		return; /* the group is already added */

	// Look for a group banner in this group folder
	CStringArray arrayGroupBanners;
	GetDirListing( ssprintf("%s\\%s\\*.png", sDir, sGroupDirName), arrayGroupBanners );
	GetDirListing( ssprintf("%s\\%s\\*.jpg", sDir, sGroupDirName), arrayGroupBanners );
	GetDirListing( ssprintf("%s\\%s\\*.gif", sDir, sGroupDirName), arrayGroupBanners );
	GetDirListing( ssprintf("%s\\%s\\*.bmp", sDir, sGroupDirName), arrayGroupBanners );
	CString sBannerPath;

	if( arrayGroupBanners.GetSize() > 0 )
	{
		sBannerPath = ssprintf("%s\\%s\\%s", sDir, sGroupDirName, arrayGroupBanners[0] );
		LOG->Trace( ssprintf("Group banner for '%s' is '%s'.", sGroupDirName, sBannerPath) );
	}

	m_arrayGroupNames.Add( sGroupDirName );
	m_GroupBannerPaths.Add(sBannerPath);
}

void SongManager::LoadStepManiaSongDir( CString sDir, void(*callback)() )
{
	// trim off the trailing slash if any
	sDir.TrimRight( "/\\" );

	// Find all group directories in "Songs" folder
	CStringArray arrayGroupDirs;
	GetDirListing( sDir+"\\*.*", arrayGroupDirs, true );
	SortCStringArray( arrayGroupDirs );
	
	for( int i=0; i< arrayGroupDirs.GetSize(); i++ )	// for each dir in /Songs/
	{
		CString sGroupDirName = arrayGroupDirs[i];

		if( 0 == stricmp( sGroupDirName, "cvs" ) )	// the directory called "CVS"
			continue;		// ignore it

		SanityCheckGroupDir(sDir+"\\"+sGroupDirName);

		// Find all Song folders in this group directory
		CStringArray arraySongDirs;
		GetDirListing( ssprintf("%s\\%s\\*.*", sDir, sGroupDirName), arraySongDirs, true );
		SortCStringArray( arraySongDirs );

		int j;
		int loaded = 0;

		for( j=0; j< arraySongDirs.GetSize(); j++ )	// for each song dir
		{
			CString sSongDirName = arraySongDirs[j];

			if( 0 == stricmp( sSongDirName, "cvs" ) )	// the directory called "CVS"
				continue;		// ignore it

			// this is a song directory.  Load a new song!
			GAMESTATE->m_sLoadingMessage = ssprintf("Loading songs...\n%s\n%s", sGroupDirName, sSongDirName);
			if( callback )
				callback();
			Song* pNewSong = new Song;
			if( !pNewSong->LoadFromSongDir( ssprintf("%s\\%s\\%s", sDir, sGroupDirName, sSongDirName) ) ) {
				/* The song failed to load. */
				delete pNewSong;
				continue;
			}
			
            m_pSongs.Add( pNewSong );
			loaded++;
		}

		/* Don't add the group name if we didn't load any songs in this group. */
		if(!loaded) continue;

		/* Add this group to the group array. */
		AddGroup(sDir, sGroupDirName);
	}

	GAMESTATE->m_sLoadingMessage = "Done loading songs.";
	if( callback )
		callback();
}

/*

void SongManager::LoadDWISongDir( CString DWIHome )
{
	// trim off the trailing slash if any
	DWIHome.TrimRight( "/\\" );

	// this has to be fixed, DWI doesn't put files
	// in it's DWI folder.  It puts them in Songs/<MIX>/<SONGNAME>
	// so what we have to do, is go to the DWI directory (which will
	// be changeable by the user so they don't have to copy everything
	// over and have two copies of everything
	// Find all directories in "DWIs" folder
	CStringArray arrayDirs;
	CStringArray MixDirs;
	// now we've got the listing of the mix directories
	// and we need to use THOSE directories to find our
	// dwis
	GetDirListing( ssprintf("%s\\Songs\\*.*", DWIHome ), MixDirs, true );
	SortCStringArray( MixDirs );
	
	for( int i=0; i< MixDirs.GetSize(); i++ )	// for each dir in /Songs/
	{
		// the dir name will most likely be something like
		// Dance Dance Revolution 4th Mix, etc.
		CString sDirName = MixDirs[i];
		sDirName.MakeLower();
		if( sDirName == "cvs" )	// ignore the directory called "CVS"
			continue;
		GetDirListing( ssprintf("%s\\Songs\\%s\\*.*", DWIHome, MixDirs[i]), arrayDirs,  true);
		SortCStringArray(arrayDirs, true);

		for( int b = 0; b < arrayDirs.GetSize(); b++)
		{
			// Find all DWIs in this directory
			CStringArray arrayDWIFiles;
			GetDirListing( ssprintf("%s\\Songs\\%s\\%s\\*.dwi", DWIHome, MixDirs[i], arrayDirs[b]), arrayDWIFiles, false);
			SortCStringArray( arrayDWIFiles );

			for( int j=0; j< arrayDWIFiles.GetSize(); j++ )	// for each DWI file
			{
				CString sDWIFileName = arrayDWIFiles[j];
				sDWIFileName.MakeLower();

				// load DWIs from the sub dirs
				DWILoader ld;
				Song* pNewSong = new Song;
				ld.LoadFromDWIFile(
					ssprintf("%s\\Songs\\%s\\%s\\%s", DWIHome, MixDirs[i], arrayDirs[b], sDWIFileName),
					*pNewSong);
				m_pSongs.Add( pNewSong );
			}
		}
	}
}
*/


void SongManager::FreeSongArray()
{
	for( int i=0; i<m_pSongs.GetSize(); i++ )
		SAFE_DELETE( m_pSongs[i] );
	m_pSongs.RemoveAll();

	m_GroupBannerPaths.RemoveAll();
}


void SongManager::ReloadSongArray()
{
	InitSongArrayFromDisk( NULL );
	FreeSongArray();
}



void SongManager::ReadStatisticsFromDisk()
{
	IniFile ini;
	ini.SetPath( g_sStatisticsFileName );
	if( !ini.ReadFile() ) {
		LOG->Trace( "WARNING: Could not read config file '%s'.", g_sStatisticsFileName );
		return;		// load nothing
	}


	// load song statistics
	IniFile::key* pKey = ini.GetKey( "Statistics" );
	if( pKey )
	{
		for( int i=0; i<pKey->names.GetSize(); i++ )
		{
			CString name = pKey->names[i];
			CString value = pKey->values[i];

			// Each value has the format "SongName::StepsName=TimesPlayed::TopGrade::TopScore::MaxCombo".
			char szSongDir[256];
			char szNotesName[256];
			int iRetVal;
			int i;

			// Parse for Song name and Notes name
			iRetVal = sscanf( name, "%[^:]::%[^\n]", szSongDir, szNotesName );
			if( iRetVal != 2 )
				continue;	// this line doesn't match what is expected
	
			
			// Search for the corresponding Song pointer.
			Song* pSong = NULL;
			for( i=0; i<m_pSongs.GetSize(); i++ )
			{
				if( m_pSongs[i]->m_sSongDir == szSongDir )	// match!
				{
					pSong = m_pSongs[i];
					break;
				}
			}
			if( pSong == NULL )	// didn't find a match
				continue;	// skip this entry


			// Search for the corresponding Notes pointer.
			Notes* pNotes = NULL;
			for( i=0; i<pSong->m_apNotes.GetSize(); i++ )
			{
				if( pSong->m_apNotes[i]->m_sDescription == szNotesName )	// match!
				{
					pNotes = pSong->m_apNotes[i];
					break;
				}
			}
			if( pNotes == NULL )	// didn't find a match
				continue;	// skip this entry


			// Parse the Notes statistics.
			char szGradeLetters[10];	// longest possible string is "AAA"

			iRetVal = sscanf( 
				value, 
				"%d::%[^:]::%d::%d", 
				&pNotes->m_iNumTimesPlayed,
				szGradeLetters,
				&pNotes->m_iTopScore,
				&pNotes->m_iMaxCombo
			);
			if( iRetVal != 4 )
				continue;

			pNotes->m_TopGrade = StringToGrade( szGradeLetters );
		}
	}
}

void SongManager::SaveStatisticsToDisk()
{
	IniFile ini;
	ini.SetPath( g_sStatisticsFileName );

	// save song statistics
	for( int i=0; i<m_pSongs.GetSize(); i++ )		// for each Song
	{
		Song* pSong = m_pSongs[i];

		for( int j=0; j<pSong->m_apNotes.GetSize(); j++ )		// for each Notes
		{
			Notes* pNotes = pSong->m_apNotes[j];

			if( pNotes->m_TopGrade == GRADE_NO_DATA )
				continue;		// skip

			// Each value has the format "SongName::NotesName=TimesPlayed::TopGrade::TopScore::MaxCombo".

			CString sName = ssprintf( "%s::%s", pSong->m_sSongDir, pNotes->m_sDescription );
			CString sValue = ssprintf( 
				"%d::%s::%d::%d",
				pNotes->m_iNumTimesPlayed,
				GradeToString( pNotes->m_TopGrade ),
				pNotes->m_iTopScore, 
				pNotes->m_iMaxCombo
			);

			ini.SetValue( "Statistics", sName, sValue );
		}
	}

	ini.WriteFile();
}


CString SongManager::GetGroupBannerPath( CString sGroupName )
{
	int i;
	for(i = 0; i < m_arrayGroupNames.GetSize(); ++i)
		if( sGroupName == m_arrayGroupNames[i] ) break;

	if( i == m_arrayGroupNames.GetSize() )
		return "";

	return m_GroupBannerPaths[i];
}

void SongManager::GetGroupNames( CStringArray &AddTo )
{
	AddTo.Copy( m_arrayGroupNames );
}

D3DXCOLOR SongManager::GetGroupColor( const CString &sGroupName )
{
	// search for the group index
	for( int i=0; i<m_arrayGroupNames.GetSize(); i++ )
	{
		if( m_arrayGroupNames[i] == sGroupName )
			break;
	}
	ASSERT( i != m_arrayGroupNames.GetSize() );	// this is not a valid group

	return g_GroupColors[i%NUM_GROUP_COLORS];
}

D3DXCOLOR SongManager::GetSongColor( Song* pSong )
{
	ASSERT( pSong );
	for( int i=0; i<pSong->m_apNotes.GetSize(); i++ )
	{
		Notes* pNotes = pSong->m_apNotes[i];
		if( pNotes->m_iMeter == 10 )
			return EXTRA_COLOR;
	}

	return GetGroupColor( pSong->m_sGroupName );
}


void SongManager::GetSongsInGroup( const CString sGroupName, CArray<Song*, Song*> &AddTo )
{
	for( int i=0; i<m_pSongs.GetSize(); i++ )
	{
		Song* pSong = m_pSongs[i];
		if( sGroupName == m_pSongs[i]->m_sGroupName )
			AddTo.Add( pSong );
	}
	SortSongPointerArrayByGroup( AddTo );
}

CString SongManager::ShortenGroupName( const CString &sOrigGroupName )
{
	CString sShortName = sOrigGroupName;
	sShortName.Replace( "Dance Dance Revolution", "DDR" );
	sShortName.Replace( "dance dance revolution", "DDR" );
	sShortName.Replace( "DANCE DANCE REVOLUTION", "DDR" );
	sShortName.Replace( "Pump It Up", "Pump" );
	sShortName.Replace( "pump it up", "pump" );
	sShortName.Replace( "PUMP IT UP", "PUMP" );
	return sShortName;
}

void SongManager::InitCoursesFromDisk()
{
	//
	// Load courses from CRS files
	//
	CStringArray saCourseFiles;
	GetDirListing( "Courses\\*.crs", saCourseFiles );
	for( int i=0; i<saCourseFiles.GetSize(); i++ )
	{
		Course course;
		course.LoadFromCRSFile( "Courses\\" + saCourseFiles[i], m_pSongs );
		if( course.m_iStages > 0 )
			m_aOniCourses.Add( course );
	}
	
	//
	// Create endless courses
	//
	CStringArray saGroupNames;
	this->GetGroupNames( saGroupNames );
	for( int g=0; g<saGroupNames.GetSize(); g++ )	// foreach Group
	{
		CString sGroupName = saGroupNames[g];
		CArray<Song*, Song*> apGroupSongs;
		GetSongsInGroup( sGroupName, apGroupSongs );

		for( DifficultyClass dc=CLASS_EASY; dc<=CLASS_HARD; dc=DifficultyClass(dc+1) )	// foreach DifficultyClass
		{
			Course course;
			course.CreateEndlessCourseFromGroupAndDifficultyClass( sGroupName, dc, apGroupSongs );

			if( course.m_iStages > 0 )
				m_aEndlessCourses.Add( course );
		}
	}


	//
	// Load extra stages
	//
	for( g=0; g<saGroupNames.GetSize(); g++ )	// foreach Group
	{
		CString sGroupName = saGroupNames[g];
		CStringArray saCourseFiles;
		GetDirListing( "Songs\\" + sGroupName + "\\*.crs", saCourseFiles );
		for( int i=0; i<saCourseFiles.GetSize(); i++ )
		{
			Course course;
			course.LoadFromCRSFile( "Songs\\" + sGroupName + "\\" + saCourseFiles[i], m_pSongs );
			if( course.m_iStages > 0 )
				m_aExtraCourses.Add( course );
		}

	}



}

void SongManager::ReloadCourses()
{

}

bool SongManager::GetExtraStageInfoFromCourse( bool bExtra2, CString sPreferredGroup,
								   Song*& pSongOut, Notes*& pNotesOut, PlayerOptions& po_out, SongOptions& so_out )
{
	CString sCoursePath = "Songs\\" + sPreferredGroup + "\\" + (bExtra2 ? "extra2" : "extra1") + ".crs";
	if( !DoesFileExist(sCoursePath) ) return false;

	Course course;
	course.LoadFromCRSFile( sCoursePath, m_pSongs );
	if( course.m_iStages <= 0 ) return false;

	pSongOut = course.m_apSongs[0];
	pNotesOut = course.GetNotesForStage( 0 );
	if( pNotesOut == NULL ) return false;
	
	po_out.FromString( course.m_asModifiers[0] );
	so_out.FromString( course.m_asModifiers[0] );

	return true;
}

void SongManager::GetExtraStageInfo( bool bExtra2, CString sPreferredGroup, const StyleDef *sd, 
								   Song*& pSongOut, Notes*& pNotesOut, PlayerOptions& po_out, SongOptions& so_out )
{
	if(GetExtraStageInfoFromCourse(bExtra2, sPreferredGroup,
			pSongOut, pNotesOut, po_out, so_out))
		return;
	
	// Choose a hard song for the extra stage
	Song*	pExtra1Song = NULL;		// the absolute hardest Song and Notes.  Use this for extra stage 1.
	Notes*	pExtra1Notes = NULL;
	Song*	pExtra2Song = NULL;		// a medium-hard Song and Notes.  Use this for extra stage 2.
	Notes*	pExtra2Notes = NULL;
	
	CArray<Song*,Song*> apSongs;
	SONGMAN->GetSongsInGroup( GAMESTATE->m_sPreferredGroup, apSongs );
	for( int s=0; s<apSongs.GetSize(); s++ )	// foreach song
	{
		Song* pSong = apSongs[s];

		CArray<Notes*,Notes*> apNotes;
		pSong->GetNotesThatMatch( sd, 0, apNotes );
		for( int n=0; n<apNotes.GetSize(); n++ )	// foreach Notes
		{
			Notes* pNotes = apNotes[n];

			if( pExtra1Notes == NULL  ||  CompareNotesPointersByDifficulty(pExtra1Notes,pNotes) == -1 )	// pNotes is harder than pHardestNotes
			{
				pExtra1Song = pSong;
				pExtra1Notes = pNotes;
			}

			// for extra 2, we don't want to choose the hardest notes possible.  So, we'll disgard Notes with meter > 8
			if(	bExtra2  &&  pNotes->m_iMeter > 8 )	
				continue;	// skip
			if( pExtra2Notes == NULL  ||  CompareNotesPointersByDifficulty(pExtra2Notes,pNotes) == -1 )	// pNotes is harder than pHardestNotes
			{
				pExtra2Song = pSong;
				pExtra2Notes = pNotes;
			}
		}
	}

	if( pExtra2Song == NULL  &&  pExtra1Song != NULL )
	{
		pExtra2Song = pExtra1Song;
		pExtra2Notes = pExtra1Notes;
	}

	// If there are any notes at all that match this NotesType, everything should be filled out.
	// Also, it's guaranteed that there is at least one Notes that matches the NotesType because the player
	// had to play something before reaching the extra stage!
	ASSERT( pExtra2Song && pExtra1Song && pExtra2Notes && pExtra1Notes );

	pSongOut = (bExtra2 ? pExtra2Song : pExtra1Song);
	pNotesOut = (bExtra2 ? pExtra2Notes : pExtra1Notes);


	po_out.Init();
	so_out.Init();
	po_out.m_bReverseScroll = true;
	po_out.m_fArrowScrollSpeed = 1.5f;
	so_out.m_DrainType = (bExtra2 ? SongOptions::DRAIN_SUDDEN_DEATH : SongOptions::DRAIN_NO_RECOVER);

	// should we do something fancy here, like turn on different effects?
	int iSongHash = GetHashForString( pSongOut->m_sSongDir );
	switch( ((UINT)iSongHash) % 6 )
	{
	case 0:	po_out.m_EffectType = PlayerOptions::EFFECT_DIZZY;	break;
	case 1:	po_out.m_bDark = true;								break;
	case 2:	po_out.m_EffectType = PlayerOptions::EFFECT_DRUNK;	break;
	case 3:	po_out.m_EffectType = PlayerOptions::EFFECT_MINI;	break;
	case 4:	po_out.m_EffectType = PlayerOptions::EFFECT_SPACE;	break;
	case 5:	po_out.m_EffectType = PlayerOptions::EFFECT_WAVE;	break;
	default:	ASSERT(0);
	}
}

/* Pick a random song (for the demonstration). */
bool SongManager::ChooseRandomSong()
{
	if( SONGMAN->m_pSongs.GetSize() == 0 )
		return false;

	int i;
	for( i=0; i<600; i++ )	// try 600 times
	{
		Song* pSong = m_pSongs[ rand()%m_pSongs.GetSize() ];
		for( int j=0; j<3; j++ )	// try 3 times
		{
			if( pSong->m_apNotes.GetSize() == 0 )
				continue;

			/* XXX: This assumes we're set to a style that doesn't use separate
			 * notes for each player, such as VERSUS. */
			Notes* pNotes = pSong->m_apNotes[ rand()%pSong->m_apNotes.GetSize() ];
			if( pNotes->m_NotesType != GAMESTATE->GetCurrentStyleDef()->m_NotesTypes[0] )
				continue;

			// found something we can use!
			GAMESTATE->m_pCurSong = pSong;
			for( int p=0; p<NUM_PLAYERS; p++ )
				GAMESTATE->m_pCurNotes[p] = pNotes;
			return true;
		}
	}

	return false;
}
