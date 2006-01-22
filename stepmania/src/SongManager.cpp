#include "global.h"
#include "SongManager.h"
#include "RageLog.h"
#include "MsdFile.h"
#include "NotesLoaderDWI.h"
#include "BannerCache.h"

#include "GameState.h"
#include "PrefsManager.h"
#include "RageException.h"
#include "arch/LoadingWindow/LoadingWindow.h"
#include "Course.h"

#include "AnnouncerManager.h"
#include "ThemeManager.h"
#include "GameManager.h"
#include "RageFile.h"
#include "Sprite.h"
#include "ProfileManager.h"
#include "MemoryCardManager.h"
#include "NotesLoaderSM.h"
#include "song.h"
#include "SongUtil.h"
#include "Steps.h"
#include "StepsUtil.h"
#include "CourseUtil.h"
#include "TrailUtil.h"
#include "RageFileManager.h"
#include "UnlockManager.h"
#include "Foreach.h"
#include "StatsManager.h"
#include "Style.h"
#include "BackgroundUtil.h"
#include "Profile.h"
#include "CourseLoaderCRS.h"
#include "TitleSubstitution.h"
#include "LocalizedString.h"

SongManager*	SONGMAN = NULL;	// global and accessable from anywhere in our program

const RString SONGS_DIR		= "Songs/";
const RString COURSES_DIR	= "Courses/";

static const ThemeMetric<RageColor> EXTRA_COLOR			("SongManager","ExtraColor");
static const ThemeMetric<int>		EXTRA_COLOR_METER	("SongManager","ExtraColorMeter");

RString SONG_GROUP_COLOR_NAME( size_t i )   { return ssprintf("SongGroupColor%i",(int) i+1); }
RString COURSE_GROUP_COLOR_NAME( size_t i ) { return ssprintf("CourseGroupColor%i",(int) i+1); }


SongManager::SongManager()
{
	NUM_SONG_GROUP_COLORS	.Load("SongManager","NumSongGroupColors");
	SONG_GROUP_COLOR		.Load("SongManager",SONG_GROUP_COLOR_NAME,NUM_SONG_GROUP_COLORS);
	NUM_COURSE_GROUP_COLORS	.Load("SongManager","NumCourseGroupColors");
	COURSE_GROUP_COLOR		.Load("SongManager",COURSE_GROUP_COLOR_NAME,NUM_COURSE_GROUP_COLORS);
}

SongManager::~SongManager()
{
	// Courses depend on Songs and Songs don't depend on Courses.
	// So, delete the Courses first.
	FreeCourses();
	FreeSongs();
}

void SongManager::InitAll( LoadingWindow *ld )
{
	InitSongsFromDisk( ld );
	InitCoursesFromDisk( ld );
	InitAutogenCourses();
}

static LocalizedString RELOADING ( "SongManager", "Reloading..." );
void SongManager::Reload( LoadingWindow *ld )
{
	FlushDirCache();

	if( ld )
		ld->SetText( RELOADING );

	// save scores before unloading songs, of the scores will be lost
	PROFILEMAN->SaveMachineProfile();

	FreeSongs();
	FreeCourses();

	/* Always check songs for changes. */
	const bool OldVal = PREFSMAN->m_bFastLoad;
	PREFSMAN->m_bFastLoad.Set( false );

	InitAll( ld );

	// reload scores afterward
	PROFILEMAN->LoadMachineProfile();

	PREFSMAN->m_bFastLoad.Set( OldVal );
}

void SongManager::InitSongsFromDisk( LoadingWindow *ld )
{
	RageTimer tm;
	LoadStepManiaSongDir( SONGS_DIR, ld );
	LOG->Trace( "Found %d songs in %f seconds.", (int)m_pSongs.size(), tm.GetDeltaTime() );
}


static LocalizedString FOLDER_CONTAINS_MUSIC_FILES( "SongManager", "The folder '%s' appears to be a song folder.  All song folders must reside in a group folder.  For example, 'Songs/Originals/My Song'." );
void SongManager::SanityCheckGroupDir( RString sDir ) const
{
	// Check to see if they put a song directly inside the group folder.
	vector<RString> arrayFiles;
	GetDirListing( sDir + "/*.mp3", arrayFiles );
	GetDirListing( sDir + "/*.ogg", arrayFiles );
	GetDirListing( sDir + "/*.wav", arrayFiles );
	if( !arrayFiles.empty() )
		RageException::Throw( FOLDER_CONTAINS_MUSIC_FILES.GetValue(), sDir.c_str() );	
}

void SongManager::AddGroup( RString sDir, RString sGroupDirName )
{
	unsigned j;
	for(j = 0; j < m_sSongGroupNames.size(); ++j)
		if( sGroupDirName == m_sSongGroupNames[j] ) break;

	if( j != m_sSongGroupNames.size() )
		return; /* the group is already added */

	// Look for a group banner in this group folder
	vector<RString> arrayGroupBanners;
	GetDirListing( sDir+sGroupDirName+"/*.png", arrayGroupBanners );
	GetDirListing( sDir+sGroupDirName+"/*.jpg", arrayGroupBanners );
	GetDirListing( sDir+sGroupDirName+"/*.gif", arrayGroupBanners );
	GetDirListing( sDir+sGroupDirName+"/*.bmp", arrayGroupBanners );

	RString sBannerPath;
	if( !arrayGroupBanners.empty() )
		sBannerPath = sDir+sGroupDirName+"/"+arrayGroupBanners[0] ;
	else
	{
		// Look for a group banner in the parent folder
		GetDirListing( sDir+sGroupDirName+".png", arrayGroupBanners );
		GetDirListing( sDir+sGroupDirName+".jpg", arrayGroupBanners );
		GetDirListing( sDir+sGroupDirName+".gif", arrayGroupBanners );
		GetDirListing( sDir+sGroupDirName+".bmp", arrayGroupBanners );
		if( !arrayGroupBanners.empty() )
			sBannerPath = sDir+arrayGroupBanners[0];
	}

	LOG->Trace( "Group banner for '%s' is '%s'.", sGroupDirName.c_str(), 
				sBannerPath != ""? sBannerPath.c_str():"(none)" );
	m_sSongGroupNames.push_back( sGroupDirName );
	m_sSongGroupBannerPaths.push_back( sBannerPath );
}

static LocalizedString LOADING_SONGS ( "SongManager", "Loading songs..." );
void SongManager::LoadStepManiaSongDir( RString sDir, LoadingWindow *ld )
{
	/* Make sure sDir has a trailing slash. */
	if( sDir.Right(1) != "/" )
		sDir += "/";

	// Find all group directories in "Songs" folder
	vector<RString> arrayGroupDirs;
	GetDirListing( sDir+"*", arrayGroupDirs, true );
	SortRStringArray( arrayGroupDirs );
	StripCvs( arrayGroupDirs );

	FOREACH_CONST( RString, arrayGroupDirs, s )	// foreach dir in /Songs/
	{
		RString sGroupDirName = *s;

		SanityCheckGroupDir(sDir+sGroupDirName);

		// Find all Song folders in this group directory
		vector<RString> arraySongDirs;
		GetDirListing( sDir+sGroupDirName + "/*", arraySongDirs, true, true );
		StripCvs( arraySongDirs );
		SortRStringArray( arraySongDirs );

		LOG->Trace("Attempting to load %i songs from \"%s\"", int(arraySongDirs.size()),
				   (sDir+sGroupDirName).c_str() );
		int loaded = 0;

		for( unsigned j=0; j< arraySongDirs.size(); ++j )	// for each song dir
		{
			RString sSongDirName = arraySongDirs[j];

			// this is a song directory.  Load a new song!
			if( ld )
			{
				ld->SetText( LOADING_SONGS.GetValue()+ssprintf("\n%s\n%s",
									  Basename(sGroupDirName).c_str(),
									  Basename(sSongDirName).c_str()));
				ld->Paint();
			}
			Song* pNewSong = new Song;
			if( !pNewSong->LoadFromSongDir( sSongDirName ) )
			{
				/* The song failed to load. */
				delete pNewSong;
				continue;
			}
			
            m_pSongs.push_back( pNewSong );
			loaded++;
		}

		LOG->Trace("Loaded %i songs from \"%s\"", loaded, (sDir+sGroupDirName).c_str() );

		/* Don't add the group name if we didn't load any songs in this group. */
		if(!loaded) continue;

		/* Add this group to the group array. */
		AddGroup(sDir, sGroupDirName);

		/* Cache and load the group banner. */
		BANNERCACHE->CacheBanner( GetSongGroupBannerPath(sGroupDirName) );
		
		/* Load the group sym links (if any)*/
		LoadGroupSymLinks(sDir, sGroupDirName);
	}
}

// Instead of "symlinks", songs should have membership in multiple groups.
// -Chris
void SongManager::LoadGroupSymLinks(RString sDir, RString sGroupFolder)
{
	// Find all symlink files in this folder
	vector<RString> arraySymLinks;
	GetDirListing( sDir+sGroupFolder+"/*.include", arraySymLinks, false );
	SortRStringArray( arraySymLinks );
	for( unsigned s=0; s< arraySymLinks.size(); s++ )	// for each symlink in this dir, add it in as a song.
	{
		MsdFile		msdF;
		msdF.ReadFile( sDir+sGroupFolder+"/"+arraySymLinks[s].c_str() );
		RString	sSymDestination = msdF.GetParam(0,1);	// Should only be 1 vale&param...period.
		
		Song* pNewSong = new Song;
		if( !pNewSong->LoadFromSongDir( sSymDestination ) )
		{
			delete pNewSong; // The song failed to load.
		}
		else
		{
			const vector<Steps*>& vpSteps = pNewSong->GetAllSteps();
			while( vpSteps.size() )
				pNewSong->DeleteSteps( vpSteps[0] );

			FOREACH_BackgroundLayer( i )
				pNewSong->GetBackgroundChanges(i).clear();

			pNewSong->m_bIsSymLink = true;	// Very important so we don't double-parse later
			pNewSong->m_sGroupName = sGroupFolder;
			m_pSongs.push_back( pNewSong );
		}
	}
}

void SongManager::PreloadSongImages()
{
	if( PREFSMAN->m_BannerCache != PrefsManager::BNCACHE_FULL )
		return;

	/* Load textures before unloading old ones, so we don't reload textures
	 * that we don't need to. */
	RageTexturePreloader preload;

	const vector<Song*> &songs = SONGMAN->GetAllSongs();
	for( unsigned i = 0; i < songs.size(); ++i )
	{
		if( !songs[i]->HasBanner() )
			continue;

		const RageTextureID ID = Sprite::SongBannerTexture( songs[i]->GetBannerPath() );
		preload.Load( ID );
	}

	vector<Course*> courses;
	SONGMAN->GetAllCourses( courses, false );
	for( unsigned i = 0; i < courses.size(); ++i )
	{
		if( !courses[i]->HasBanner() )
			continue;

		const RageTextureID ID = Sprite::SongBannerTexture( courses[i]->m_sBannerPath );
		preload.Load( ID );
	}

	preload.Swap( m_TexturePreload );
}

void SongManager::FreeSongs()
{
	m_sSongGroupNames.clear();
	m_sSongGroupBannerPaths.clear();

	for( unsigned i=0; i<m_pSongs.size(); i++ )
		SAFE_DELETE( m_pSongs[i] );
	m_pSongs.clear();

	m_sSongGroupBannerPaths.clear();

	for( int i = 0; i < NUM_ProfileSlot; ++i )
		m_pBestSongs[i].clear();
	m_pShuffledSongs.clear();
}

RString SongManager::GetSongGroupBannerPath( RString sSongGroup )
{
	for( unsigned i = 0; i < m_sSongGroupNames.size(); ++i )
	{
		if( sSongGroup == m_sSongGroupNames[i] ) 
			return m_sSongGroupBannerPaths[i];
	}

	return RString();
}

void SongManager::GetSongGroupNames( vector<RString> &AddTo )
{
	AddTo.insert(AddTo.end(), m_sSongGroupNames.begin(), m_sSongGroupNames.end() );
}

bool SongManager::DoesSongGroupExist( RString sSongGroup )
{
	return find( m_sSongGroupNames.begin(), m_sSongGroupNames.end(), sSongGroup ) != m_sSongGroupNames.end();
}

RageColor SongManager::GetSongGroupColor( const RString &sSongGroup )
{
	for( unsigned i=0; i<m_sSongGroupNames.size(); i++ )
	{
		if( m_sSongGroupNames[i] == sSongGroup )
			return SONG_GROUP_COLOR.GetValue( i%NUM_SONG_GROUP_COLORS );	// TODO: Add course group colors?
	}
	
	ASSERT_M( 0, ssprintf("requested color for song group '%s' that doesn't exist",sSongGroup.c_str()) );
	return RageColor(1,1,1,1);
}

RageColor SongManager::GetSongColor( const Song* pSong )
{
	ASSERT( pSong );

	/* XXX:
	 * Previously, this matched all notes, which set a song to "extra" if it
	 * had any 10-foot steps at all, even edits or doubles.
	 *
	 * For now, only look at notes for the current note type.  This means that
	 * if a song has 10-foot steps on Doubles, it'll only show up red in Doubles.
	 * That's not too bad, I think.  This will also change it in the song scroll,
	 * which is a little odd but harmless. 
	 *
	 * XXX: Ack.  This means this function can only be called when we have a style
	 * set up, which is too restrictive.  How to handle this?
	 */
//	const StepsType st = GAMESTATE->GetCurrentStyle()->m_StepsType;
	const vector<Steps*>& vpSteps = pSong->GetAllSteps();
	for( unsigned i=0; i<vpSteps.size(); i++ )
	{
		const Steps* pSteps = vpSteps[i];
		switch( pSteps->GetDifficulty() )
		{
		case DIFFICULTY_CHALLENGE:
		case DIFFICULTY_EDIT:
			continue;
		}

//		if(pSteps->m_StepsType != st)
//			continue;

		if( pSteps->GetMeter() >= EXTRA_COLOR_METER )
			return (RageColor)EXTRA_COLOR;
	}

	return GetSongGroupColor( pSong->m_sGroupName );
}

RString SongManager::GetCourseGroupBannerPath( const RString &sCourseGroup )
{
	map<RString, CourseGroupInfo>::const_iterator iter = m_mapCourseGroupToInfo.find( sCourseGroup );
	if( iter == m_mapCourseGroupToInfo.end() )
	{
		ASSERT_M( 0, ssprintf("requested banner for course group '%s' that doesn't exist",sCourseGroup.c_str()) );
		return RString();
	}
	else 
	{
		return iter->second.m_sBannerPath;
	}
}

void SongManager::GetCourseGroupNames( vector<RString> &AddTo )
{
	FOREACHM_CONST( RString, CourseGroupInfo, m_mapCourseGroupToInfo, iter )
		AddTo.push_back( iter->first );
}

bool SongManager::DoesCourseGroupExist( const RString &sCourseGroup )
{
	return m_mapCourseGroupToInfo.find( sCourseGroup ) != m_mapCourseGroupToInfo.end();
}

RageColor SongManager::GetCourseGroupColor( const RString &sCourseGroup )
{
	int iIndex = 0;
	FOREACHM_CONST( RString, CourseGroupInfo, m_mapCourseGroupToInfo, iter )
	{
		if( iter->first == sCourseGroup )
			return SONG_GROUP_COLOR.GetValue( iIndex%NUM_SONG_GROUP_COLORS );
		iIndex++;
	}
	
	ASSERT_M( 0, ssprintf("requested color for course group '%s' that doesn't exist",sCourseGroup.c_str()) );
	return RageColor(1,1,1,1);
}

RageColor SongManager::GetCourseColor( const Course* pCourse )
{
	return GetCourseGroupColor( pCourse->m_sGroupName );
}

static void GetSongsFromVector( const vector<Song*> &Songs, vector<Song*> &AddTo, RString sGroupName, int iMaxStages )
{
	AddTo.clear();

	for( unsigned i=0; i<Songs.size(); i++ )
		if( sGroupName==GROUP_ALL || sGroupName==Songs[i]->m_sGroupName )
			if( SongManager::GetNumStagesForSong(Songs[i]) <= iMaxStages )
				AddTo.push_back( Songs[i] );
}

void SongManager::GetSongs( vector<Song*> &AddTo, RString sGroupName, int iMaxStages ) const
{
	GetSongsFromVector( m_pSongs, AddTo, sGroupName, iMaxStages );
}

void SongManager::GetBestSongs( vector<Song*> &AddTo, RString sGroupName, int iMaxStages, ProfileSlot slot ) const
{
	GetSongsFromVector( m_pBestSongs[slot], AddTo, sGroupName, iMaxStages );
}

int SongManager::GetNumSongs() const
{
	return m_pSongs.size();
}

int SongManager::GetNumSongGroups() const
{
	return m_sSongGroupNames.size();
}

int SongManager::GetNumCourses() const
{
	return m_pCourses.size();
}

int SongManager::GetNumCourseGroups() const
{
	return m_mapCourseGroupToInfo.size();
}

int SongManager::GetNumEditCourses( ProfileSlot slot ) const
{
	int iNum = 0;
	FOREACH_CONST( Course*, m_pCourses, p )
	{
		if( (*p)->GetLoadedFromProfileSlot() == slot )
			iNum++;
	}
	return iNum;
}

RString SongManager::ShortenGroupName( RString sLongGroupName )
{
	static TitleSubst tsub("Groups");

	TitleFields title;
	title.Title = sLongGroupName;
	tsub.Subst( title );
	return title.Title;
}

int SongManager::GetNumStagesForSong( const Song* pSong )
{
	ASSERT( pSong );
	if( pSong->m_fMusicLengthSeconds > PREFSMAN->m_fMarathonVerSongSeconds )
		return 3;
	if( pSong->m_fMusicLengthSeconds > PREFSMAN->m_fLongVerSongSeconds )
		return 2;
	else
		return 1;
}

static LocalizedString LOADING_COURSES ( "SongManager", "Loading courses..." );
void SongManager::InitCoursesFromDisk( LoadingWindow *ld )
{
	LOG->Trace( "Loading courses." );


	//
	// Load courses from in Courses dir
	//
	{
		vector<RString> saCourseFiles;
		GetDirListing( COURSES_DIR+"*.crs", saCourseFiles, false, true );
		for( unsigned i=0; i<saCourseFiles.size(); i++ )
		{
			Course* pCourse = new Course;
			CourseLoaderCRS::LoadFromCRSFile( saCourseFiles[i], *pCourse );
			m_pCourses.push_back( pCourse );

			if( ld )
			{
				ld->SetText( LOADING_COURSES.GetValue()+ssprintf("\n%s\n%s",
					"Courses",
					Basename(saCourseFiles[i]).c_str()));
				ld->Paint();
			}
		}
	}


	// Find all group directories in Courses dir
	{
		vector<RString> vsCourseGroupNames;
		GetDirListing( COURSES_DIR+"*", vsCourseGroupNames, true );
		StripCvs( vsCourseGroupNames );
		SortRStringArray( vsCourseGroupNames );
		
		FOREACH( RString, vsCourseGroupNames, sCourseGroup )	// for each dir in /Courses/
		{
			// Find all CRS files in this group directory
			vector<RString> vsCoursePaths;
			GetDirListing( COURSES_DIR + *sCourseGroup + "/*.crs", vsCoursePaths, false, true );
			SortRStringArray( vsCoursePaths );

			FOREACH_CONST( RString, vsCoursePaths, sCoursePath )
			{
				if( ld )
				{
					ld->SetText( LOADING_COURSES.GetValue()+ssprintf("\n%s\n%s",
						Basename(*sCourseGroup).c_str(),
						Basename(*sCoursePath).c_str()));
					ld->Paint();
				}

				Course* pCourse = new Course;
				CourseLoaderCRS::LoadFromCRSFile( *sCoursePath, *pCourse );
				m_pCourses.push_back( pCourse );
			}
		}
	}

	RefreshCourseGroupInfo();
}
	
void SongManager::InitAutogenCourses()
{
	//
	// Create group courses for Endless and Nonstop
	//
	vector<RString> saGroupNames;
	this->GetSongGroupNames( saGroupNames );
	Course* pCourse;
	for( unsigned g=0; g<saGroupNames.size(); g++ )	// foreach Group
	{
		RString sGroupName = saGroupNames[g];
		vector<Song*> apGroupSongs;
		GetSongs( apGroupSongs, sGroupName );

		// Generate random courses from each group.
		pCourse = new Course;
		CourseUtil::AutogenEndlessFromGroup( sGroupName, DIFFICULTY_MEDIUM, *pCourse );
		m_pCourses.push_back( pCourse );

		pCourse = new Course;
		CourseUtil::AutogenNonstopFromGroup( sGroupName, DIFFICULTY_MEDIUM, *pCourse );
		m_pCourses.push_back( pCourse );
	}
	
	vector<Song*> apCourseSongs = GetAllSongs();

	// Generate "All Songs" endless course.
	pCourse = new Course;
	CourseUtil::AutogenEndlessFromGroup( "", DIFFICULTY_MEDIUM, *pCourse );
	m_pCourses.push_back( pCourse );

	/* Generate Oni courses from artists.  Only create courses if we have at least
	 * four songs from an artist; create 3- and 4-song courses. */
	{
		/* We normally sort by translit artist.  However, display artist is more
		 * consistent.  For example, transliterated Japanese names are alternately
		 * spelled given- and family-name first, but display titles are more consistent. */
		vector<Song*> apSongs = this->GetAllSongs();
		SongUtil::SortSongPointerArrayByDisplayArtist( apSongs );

		RString sCurArtist = "";
		RString sCurArtistTranslit = "";
		int iCurArtistCount = 0;

		vector<Song *> aSongs;
		unsigned i = 0;
		do {
			RString sArtist = i >= apSongs.size()? RString(""): apSongs[i]->GetDisplayArtist();
			RString sTranslitArtist = i >= apSongs.size()? RString(""): apSongs[i]->GetTranslitArtist();
			if( i < apSongs.size() && !sCurArtist.CompareNoCase(sArtist) )
			{
				aSongs.push_back( apSongs[i] );
				++iCurArtistCount;
				continue;
			}

			/* Different artist, or we're at the end.  If we have enough entries for
			 * the last artist, add it.  Skip blanks and "Unknown artist". */
			if( iCurArtistCount >= 3 && sCurArtistTranslit != "" &&
				sCurArtistTranslit.CompareNoCase("Unknown artist") &&
				sCurArtist.CompareNoCase("Unknown artist") )
			{
				pCourse = new Course;
				CourseUtil::AutogenOniFromArtist( sCurArtist, sCurArtistTranslit, aSongs, DIFFICULTY_HARD, *pCourse );
				m_pCourses.push_back( pCourse );
			}

			aSongs.clear();
			
			if( i < apSongs.size() )
			{
				sCurArtist = sArtist;
				sCurArtistTranslit = sTranslitArtist;
				iCurArtistCount = 1;
				aSongs.push_back( apSongs[i] );
			}
		} while( i++ < apSongs.size() );
	}
}


void SongManager::FreeCourses()
{
	for( unsigned i=0; i<m_pCourses.size(); i++ )
		delete m_pCourses[i];
	m_pCourses.clear();

	for( int i = 0; i < NUM_ProfileSlot; ++i )
		FOREACH_CourseType( ct )
			m_pBestCourses[i][ct].clear();
	m_pShuffledCourses.clear();

	m_mapCourseGroupToInfo.clear();
}

void SongManager::AddCourse( Course *pCourse )
{
	m_pCourses.push_back( pCourse );
	UpdateBest();
	UpdateShuffled();
	m_mapCourseGroupToInfo[ pCourse->m_sGroupName ];	// insert
}

void SongManager::DeleteCourse( Course *pCourse )
{
	vector<Course*>::iterator iter = find( m_pCourses.begin(), m_pCourses.end(), pCourse );
	ASSERT( iter != m_pCourses.end() );
	m_pCourses.erase( iter );
	UpdateBest();
	UpdateShuffled();
	RefreshCourseGroupInfo();
}

/* Called periodically to wipe out cached NoteData.  This is called when we change
 * screens. */
void SongManager::Cleanup()
{
	for( unsigned i=0; i<m_pSongs.size(); i++ )
	{
		Song* pSong = m_pSongs[i];
		const vector<Steps*>& vpSteps = pSong->GetAllSteps();
		for( unsigned n=0; n<vpSteps.size(); n++ )
		{
			Steps* pSteps = vpSteps[n];
			pSteps->Compress();
		}
	}
}

/* Flush all Song*, Steps* and Course* caches.  This is called on reload, and when
 * any of those are removed or changed.  This doesn't touch GAMESTATE and StageStats
 * pointers, which are updated explicitly in Song::RevertFromDisk. */
void SongManager::Invalidate( Song *pStaleSong )
{
	//
	// Save list of all old Course and Trail pointers
	//
	map<Course*,CourseID> mapOldCourseToCourseID;
	typedef pair<TrailID,Course*> TrailIDAndCourse;
	map<Trail*,TrailIDAndCourse> mapOldTrailToTrailIDAndCourse;
	FOREACH_CONST( Course*, this->m_pCourses, pCourse )
	{
		CourseID id;
		id.FromCourse( *pCourse );
		mapOldCourseToCourseID[*pCourse] = id;
		vector<Trail *> Trails;
		(*pCourse)->GetAllCachedTrails( Trails );
		FOREACH_CONST( Trail*, Trails, pTrail )
		{
			TrailID id;
			id.FromTrail( *pTrail );
			mapOldTrailToTrailIDAndCourse[*pTrail] = TrailIDAndCourse(id, *pCourse);
		}
	}

	// It's a real pain to selectively invalidate only those Courses with 
	// dependencies on the stale Song.  So, instead, just reload all Courses.
	// It doesn't take very long.
	FreeCourses();
	InitCoursesFromDisk( NULL );
	InitAutogenCourses();

	// invalidate cache
	StepsID::ClearCache();

#define CONVERT_COURSE_POINTER( pCourse ) do { \
	CourseID id = mapOldCourseToCourseID[pCourse]; /* this will always succeed */ \
	pCourse = id.ToCourse(); \
} while(false)

	/* Ugly: We need the course pointer to restore a trail pointer, and both have
	 * been invalidated.  We need to go through our mapping, and update the course
	 * pointers, so we can use that to update trail pointers.  */
	{
		map<Trail*,TrailIDAndCourse>::iterator it;
		for( it = mapOldTrailToTrailIDAndCourse.begin(); it != mapOldTrailToTrailIDAndCourse.end(); ++it )
		{
			TrailIDAndCourse &tidc = it->second;
			CONVERT_COURSE_POINTER( tidc.second );
		}
	}

	{
		CourseID id = mapOldCourseToCourseID[GAMESTATE->m_pCurCourse]; /* this will always succeed */
		GAMESTATE->m_pCurCourse.Set( id.ToCourse() );
	}
	CONVERT_COURSE_POINTER( GAMESTATE->m_pPreferredCourse );

#define CONVERT_TRAIL_POINTER( pTrail ) do { \
	if( pTrail != NULL ) { \
		map<Trail*,TrailIDAndCourse>::iterator it; \
		it = mapOldTrailToTrailIDAndCourse.find(pTrail); \
		ASSERT_M( it != mapOldTrailToTrailIDAndCourse.end(), ssprintf("%p", pTrail.Get()) ); \
		const TrailIDAndCourse &tidc = it->second; \
		const TrailID &id = tidc.first; \
		const Course *pCourse = tidc.second; \
		pTrail.Set( id.ToTrail( pCourse, true ) ); \
	} \
} while(false)

	FOREACH_PlayerNumber( pn )
	{
		CONVERT_TRAIL_POINTER( GAMESTATE->m_pCurTrail[pn] );
	}
}

/* If bAllowNotesLoss is true, any global notes pointers which no longer exist
 * (or exist but couldn't be matched) will be set to NULL.  This is used when
 * reverting out of the editor.  If false, this is unexpected and will assert.
 * This is used when reverting out of gameplay, in which case we may have StageStats,
 * etc. which may cause hard-to-trace crashes down the line if we set them to NULL. */
void CONVERT_STEPS_POINTER( Steps *&pSteps, const map<Steps*,StepsID> &mapOldStepsToStepsID, const Song *pSong, bool bAllowNotesLoss )
{
	if( pSteps == NULL )
		return;

	map<Steps*,StepsID>::const_iterator it = mapOldStepsToStepsID.find(pSteps);
	if( it != mapOldStepsToStepsID.end() )
		pSteps = it->second.ToSteps(pSong, bAllowNotesLoss);
}
void CONVERT_STEPS_POINTER( BroadcastOnChangePtr<Steps> &pSteps, const map<Steps*,StepsID> &mapOldStepsToStepsID, const Song *pSong, bool bAllowNotesLoss )
{
	if( pSteps == NULL )
		return;

	map<Steps*,StepsID>::const_iterator it = mapOldStepsToStepsID.find(pSteps);
	if( it != mapOldStepsToStepsID.end() )
		pSteps.Set( it->second.ToSteps(pSong, bAllowNotesLoss) );
}
void SongManager::RevertFromDisk( Song *pSong, bool bAllowNotesLoss )
{
	/* Reverting from disk is brittle, and touches a lot of tricky and rarely-
	 * used code paths.  If it's ever used during a game, log it. */
	LOG->MapLog( "RevertFromDisk", "Reverted \"%s\" from disk", pSong->GetTranslitMainTitle().c_str() );

	// Ugly:  When we re-load the song, the Steps* will change.
	// Fix GAMESTATE->m_CurSteps, STATSMAN->m_CurStageStats, STATSMAN->m_vPlayedStageStats[] after reloading.
	/* XXX: This is very brittle.  However, we must know about all globals uses of Steps*,
	 * so we can check to make sure we didn't lose any steps which are referenced ... */


	//
	// Save list of all old Steps pointers for the song
	//
	map<Steps*,StepsID> mapOldStepsToStepsID;
	FOREACH_CONST( Steps*, pSong->GetAllSteps(), pSteps )
	{
		StepsID id;
		id.FromSteps( *pSteps );
		mapOldStepsToStepsID[*pSteps] = id;
	}


	//
	// Reload the song
	//
	const RString dir = pSong->GetSongDir();
	FILEMAN->FlushDirCache( dir );

	/* Erase existing data and reload. */
	pSong->Reset();
	const bool OldVal = PREFSMAN->m_bFastLoad;
	PREFSMAN->m_bFastLoad.Set( false );
	pSong->LoadFromSongDir( dir );	
	/* XXX: reload edits? */
	PREFSMAN->m_bFastLoad.Set( OldVal );


	/* Courses cache Steps pointers.  On the off chance that this isn't the last
	 * thing this screen does, clear that cache. */
	/* TODO: Don't make Song depend on SongManager.  This is breaking 
	 * encapsulation and placing confusing limitation on what can be done in 
	 * SONGMAN->Invalidate(). -Chris */
	this->Invalidate( pSong );
	StepsID::ClearCache();


	FOREACH_PlayerNumber( p )
	{
		CONVERT_STEPS_POINTER( GAMESTATE->m_pCurSteps[p], mapOldStepsToStepsID, pSong, bAllowNotesLoss );

		FOREACH( Steps*, STATSMAN->m_CurStageStats.m_player[p].vpPlayedSteps, pSteps )
			CONVERT_STEPS_POINTER( *pSteps, mapOldStepsToStepsID, pSong, bAllowNotesLoss );

		FOREACH( StageStats, STATSMAN->m_vPlayedStageStats, ss )
			FOREACH( Steps*, ss->m_player[p].vpPlayedSteps, pSteps )
				CONVERT_STEPS_POINTER( *pSteps, mapOldStepsToStepsID, pSong, bAllowNotesLoss );
	}

	CONVERT_STEPS_POINTER( GAMESTATE->m_pEditSourceSteps, mapOldStepsToStepsID, pSong, bAllowNotesLoss );
}

void SongManager::RegenerateNonFixedCourses()
{
	for( unsigned i=0; i < m_pCourses.size(); i++ )
		m_pCourses[i]->RegenerateNonFixedTrails();
}

void SongManager::SetPreferences()
{
	for( unsigned int i=0; i<m_pSongs.size(); i++ )
	{
		/* PREFSMAN->m_bAutogenSteps may have changed. */
		m_pSongs[i]->RemoveAutoGenNotes();
		m_pSongs[i]->AddAutoGenNotes();
	}
}

void SongManager::GetStepsLoadedFromProfile( vector<Steps*> &AddTo, ProfileSlot slot )
{
	const vector<Song*> &vSongs = SONGMAN->GetAllSongs();
	FOREACH_CONST( Song*, vSongs, song )
	{
		(*song)->GetStepsLoadedFromProfile( slot, AddTo );
	}
}

Song *SongManager::GetSongFromSteps( Steps *pSteps )
{
	ASSERT( pSteps );
	const vector<Song*> &vSongs = SONGMAN->GetAllSongs();
	FOREACH_CONST( Song*, vSongs, song )
	{
		vector<Steps*> vSteps;
		(*song)->GetSteps( vSteps );

		FOREACH_CONST( Steps*, vSteps, steps )
		{
			if( *steps == pSteps )
			{
				return *song;
			}
		}
	}
	ASSERT(0);
	return NULL;
}

void SongManager::DeleteSteps( Steps *pSteps )
{
	Song *pSong = GetSongFromSteps( pSteps );
	pSong->DeleteSteps( pSteps );
}

void SongManager::GetAllCourses( vector<Course*> &AddTo, bool bIncludeAutogen )
{
	for( unsigned i=0; i<m_pCourses.size(); i++ )
		if( bIncludeAutogen || !m_pCourses[i]->m_bIsAutogen )
			AddTo.push_back( m_pCourses[i] );
}

void SongManager::GetCourses( CourseType ct, vector<Course*> &AddTo, bool bIncludeAutogen )
{
	for( unsigned i=0; i<m_pCourses.size(); i++ )
		if( m_pCourses[i]->GetCourseType() == ct )
			if( bIncludeAutogen || !m_pCourses[i]->m_bIsAutogen )
				AddTo.push_back( m_pCourses[i] );
}

void SongManager::GetCoursesInGroup( vector<Course*> &AddTo, const RString &sCourseGroup, bool bIncludeAutogen )
{
	for( unsigned i=0; i<m_pCourses.size(); i++ )
		if( m_pCourses[i]->m_sGroupName == sCourseGroup )
			if( bIncludeAutogen || !m_pCourses[i]->m_bIsAutogen )
				AddTo.push_back( m_pCourses[i] );
}

bool SongManager::GetExtraStageInfoFromCourse( bool bExtra2, RString sPreferredGroup,
								   Song*& pSongOut, Steps*& pStepsOut, PlayerOptions *pPlayerOptionsOut, SongOptions *pSongOptionsOut )
{
	const RString sCourseSuffix = sPreferredGroup + "/" + (bExtra2 ? "extra2" : "extra1") + ".crs";
	RString sCoursePath = SONGS_DIR + sCourseSuffix;

	/* Couldn't find course in DWI path or alternative song folders */
	if( !DoesFileExist(sCoursePath) )
		return false;

	Course course;
	CourseLoaderCRS::LoadFromCRSFile( sCoursePath, course );
	if( course.GetEstimatedNumStages() <= 0 ) return false;

	Trail *pTrail = course.GetTrail( GAMESTATE->GetCurrentStyle()->m_StepsType );
	if( pTrail->m_vEntries.empty() )
		return false;

	if( pPlayerOptionsOut != NULL )
	{
		pPlayerOptionsOut->Init();
		pPlayerOptionsOut->FromString( pTrail->m_vEntries[0].Modifiers );
	}
	if( pSongOptionsOut != NULL )
	{
		pSongOptionsOut->Init();
		pSongOptionsOut->FromString( pTrail->m_vEntries[0].Modifiers );
	}

	pSongOut = pTrail->m_vEntries[0].pSong;
	pStepsOut = pTrail->m_vEntries[0].pSteps;
	return true;
}

/* Return true if n1 < n2. */
bool CompareNotesPointersForExtra(const Steps *n1, const Steps *n2)
{
	/* Equate CHALLENGE to HARD. */
	Difficulty d1 = min(n1->GetDifficulty(), DIFFICULTY_HARD);
	Difficulty d2 = min(n2->GetDifficulty(), DIFFICULTY_HARD);

	if(d1 < d2) return true;
	if(d1 > d2) return false;
	/* n1 difficulty == n2 difficulty */

	if(StepsUtil::CompareNotesPointersByMeter(n1,n2)) return true;
	if(StepsUtil::CompareNotesPointersByMeter(n2,n1)) return false;
	/* n1 meter == n2 meter */

	return StepsUtil::CompareNotesPointersByRadarValues(n1,n2);
}

void SongManager::GetExtraStageInfo( bool bExtra2, const Style *sd, 
								   Song*& pSongOut, Steps*& pStepsOut, PlayerOptions *pPlayerOptionsOut, SongOptions *pSongOptionsOut )
{
	RString sGroup = GAMESTATE->m_sPreferredSongGroup;
	if( sGroup == GROUP_ALL )
	{
		if( GAMESTATE->m_pCurSong == NULL )
		{
			/* This normally shouldn't happen, but it's helpful to permit it for testing. */
			LOG->Warn( "GetExtraStageInfo() called in GROUP_ALL, but GAMESTATE->m_pCurSong == NULL" );
			GAMESTATE->m_pCurSong.Set( SONGMAN->GetRandomSong() );
		}
		sGroup = GAMESTATE->m_pCurSong->m_sGroupName;
	}

	ASSERT_M( sGroup != "", ssprintf("%p '%s' '%s'",
		GAMESTATE->m_pCurSong.Get(),
		GAMESTATE->m_pCurSong? GAMESTATE->m_pCurSong->GetSongDir().c_str():"",
		GAMESTATE->m_pCurSong? GAMESTATE->m_pCurSong->m_sGroupName.c_str():"") );

	if( GetExtraStageInfoFromCourse(bExtra2, sGroup, pSongOut, pStepsOut, pPlayerOptionsOut, pSongOptionsOut) )
		return;
	
	// Choose a hard song for the extra stage
	Song*	pExtra1Song = NULL;		// the absolute hardest Song and Steps.  Use this for extra stage 1.
	Steps*	pExtra1Notes = NULL;
	Song*	pExtra2Song = NULL;		// a medium-hard Song and Steps.  Use this for extra stage 2.
	Steps*	pExtra2Notes = NULL;
	
	vector<Song*> apSongs;
	SONGMAN->GetSongs( apSongs, sGroup );
	for( unsigned s=0; s<apSongs.size(); s++ )	// foreach song
	{
		Song* pSong = apSongs[s];

		vector<Steps*> apSteps;
		pSong->GetSteps( apSteps, sd->m_StepsType );
		for( unsigned n=0; n<apSteps.size(); n++ )	// foreach Steps
		{
			Steps* pSteps = apSteps[n];

			if( pExtra1Notes == NULL || CompareNotesPointersForExtra(pExtra1Notes,pSteps) )	// pSteps is harder than pHardestNotes
			{
				pExtra1Song = pSong;
				pExtra1Notes = pSteps;
			}

			// for extra 2, we don't want to choose the hardest notes possible.  So, we'll disgard Steps with meter > 8
			if(	bExtra2  &&  pSteps->GetMeter() > 8 )	
				continue;	// skip
			if( pExtra2Notes == NULL  ||  CompareNotesPointersForExtra(pExtra2Notes,pSteps) )	// pSteps is harder than pHardestNotes
			{
				pExtra2Song = pSong;
				pExtra2Notes = pSteps;
			}
		}
	}

	if( pExtra2Song == NULL  &&  pExtra1Song != NULL )
	{
		pExtra2Song = pExtra1Song;
		pExtra2Notes = pExtra1Notes;
	}

	// If there are any notes at all that match this StepsType, everything should be filled out.
	// Also, it's guaranteed that there is at least one Steps that matches the StepsType because the player
	// had to play something before reaching the extra stage!
	ASSERT( pExtra2Song && pExtra1Song && pExtra2Notes && pExtra1Notes );

	pSongOut = (bExtra2 ? pExtra2Song : pExtra1Song);
	pStepsOut = (bExtra2 ? pExtra2Notes : pExtra1Notes);


	if( pPlayerOptionsOut != NULL )
	{
		pPlayerOptionsOut->Init();
		pPlayerOptionsOut->m_fScrolls[PlayerOptions::SCROLL_REVERSE] = 1;
		pPlayerOptionsOut->m_fScrollSpeed = 1.5f;
		pPlayerOptionsOut->m_fDark = 1;
	}

	if( pSongOptionsOut != NULL )
	{
		pSongOptionsOut->Init();
		pSongOptionsOut->m_DrainType = (bExtra2 ? SongOptions::DRAIN_SUDDEN_DEATH : SongOptions::DRAIN_NO_RECOVER);
	}
}

Song* SongManager::GetRandomSong()
{
	if( m_pShuffledSongs.empty() )
		return NULL;

	static int i = 0;

	for( int iThrowAway=0; iThrowAway<100; iThrowAway++ )
	{
		i++;
		wrap( i, m_pShuffledSongs.size() );
		Song *pSong = m_pShuffledSongs[ i ];
		if( pSong->IsTutorial() )
			continue;
		if( UNLOCKMAN->SongIsLocked(pSong) )
			continue;
		return pSong;
	}

	return NULL;
}

Course* SongManager::GetRandomCourse()
{
	if( m_pShuffledCourses.empty() )
		return NULL;

	static int i = 0;

	for( int iThrowAway=0; iThrowAway<100; iThrowAway++ )
	{
		i++;
		wrap( i, m_pShuffledCourses.size() );
		Course *pCourse = m_pShuffledCourses[ i ];
		if( pCourse->m_bIsAutogen && !PREFSMAN->m_bAutogenGroupCourses )
			continue;
		if( pCourse->GetCourseType() == COURSE_TYPE_ENDLESS )
			continue;
		if( UNLOCKMAN->CourseIsLocked(pCourse) )
			continue;
		return pCourse;
	}

	return NULL;
}

Song* SongManager::GetSongFromDir( RString sDir )
{
	if( sDir.Right(1) != "/" )
		sDir += "/";

	sDir.Replace( '\\', '/' );

	for( unsigned int i=0; i<m_pSongs.size(); i++ )
		if( sDir.CompareNoCase(m_pSongs[i]->GetSongDir()) == 0 )
			return m_pSongs[i];

	return NULL;
}

Course* SongManager::GetCourseFromPath( RString sPath )
{
	if( sPath == "" )
		return NULL;

	FOREACH_CONST( Course*, m_pCourses, c )
	{
		if( sPath.CompareNoCase((*c)->m_sPath) == 0 )
			return *c;
	}

	return NULL;
}

Course* SongManager::GetCourseFromName( RString sName )
{
	if( sName == "" )
		return NULL;

	for( unsigned int i=0; i<m_pCourses.size(); i++ )
		if( sName.CompareNoCase(m_pCourses[i]->GetDisplayFullTitle()) == 0 )
			return m_pCourses[i];

	return NULL;
}


/*
 * GetSongDir() contains a path to the song, possibly a full path, eg:
 * Songs\Group\SongName                   or 
 * My Other Song Folder\Group\SongName    or
 * c:\Corny J-pop\Group\SongName
 *
 * Most course group names are "Group\SongName", so we want to
 * match against the last two elements. Let's also support
 * "SongName" alone, since the group is only important when it's
 * potentially ambiguous.
 *
 * Let's *not* support "Songs\Group\SongName" in course files.
 * That's probably a common error, but that would result in
 * course files floating around that only work for people who put
 * songs in "Songs"; we don't want that.
 */

Song *SongManager::FindSong( RString sPath )
{
	sPath.Replace( '\\', '/' );
	vector<RString> bits;
	split( sPath, "/", bits );

	if( bits.size() == 1 )
		return FindSong( "", bits[0] );
	else if( bits.size() == 2 )
		return FindSong( bits[0], bits[1] );

	return NULL;	
}

Song *SongManager::FindSong( RString sGroup, RString sSong )
{
	// foreach song
	for( unsigned i = 0; i < m_pSongs.size(); i++ )
	{
		if( m_pSongs[i]->Matches(sGroup, sSong) )
			return m_pSongs[i];
	}

	return NULL;	
}

Course *SongManager::FindCourse( RString sName )
{
	for( unsigned i = 0; i < m_pCourses.size(); i++ )
	{
		if( !sName.CompareNoCase(m_pCourses[i]->GetDisplayFullTitle()) )
			return m_pCourses[i];
	}

	return NULL;
}

void SongManager::UpdateBest()
{
	// update players best
	vector<Song*> apBestSongs = m_pSongs;
	for ( unsigned j=0; j < apBestSongs.size() ; ++j )
	{
		bool bFiltered = false;
		/* Filter out hidden songs. */
		if( apBestSongs[j]->GetDisplayed() != Song::SHOW_ALWAYS )
			bFiltered = true;
		/* Filter out locked songs. */
		if( UNLOCKMAN->SongIsLocked(apBestSongs[j]) )
			bFiltered = true;
		if( !bFiltered )
			continue;

		/* Remove it. */
		swap( apBestSongs[j], apBestSongs.back() );
		apBestSongs.erase( apBestSongs.end()-1 );
		--j;
	}

	SongUtil::SortSongPointerArrayByTitle( apBestSongs );
	
	vector<Course*> apBestCourses[NUM_CourseType];
	FOREACH_CourseType( ct )
	{
		GetCourses( ct, apBestCourses[ct], PREFSMAN->m_bAutogenGroupCourses );
		CourseUtil::SortCoursePointerArrayByTitle( apBestCourses[ct] );
	}

	FOREACH_ProfileSlot( i )
	{
		m_pBestSongs[i] = apBestSongs;

		SongUtil::SortSongPointerArrayByNumPlays( m_pBestSongs[i], i, true );

		FOREACH_CourseType( ct )
		{
			vector<Course*> &vpCourses = m_pBestCourses[i][ct];
			vpCourses = apBestCourses[ct];
			CourseUtil::SortCoursePointerArrayByNumPlays( vpCourses, i, true );
		}
	}
}

void SongManager::UpdateShuffled()
{
	// update shuffled
	m_pShuffledSongs = m_pSongs;
	random_shuffle( m_pShuffledSongs.begin(), m_pShuffledSongs.end() );

	m_pShuffledCourses = m_pCourses;
	random_shuffle( m_pShuffledCourses.begin(), m_pShuffledCourses.end() );
}

void SongManager::SortSongs()
{
	SongUtil::SortSongPointerArrayByTitle( m_pSongs );
}

void SongManager::UpdateRankingCourses()
{
	/* Updating the ranking courses data is fairly expensive
	 * since it involves comparing strings.  Do so sparingly. */
	vector<RString> RankingCourses;
	split( THEME->GetMetric("ScreenRanking","CoursesToShow"),",", RankingCourses);

	for( unsigned i=0; i < m_pCourses.size(); i++ )
	{
		if( m_pCourses[i]->GetEstimatedNumStages() > 7 )
			m_pCourses[i]->m_SortOrder_Ranking = 3;
		else
			m_pCourses[i]->m_SortOrder_Ranking = 2;
		
		for( unsigned j = 0; j < RankingCourses.size(); j++ )
			if( !RankingCourses[j].CompareNoCase(m_pCourses[i]->m_sPath) )
				m_pCourses[i]->m_SortOrder_Ranking = 1;
	}
}

void SongManager::RefreshCourseGroupInfo()
{
	m_mapCourseGroupToInfo.clear();

	FOREACH_CONST( Course*, m_pCourses, c )
	{
		m_mapCourseGroupToInfo[(*c)->m_sGroupName];	// insert
	}

	// TODO: Search for course group banners
	FOREACHM( RString, CourseGroupInfo, m_mapCourseGroupToInfo, iter )
	{
	}
}

void SongManager::LoadAllFromProfileDir( const RString &sProfileDir, ProfileSlot slot )
{
	{
		//
		// Load all edit steps
		//
		RString sDir = sProfileDir + EDIT_STEPS_SUBDIR;

		vector<RString> vsFiles;
		GetDirListing( sDir+"*.edit", vsFiles, false, true );

		int iNumEditsLoaded = GetNumEditsLoadedFromProfile( slot );
		int size = min( (int) vsFiles.size(), MAX_EDIT_STEPS_PER_PROFILE - iNumEditsLoaded );

		for( int i=0; i<size; i++ )
		{
			RString fn = vsFiles[i];

			SMLoader::LoadEdit( fn, slot );
		}
	}

	{
		//
		// Load all edit courses
		//
		RString sDir = sProfileDir + EDIT_COURSES_SUBDIR;

		vector<RString> vsFiles;
		GetDirListing( sDir+"*.crs", vsFiles, false, true );

		int iNumEditsLoaded = GetNumEditsLoadedFromProfile( slot );
		int size = min( (int) vsFiles.size(), MAX_EDIT_COURSES_PER_PROFILE - iNumEditsLoaded );

		for( int i=0; i<size; i++ )
		{
			RString fn = vsFiles[i];

			CourseLoaderCRS::LoadEdit( fn, slot );
		}
	}
}

int SongManager::GetNumEditsLoadedFromProfile( ProfileSlot slot ) const
{
	int iCount = 0;
	for( unsigned s=0; s<m_pSongs.size(); s++ )
	{
		const Song *pSong = m_pSongs[s];
		vector<Steps*> apSteps;
		pSong->GetSteps( apSteps );

		for( unsigned i = 0; i < apSteps.size(); ++i )
		{
			const Steps *pSteps = apSteps[i];
			if( pSteps->WasLoadedFromProfile() && pSteps->GetLoadedFromProfileSlot() == slot )
				++iCount;
		}
	}
	return iCount;
}

void SongManager::FreeAllLoadedFromProfile( ProfileSlot slot )
{
	FOREACH( Song*, m_pSongs, s )
		(*s)->FreeAllLoadedFromProfile( slot );

	vector<Course*> apToDelete;
	FOREACH( Course*, m_pCourses, c )
	{
		Course *pCourse = *c;
		if( pCourse->GetLoadedFromProfileSlot() == ProfileSlot_INVALID )
			continue;
		
		if( slot == ProfileSlot_INVALID || pCourse->GetLoadedFromProfileSlot() == slot )
			apToDelete.push_back( *c );
	}

	// XXX: this will update best, etc. every time; too slow
	for( unsigned i = 0; i < apToDelete.size(); ++i )
		this->DeleteCourse( apToDelete[i] );

	// After freeing some Steps pointers, the cache will be invalid.
	StepsID::ClearCache();
}

int SongManager::GetNumStepsLoadedFromProfile()
{
	int iCount = 0;
	FOREACH( Song*, m_pSongs, s )
	{
		vector<Steps*> vpAllSteps = (*s)->GetAllSteps();

		FOREACH( Steps*, vpAllSteps, ss )
		{
			if( (*ss)->GetLoadedFromProfileSlot() != ProfileSlot_INVALID )
				iCount++;
		}
	}	

	return iCount;
}


// lua start
#include "LuaBinding.h"

class LunaSongManager: public Luna<SongManager>
{
public:
	LunaSongManager() { LUA->Register( Register ); }

	static int GetAllSongs( T* p, lua_State *L )
	{
		const vector<Song*> &v = p->GetAllSongs();
		LuaHelpers::CreateTableFromArray<Song*>( v, L );
		return 1;
	}
	static int GetAllCourses( T* p, lua_State *L )
	{
		vector<Course*> v;
		p->GetAllCourses( v, BArg(1) );
		LuaHelpers::CreateTableFromArray<Course*>( v, L );
		return 1;
	}
	static int FindSong( T* p, lua_State *L )		{ Song *pS = p->FindSong(SArg(1)); if(pS) pS->PushSelf(L); else lua_pushnil(L); return 1; }
	static int FindCourse( T* p, lua_State *L )		{ Course *pC = p->FindCourse(SArg(1)); if(pC) pC->PushSelf(L); else lua_pushnil(L); return 1; }
	static int GetRandomSong( T* p, lua_State *L )	{ Song *pS = p->GetRandomSong(); if(pS) pS->PushSelf(L); else lua_pushnil(L); return 1; }
	static int GetRandomCourse( T* p, lua_State *L ){ Course *pC = p->GetRandomCourse(); if(pC) pC->PushSelf(L); else lua_pushnil(L); return 1; }
	static int GetNumSongs( T* p, lua_State *L )    { lua_pushnumber( L, p->GetNumSongs() ); return 1; }
	static int GetNumSongGroups( T* p, lua_State *L ) { lua_pushnumber( L, p->GetNumSongGroups() ); return 1; }
	static int GetNumCourses( T* p, lua_State *L )  { lua_pushnumber( L, p->GetNumCourses() ); return 1; }
	static int GetNumCourseGroups( T* p, lua_State *L ) { lua_pushnumber( L, p->GetNumCourseGroups() ); return 1; }
	static int GetSongFromSteps( T* p, lua_State *L )
	{
		Song *pSong = NULL;
		if( lua_isnil(L,1) ) { pSong = p->GetSongFromSteps( NULL ); }
		else { Steps *pSteps = Luna<Steps>::check(L,1); pSong = p->GetSongFromSteps( pSteps ); }
		if(pSong) pSong->PushSelf(L);
		else lua_pushnil(L);
		return 1;
	}

	static void Register(lua_State *L)
	{
		ADD_METHOD( GetAllSongs );
		ADD_METHOD( GetAllCourses );
		ADD_METHOD( FindSong );
		ADD_METHOD( FindCourse );
		ADD_METHOD( GetRandomSong );
		ADD_METHOD( GetRandomCourse );
		ADD_METHOD( GetNumSongs );
		ADD_METHOD( GetNumSongGroups );
		ADD_METHOD( GetNumCourses );
		ADD_METHOD( GetNumCourseGroups );
		ADD_METHOD( GetSongFromSteps );

		Luna<T>::Register( L );

		// Add global singleton if constructed already.  If it's not constructed yet,
		// then we'll register it later when we reinit Lua just before 
		// initializing the display.
		if( SONGMAN )
		{
			lua_pushstring(L, "SONGMAN");
			SONGMAN->PushSelf( L );
			lua_settable(L, LUA_GLOBALSINDEX);
		}
	}
};

LUA_REGISTER_CLASS( SongManager )
// lua end

/*
 * (c) 2001-2004 Chris Danford, Glenn Maynard
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
