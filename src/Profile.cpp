#include "global.h"
#include "Profile.h"
#include "RageUtil.h"
#include "PrefsManager.h"
#include "XmlFile.h"
#include "IniFile.h"
#include "GameManager.h"
#include "RageLog.h"
#include "RageFile.h"
#include "song.h"
#include "SongManager.h"
#include "Steps.h"
#include "Course.h"
#include <ctime>
#include "ThemeManager.h"
#include "CryptManager.h"
#include "ProfileHtml.h"
#include "ProfileManager.h"
#include "RageFileManager.h"
#include "ScoreKeeperMAX2.h"
#include "crypto/CryptRand.h"
#include "UnlockSystem.h"
#include "XmlFile.h"
#include "Foreach.h"

//
// Old file versions for backward compatibility
//
#define SM_300_STATISTICS_INI	"statistics.ini"

#define GUID_SIZE_BYTES 8

#define MAX_RECENT_SCORES_TO_SAVE 100

#define MAX_EDITABLE_INI_SIZE_BYTES			2*1024		// 2KB
#define MAX_PLAYER_STATS_XML_SIZE_BYTES	\
	100 /* Songs */						\
	* 3 /* Steps per Song */			\
	* 10 /* HighScores per Steps */		\
	* 1024 /* size in bytes of a HighScores XNode */

#if defined(WIN32)
#pragma warning (disable : 4706) // assignment within conditional expression
#endif

#define FOREACH_Node( Node, Var ) \
	XNodes::const_iterator Var##Iter; \
	const XNode *Var = NULL; \
	for( Var##Iter = Node->childs.begin(); \
		(Var##Iter != Node->childs.end() && (Var = *Var##Iter) ),  Var##Iter != Node->childs.end(); \
		++Var##Iter )

void Profile::InitEditableData()
{
	m_sDisplayName = "";	
	m_sLastUsedHighScoreName = "";
	m_iWeightPounds = 0;
}

void Profile::InitGeneralData()
{
	// Init m_iGuid.
	// Does the RNG need to be inited and seeded every time?
	random_init();
	random_add_noise( "ai8049ujr3odusj" );
	
	{
		m_sGuid = "";
		for( unsigned i=0; i<GUID_SIZE_BYTES; i++ )
			m_sGuid += ssprintf( "%02x", random_byte() );
	}


	m_bUsingProfileDefaultModifiers = false;
	m_sDefaultModifiers = "";
	m_SortOrder = SORT_INVALID;
	m_LastDifficulty = DIFFICULTY_INVALID;
	m_LastCourseDifficulty = DIFFICULTY_INVALID;
	m_pLastSong = NULL;
	m_pLastCourse = NULL;
	m_iTotalPlays = 0;
	m_iTotalPlaySeconds = 0;
	m_iTotalGameplaySeconds = 0;
	m_iCurrentCombo = 0;
	m_fTotalCaloriesBurned = 0;
	m_iTotalDancePoints = 0;
	m_iNumExtraStagesPassed = 0;
	m_iNumExtraStagesFailed = 0;
	m_iNumToasties = 0;
	m_UnlockedSongs.clear();
	m_sLastPlayedMachineGuid = "";
	m_iTotalTapsAndHolds = 0;
	m_iTotalJumps = 0;
	m_iTotalHolds = 0;
	m_iTotalMines = 0;
	m_iTotalHands = 0;

	int i;
	for( i=0; i<NUM_PLAY_MODES; i++ )
		m_iNumSongsPlayedByPlayMode[i] = 0;
	for( i=0; i<NUM_STYLES; i++ )
		m_iNumSongsPlayedByStyle[i] = 0;
	for( i=0; i<NUM_DIFFICULTIES; i++ )
		m_iNumSongsPlayedByDifficulty[i] = 0;
	for( i=0; i<MAX_METER+1; i++ )
		m_iNumSongsPlayedByMeter[i] = 0;
	ZERO( m_iNumSongsPassedByPlayMode );
	ZERO( m_iNumSongsPassedByGrade );
}

void Profile::InitSongScores()
{
	m_SongHighScores.clear();
}

void Profile::InitCourseScores()
{
	m_CourseHighScores.clear();
}

void Profile::InitCategoryScores()
{
	for( int st=0; st<NUM_STEPS_TYPES; st++ )
		for( int rc=0; rc<NUM_RANKING_CATEGORIES; rc++ )
			m_CategoryHighScores[st][rc].Init();
}

void Profile::InitScreenshotData()
{
	m_vScreenshots.clear();
}

void Profile::InitCalorieData()
{
	m_mapDayToCaloriesBurned.clear();
}

void Profile::InitAwards()
{
	FOREACH_StepsType( st )
	{
		FOREACH_Difficulty( dc )
			FOREACH_PerDifficultyAward( pda )
				m_PerDifficultyAwards[st][dc][pda].Unset();
	}
	FOREACH_PeakComboAward( pca )
	{
		m_PeakComboAwards[pca].Unset();
	}
}

void Profile::InitRecentSongScores()
{
	m_vRecentStepsScores.clear();
}

void Profile::InitRecentCourseScores()
{
	m_vRecentCourseScores.clear();
}

CString Profile::GetDisplayName() const
{
	if( !m_sDisplayName.empty() )
		return m_sDisplayName;
	else if( !m_sLastUsedHighScoreName.empty() )
		return m_sLastUsedHighScoreName;
	else
		return "NoName";
}

CString Profile::GetDisplayTotalCaloriesBurned() const
{
	if( m_iWeightPounds == 0 )	// weight not entered
		return "N/A";
	else 
		return Commify((int)m_fTotalCaloriesBurned) + " Cal";
}

int Profile::GetTotalNumSongsPlayed() const
{
	int iTotal = 0;
	for( int i=0; i<NUM_PLAY_MODES; i++ )
		iTotal += m_iNumSongsPlayedByPlayMode[i];
	return iTotal;
}

int Profile::GetTotalNumSongsPassed() const
{
	int iTotal = 0;
	for( int i=0; i<NUM_PLAY_MODES; i++ )
		iTotal += m_iNumSongsPassedByPlayMode[i];
	return iTotal;
}

int Profile::GetPossibleSongDancePointsForStepsType( StepsType st ) const
{
	int iTotal = 0;

	// add steps high scores
	{
		const vector<Song*> vSongs = SONGMAN->GetAllSongs();
		for( unsigned i=0; i<vSongs.size(); i++ )
		{
			Song* pSong = vSongs[i];
			
			if( pSong->m_SelectionDisplay == Song::SHOW_NEVER )
				continue;	// skip

			vector<Steps*> vSteps = pSong->GetAllSteps();
			for( unsigned j=0; j<vSteps.size(); j++ )
			{
				Steps* pSteps = vSteps[j];
				
				if( pSteps->m_StepsType != st )
					continue;	// skip

				const RadarValues& fRadars = pSteps->GetRadarValues();
				iTotal += ScoreKeeperMAX2::GetPossibleDancePoints( fRadars );
			}
		}
	}

	return iTotal;
}

int Profile::GetActualSongDancePointsForStepsType( StepsType st ) const
{
	int iTotal = 0;
	
	// add steps high scores
	{
		for( std::map<SongID,HighScoresForASong>::const_iterator i = m_SongHighScores.begin();
			i != m_SongHighScores.end();
			++i )
		{
			const SongID &id = i->first;
			Song* pSong = id.ToSong();
			
			// If the Song isn't loaded on the current machine, then we can't 
			// get radar values to compute dance points.
			if( pSong == NULL )
				continue;

			if( pSong->m_SelectionDisplay == Song::SHOW_NEVER )
				continue;	// skip

			const HighScoresForASong &hsfas = i->second;

			for( std::map<StepsID,HighScoresForASteps>::const_iterator j = hsfas.m_StepsHighScores.begin();
				j != hsfas.m_StepsHighScores.end();
				++j )
			{
				const StepsID &id = j->first;
				Steps* pSteps = id.ToSteps( pSong, true );
				
				// If the Steps isn't loaded on the current machine, then we can't 
				// get radar values to compute dance points.
				if( pSteps == NULL )
					continue;

				if( pSteps->m_StepsType != st )
					continue;

				const HighScoresForASteps& h = j->second;
				const HighScoreList& hs = h.hs;

				const RadarValues& fRadars = pSteps->GetRadarValues();
				int iPossibleDP = ScoreKeeperMAX2::GetPossibleDancePoints( fRadars );
				iTotal += (int)truncf( hs.GetTopScore().fPercentDP * iPossibleDP );
			}
		}
	}

	return iTotal;
}

int Profile::GetPossibleCourseDancePointsForStepsType( StepsType st ) const
{
	int iTotal = 0;

	// add course high scores
	vector<Course*> vCourses;
	SONGMAN->GetAllCourses( vCourses, false );
	for( unsigned i=0; i<vCourses.size(); i++ )
	{
		const Course* pCourse = vCourses[i];
		
		// Don't count any course that has any entries that change over time.
		if( !pCourse->AllSongsAreFixed() )
			continue;

		FOREACH_ShownCourseDifficulty( cd )
		{
			Trail* pTrail = pCourse->GetTrail(st,cd);
			if( pTrail == NULL )
				continue;

			const RadarValues& fRadars = pTrail->GetRadarValues();
			iTotal += ScoreKeeperMAX2::GetPossibleDancePoints( fRadars );
		}
	}

	return iTotal;
}

int Profile::GetActualCourseDancePointsForStepsType( StepsType st ) const
{
	int iTotal = 0;
	
	// add course high scores
	{
		for( std::map<CourseID,HighScoresForACourse>::const_iterator i = m_CourseHighScores.begin();
			i != m_CourseHighScores.end();
			i++ )
		{
			CourseID id = i->first;
			const Course* pCourse = id.ToCourse();
			
			// If the Course isn't loaded on the current machine, then we can't 
			// get radar values to compute dance points.
			if( pCourse == NULL )
				continue;
			
			// Don't count any course that has any entries that change over time.
			if( !pCourse->AllSongsAreFixed() )
				continue;

			const HighScoresForACourse &hsfac = i->second;

			for( std::map<TrailID,HighScoresForATrail>::const_iterator j = hsfac.m_TrailHighScores.begin();
				j != hsfac.m_TrailHighScores.end();
				++j )
			{
				const TrailID &id = j->first;
				Trail* pTrail = id.ToTrail( pCourse, true );
				
				// If the Steps isn't loaded on the current machine, then we can't 
				// get radar values to compute dance points.
				if( pTrail == NULL )
					continue;

				if( pTrail->m_StepsType != st )
					continue;

				const HighScoresForATrail& h = j->second;
				const HighScoreList& hs = h.hs;

				const RadarValues& fRadars = pTrail->GetRadarValues();
				int iPossibleDP = ScoreKeeperMAX2::GetPossibleDancePoints( fRadars );
				iTotal += (int)truncf( hs.GetTopScore().fPercentDP * iPossibleDP );
			}
		}
	}

	return iTotal;
}

float Profile::GetPercentCompleteForStepsType( StepsType st ) const
{
	int iPossible = 
		GetPossibleSongDancePointsForStepsType( st ) +
		GetPossibleCourseDancePointsForStepsType( st );
	int iActual = 
		GetActualSongDancePointsForStepsType( st ) +
		GetActualCourseDancePointsForStepsType( st );

	return float(iActual) / iPossible;
}

CString Profile::GetProfileDisplayNameFromDir( CString sDir )
{
	Profile profile;
	profile.LoadEditableDataFromDir( sDir );
	return profile.GetDisplayName();
}

int Profile::GetSongNumTimesPlayed( const Song* pSong ) const
{
	SongID songID;
	songID.FromSong( pSong );
	return GetSongNumTimesPlayed( songID );
}

int Profile::GetSongNumTimesPlayed( const SongID& songID ) const
{
	const HighScoresForASong *hsSong = GetHighScoresForASong( songID );
	if( hsSong == NULL )
		return 0;

	int iTotalNumTimesPlayed = 0;
	for( std::map<StepsID,HighScoresForASteps>::const_iterator j = hsSong->m_StepsHighScores.begin();
		j != hsSong->m_StepsHighScores.end();
		j++ )
	{
		const HighScoresForASteps &hsSteps = j->second;

		iTotalNumTimesPlayed += hsSteps.hs.iNumTimesPlayed;
	}
	return iTotalNumTimesPlayed;
}

//
// Steps high scores
//
void Profile::AddStepsHighScore( const Song* pSong, const Steps* pSteps, HighScore hs, int &iIndexOut )
{
	GetStepsHighScoreList(pSong,pSteps).AddHighScore( hs, iIndexOut, IsMachine() );
}

const HighScoreList& Profile::GetStepsHighScoreList( const Song* pSong, const Steps* pSteps ) const
{
	return ((Profile*)this)->GetStepsHighScoreList(pSong,pSteps);
}

HighScoreList& Profile::GetStepsHighScoreList( const Song* pSong, const Steps* pSteps )
{
	SongID songID;
	songID.FromSong( pSong );
	
	StepsID stepsID;
	stepsID.FromSteps( pSteps );
	
	HighScoresForASong &hsSong = m_SongHighScores[songID];	// operator[] inserts into map
	HighScoresForASteps &hsSteps = hsSong.m_StepsHighScores[stepsID];	// operator[] inserts into map

	return hsSteps.hs;
}

int Profile::GetStepsNumTimesPlayed( const Song* pSong, const Steps* pSteps ) const
{
	return GetStepsHighScoreList(pSong,pSteps).iNumTimesPlayed;
}

void Profile::IncrementStepsPlayCount( const Song* pSong, const Steps* pSteps )
{
	GetStepsHighScoreList(pSong,pSteps).iNumTimesPlayed++;
}

void Profile::GetGrades( const Song* pSong, StepsType st, int iCounts[NUM_GRADES] ) const
{
	SongID songID;
	songID.FromSong( pSong );

	
	memset( iCounts, 0, sizeof(int)*NUM_GRADES );
	const HighScoresForASong *hsSong = GetHighScoresForASong( songID );
	if( hsSong == NULL )
		return;

	FOREACH_Grade(g)
	{
		std::map<StepsID,HighScoresForASteps>::const_iterator it;
		for( it = hsSong->m_StepsHighScores.begin(); it != hsSong->m_StepsHighScores.end(); ++it )
		{
			const StepsID &id = it->first;
			if( !id.MatchesStepsType(st) )
				continue;

			const HighScoresForASteps &hsSteps = it->second;
			if( hsSteps.hs.GetTopScore().grade == g )
				iCounts[g]++;
		}
	}
}

//
// Course high scores
//
void Profile::AddCourseHighScore( const Course* pCourse, const Trail* pTrail, HighScore hs, int &iIndexOut )
{
	GetCourseHighScoreList(pCourse,pTrail).AddHighScore( hs, iIndexOut, IsMachine() );
}

const HighScoreList& Profile::GetCourseHighScoreList( const Course* pCourse, const Trail* pTrail ) const
{
	return ((Profile *)this)->GetCourseHighScoreList( pCourse, pTrail );
}

HighScoreList& Profile::GetCourseHighScoreList( const Course* pCourse, const Trail* pTrail )
{
	CourseID courseID;
	courseID.FromCourse( pCourse );

	TrailID trailID;
	trailID.FromTrail( pTrail );

	HighScoresForACourse &hsCourse = m_CourseHighScores[courseID];	// operator[] inserts into map
	HighScoresForATrail &hsTrail = hsCourse.m_TrailHighScores[trailID];	// operator[] inserts into map

	return hsTrail.hs;
}

int Profile::GetCourseNumTimesPlayed( const Course* pCourse ) const
{
	CourseID courseID;
	courseID.FromCourse( pCourse );

	return GetCourseNumTimesPlayed( courseID );
}

int Profile::GetCourseNumTimesPlayed( const CourseID &courseID ) const
{
	const HighScoresForACourse *hsCourse = GetHighScoresForACourse( courseID );
	if( hsCourse == NULL )
		return 0;

	int iTotalNumTimesPlayed = 0;
	for( std::map<TrailID,HighScoresForATrail>::const_iterator j = hsCourse->m_TrailHighScores.begin();
		j != hsCourse->m_TrailHighScores.end();
		j++ )
	{
		const HighScoresForATrail &hsTrail = j->second;

		iTotalNumTimesPlayed += hsTrail.hs.iNumTimesPlayed;
	}
	return iTotalNumTimesPlayed;
}

void Profile::IncrementCoursePlayCount( const Course* pCourse, const Trail* pTrail )
{
	GetCourseHighScoreList(pCourse,pTrail).iNumTimesPlayed++;
}

//
// Category high scores
//
void Profile::AddCategoryHighScore( StepsType st, RankingCategory rc, HighScore hs, int &iIndexOut )
{
	m_CategoryHighScores[st][rc].AddHighScore( hs, iIndexOut, IsMachine() );
}

const HighScoreList& Profile::GetCategoryHighScoreList( StepsType st, RankingCategory rc ) const
{
	return ((Profile *)this)->m_CategoryHighScores[st][rc];
}

HighScoreList& Profile::GetCategoryHighScoreList( StepsType st, RankingCategory rc )
{
	return m_CategoryHighScores[st][rc];
}

int Profile::GetCategoryNumTimesPlayed( StepsType st ) const
{
	int iNumTimesPlayed = 0;
	FOREACH_RankingCategory( rc )
		iNumTimesPlayed += m_CategoryHighScores[st][rc].iNumTimesPlayed;
	return iNumTimesPlayed;
}

void Profile::IncrementCategoryPlayCount( StepsType st, RankingCategory rc )
{
	m_CategoryHighScores[st][rc].iNumTimesPlayed++;
}


//
// Loading and saving
//

#define WARN	LOG->Warn("Error parsing file at %s:%d",__FILE__,__LINE__);
#define WARN_AND_RETURN { WARN; return; }
#define WARN_AND_CONTINUE { WARN; continue; }
#define WARN_AND_BREAK { WARN; break; }
#define LOAD_NODE(X)	{ \
	XNode* X = xml.GetChild(#X); \
	if( X==NULL ) LOG->Warn("Failed to read section " #X); \
	else Load##X##FromNode(X); }
int g_iOnceCtr;
#define FOR_ONCE	for(g_iOnceCtr=0;g_iOnceCtr<1;g_iOnceCtr++)


bool Profile::LoadAllFromDir( CString sDir, bool bRequireSignature )
{
	CHECKPOINT;

	LOG->Trace( "Profile::LoadAllFromDir( %s )", sDir.c_str() );

	InitAll();

	LoadEditableDataFromDir( sDir );
	
	// Read stats.xml
	FOR_ONCE
	{
		CString fn = sDir + STATS_XML;
		if( !IsAFile(fn) )
			break;

		//
		// Don't unreasonably large stats.xml files.
		//
		if( !IsMachine() )	// only check stats coming from the player
		{
			int iBytes = FILEMAN->GetFileSizeInBytes( fn );
			if( iBytes > MAX_PLAYER_STATS_XML_SIZE_BYTES )
			{
				LOG->Warn( "The file '%s' is unreasonably large.  It won't be loaded.", fn.c_str() );
				break;
			}
		}

		if( bRequireSignature )
		{

			CString sStatsXmlSigFile = fn+SIGNATURE_APPEND;
			CString sDontShareFile = sDir + DONT_SHARE_SIG;

			LOG->Trace( "Verifying don't share signature" );
			// verify the stats.xml signature with the "don't share" file
			if( !CryptManager::VerifyFileWithFile(sStatsXmlSigFile, sDontShareFile) )
			{
				LOG->Warn( "The don't share check for '%s' failed.  Data will be ignored.", sStatsXmlSigFile.c_str() );
				break;
			}
			LOG->Trace( "Done." );

			// verify stats.xml
			LOG->Trace( "Verifying stats.xml signature" );
			if( !CryptManager::VerifyFileWithFile(fn, sStatsXmlSigFile) )
			{
				LOG->Warn( "The signature check for '%s' failed.  Data will be ignored.", fn.c_str() );
				break;
			}
			LOG->Trace( "Done." );
		}

		LOG->Trace( "Loading stats.xml" );
		XNode xml;
		if( !xml.LoadFromFile( fn ) )
		{
			LOG->Warn( "Couldn't open file '%s' for reading.", fn.c_str() );
			break;
		}
		LOG->Trace( "Done." );

		if( xml.name != "Stats" )
			WARN_AND_BREAK;

		LOAD_NODE( GeneralData );
		LOAD_NODE( SongScores );
		LOAD_NODE( CourseScores );
		LOAD_NODE( CategoryScores );
		LOAD_NODE( ScreenshotData );
		LOAD_NODE( CalorieData );
		LOAD_NODE( Awards );
		LOAD_NODE( RecentSongScores );
		LOAD_NODE( RecentCourseScores );
	}
		
	return true;	// FIXME?  Investigate what happens if we always return true.
}

bool Profile::SaveAllToDir( CString sDir, bool bSignData ) const
{
	m_sLastPlayedMachineGuid = PROFILEMAN->GetMachineProfile()->m_sGuid;

	// Save editable.xml
	SaveEditableDataToDir( sDir );

	// Save stats.xml
	{
		CString fn = sDir + STATS_XML;

		XNode xml;
		xml.name = "Stats";
		xml.AppendChild( SaveGeneralDataCreateNode() );
		xml.AppendChild( SaveSongScoresCreateNode() );
		xml.AppendChild( SaveCourseScoresCreateNode() );
		xml.AppendChild( SaveCategoryScoresCreateNode() );
		xml.AppendChild( SaveScreenshotDataCreateNode() );
		xml.AppendChild( SaveCalorieDataCreateNode() );
		xml.AppendChild( SaveAwardsCreateNode() );
		xml.AppendChild( SaveRecentSongScoresCreateNode() );
		xml.AppendChild( SaveRecentCourseScoresCreateNode() );
		bool bSaved = xml.SaveToFile(fn);
		
		// Update file cache, or else IsAFile in CryptManager won't see this new file.
		FILEMAN->FlushDirCache( sDir );
		
		if( bSaved && bSignData )
		{
			CString sStatsXmlSigFile = fn+SIGNATURE_APPEND;
			CryptManager::SignFileToFile(fn, sStatsXmlSigFile);

			// Update file cache, or else IsAFile in CryptManager won't see sStatsXmlSigFile.
			FILEMAN->FlushDirCache( sDir );

			// Save the "don't share" file
			CString sDontShareFile = sDir + DONT_SHARE_SIG;
			CryptManager::SignFileToFile(sStatsXmlSigFile, sDontShareFile);
		}
	}

	if( (IsMachine() && PREFSMAN->m_bWriteMachineStatsHtml) ||
		(!IsMachine() && PREFSMAN->m_bWritePlayerStatsHtml) )
		SaveStatsWebPageToDir( sDir );

	//
	// create edits dir
	//
	CString sEditsTempFile = sDir + EDITS_SUBDIR + "temp";
	RageFile f;
	f.Open( sDir + EDITS_SUBDIR + "temp", RageFile::WRITE );
	f.Close();

	FILEMAN->FlushDirCache( sDir + EDITS_SUBDIR );

	FILEMAN->Remove( sEditsTempFile );

	return true;
}

void Profile::SaveEditableDataToDir( CString sDir ) const
{
	IniFile ini;

	ini.SetValue( "Editable", "DisplayName",			m_sDisplayName );
	ini.SetValue( "Editable", "LastUsedHighScoreName",	m_sLastUsedHighScoreName );
	ini.SetValue( "Editable", "WeightPounds",			m_iWeightPounds );

	ini.WriteFile( sDir + EDITABLE_INI );
}

XNode* Profile::SaveGeneralDataCreateNode() const
{
	XNode* pGeneralDataNode = new XNode;
	pGeneralDataNode->name = "GeneralData";

	// TRICKY: These are write-only elements that are never read again.  This 
	// data is required by other apps (like internet ranking), but is 
	// redundant to the game app.
	pGeneralDataNode->AppendChild( "DisplayName",					GetDisplayName() );
	pGeneralDataNode->AppendChild( "IsMachine",						IsMachine() );

	pGeneralDataNode->AppendChild( "Guid",							m_sGuid );
	pGeneralDataNode->AppendChild( "UsingProfileDefaultModifiers",	m_bUsingProfileDefaultModifiers );
	pGeneralDataNode->AppendChild( "DefaultModifiers",				m_sDefaultModifiers );
	pGeneralDataNode->AppendChild( "SortOrder",						SortOrderToString(m_SortOrder) );
	pGeneralDataNode->AppendChild( "LastDifficulty",				DifficultyToString(m_LastDifficulty) );
	pGeneralDataNode->AppendChild( "LastCourseDifficulty",			CourseDifficultyToString(m_LastCourseDifficulty) );
	if( m_pLastSong )	pGeneralDataNode->AppendChild( "LastSong",	m_pLastSong->GetSongDir() );
	if( m_pLastCourse )	pGeneralDataNode->AppendChild( "LastCourse",m_pLastCourse->m_sPath );
	pGeneralDataNode->AppendChild( "TotalPlays",					m_iTotalPlays );
	pGeneralDataNode->AppendChild( "TotalPlaySeconds",				m_iTotalPlaySeconds );
	pGeneralDataNode->AppendChild( "TotalGameplaySeconds",			m_iTotalGameplaySeconds );
	pGeneralDataNode->AppendChild( "CurrentCombo",					m_iCurrentCombo );
	pGeneralDataNode->AppendChild( "TotalCaloriesBurned",			m_fTotalCaloriesBurned );
	pGeneralDataNode->AppendChild( "LastPlayedMachineGuid",			m_sLastPlayedMachineGuid );
	pGeneralDataNode->AppendChild( "TotalDancePoints",				m_iTotalDancePoints );
	pGeneralDataNode->AppendChild( "NumExtraStagesPassed",			m_iNumExtraStagesPassed );
	pGeneralDataNode->AppendChild( "NumExtraStagesFailed",			m_iNumExtraStagesFailed );
	pGeneralDataNode->AppendChild( "NumToasties",					m_iNumToasties );
	pGeneralDataNode->AppendChild( "TotalTapsAndHolds",				m_iTotalTapsAndHolds );
	pGeneralDataNode->AppendChild( "TotalJumps",					m_iTotalJumps );
	pGeneralDataNode->AppendChild( "TotalHolds",					m_iTotalHolds );
	pGeneralDataNode->AppendChild( "TotalMines",					m_iTotalMines );
	pGeneralDataNode->AppendChild( "TotalHands",					m_iTotalHands );

	{
		XNode* pUnlockedSongs = pGeneralDataNode->AppendChild("UnlockedSongs");
		for( set<int>::const_iterator it = m_UnlockedSongs.begin(); it != m_UnlockedSongs.end(); ++it )
			pUnlockedSongs->AppendChild( ssprintf("Unlock%i", *it) );
	}

	{
		XNode* pNumSongsPlayedByPlayMode = pGeneralDataNode->AppendChild("NumSongsPlayedByPlayMode");
		FOREACH_PlayMode( pm )
		{
			/* Don't save unplayed PlayModes. */
			if( !m_iNumSongsPlayedByPlayMode[pm] )
				continue;
			pNumSongsPlayedByPlayMode->AppendChild( PlayModeToString(pm), m_iNumSongsPlayedByPlayMode[pm] );
		}
	}

	{
		XNode* pNumSongsPlayedByStyle = pGeneralDataNode->AppendChild("NumSongsPlayedByStyle");
		for( int i=0; i<NUM_STYLES; i++ )
		{
			/* Don't save unplayed styles. */
			if( !m_iNumSongsPlayedByStyle[i] )
				continue;
			
			XNode *pStyleNode = pNumSongsPlayedByStyle->AppendChild( "Style", m_iNumSongsPlayedByStyle[i] );

			const StyleDef *pStyle = GAMEMAN->GetStyleDefForStyle((Style)i);
			const GameDef *g = GAMEMAN->GetGameDefForGame( pStyle->m_Game );
			ASSERT( g );
			pStyleNode->AppendAttr( "Game", g->m_szName );
			pStyleNode->AppendAttr( "Style", pStyle->m_szName );
		}
	}

	{
		XNode* pNumSongsPlayedByDifficulty = pGeneralDataNode->AppendChild("NumSongsPlayedByDifficulty");
		FOREACH_Difficulty( dc )
		{
			if( !m_iNumSongsPlayedByDifficulty[dc] )
				continue;
			pNumSongsPlayedByDifficulty->AppendChild( DifficultyToString(dc), m_iNumSongsPlayedByDifficulty[dc] );
		}
	}

	{
		XNode* pNumSongsPlayedByMeter = pGeneralDataNode->AppendChild("NumSongsPlayedByMeter");
		for( int i=0; i<MAX_METER+1; i++ )
		{
			if( !m_iNumSongsPlayedByMeter[i] )
				continue;
			pNumSongsPlayedByMeter->AppendChild( ssprintf("Meter%d",i), m_iNumSongsPlayedByMeter[i] );
		}
	}

	{
		XNode* pNumSongsPassedByGrade = pGeneralDataNode->AppendChild("NumSongsPassedByGrade");
		FOREACH_Grade( g )
		{
			if( !m_iNumSongsPassedByGrade[g] )
				continue;
			pNumSongsPassedByGrade->AppendChild( GradeToString(g), m_iNumSongsPassedByGrade[g] );
		}
	}

	{
		XNode* pNumSongsPassedByPlayMode = pGeneralDataNode->AppendChild("NumSongsPassedByPlayMode");
		FOREACH_PlayMode( pm )
		{
			/* Don't save unplayed PlayModes. */
			if( !m_iNumSongsPassedByPlayMode[pm] )
				continue;
			pNumSongsPassedByPlayMode->AppendChild( PlayModeToString(pm), m_iNumSongsPassedByPlayMode[pm] );
		}
	}

	return pGeneralDataNode;
}

void Profile::LoadEditableDataFromDir( CString sDir )
{
	CString fn = sDir + EDITABLE_INI;

	//
	// Don't load unreasonably large editable.xml files.
	//
	int iBytes = FILEMAN->GetFileSizeInBytes( fn );
	if( iBytes > MAX_EDITABLE_INI_SIZE_BYTES )
	{
		LOG->Warn( "The file '%s' is unreasonably large.  It won't be loaded.", fn.c_str() );
		return;
	}


	IniFile ini;
	ini.ReadFile( fn );

	ini.GetValue("Editable","DisplayName",				m_sDisplayName);
	ini.GetValue("Editable","LastUsedHighScoreName",	m_sLastUsedHighScoreName);
	ini.GetValue("Editable","WeightPounds",				m_iWeightPounds);

	// This is data that the user can change, so we have to validate it.
	wstring wstr = CStringToWstring(m_sDisplayName);
	if( wstr.size() > 12 )
		wstr = wstr.substr(0, 12);
	m_sDisplayName = WStringToCString(wstr);
	// TODO: strip invalid chars?
	if( m_iWeightPounds != 0 )
		CLAMP( m_iWeightPounds, 50, 500 );
}

void Profile::LoadGeneralDataFromNode( const XNode* pNode )
{
	ASSERT( pNode->name == "GeneralData" );

	CString s;

	pNode->GetChildValue( "Guid",							m_sGuid );
	pNode->GetChildValue( "UsingProfileDefaultModifiers",	m_bUsingProfileDefaultModifiers );
	pNode->GetChildValue( "DefaultModifiers",				m_sDefaultModifiers );
	pNode->GetChildValue( "SortOrder",						s );	m_SortOrder = StringToSortOrder( s );
	pNode->GetChildValue( "LastDifficulty",					s );	m_LastDifficulty = StringToDifficulty( s );
	pNode->GetChildValue( "LastCourseDifficulty",			s );	m_LastCourseDifficulty = StringToCourseDifficulty( s );
	pNode->GetChildValue( "LastSong",						s );	m_pLastSong = SONGMAN->GetSongFromDir(s);
	pNode->GetChildValue( "LastCourse",						s );	m_pLastCourse = SONGMAN->GetCourseFromPath(s);
	pNode->GetChildValue( "TotalPlays",						m_iTotalPlays );
	pNode->GetChildValue( "TotalPlaySeconds",				m_iTotalPlaySeconds );
	pNode->GetChildValue( "TotalGameplaySeconds",			m_iTotalGameplaySeconds );
	pNode->GetChildValue( "CurrentCombo",					m_iCurrentCombo );
	pNode->GetChildValue( "TotalCaloriesBurned",			m_fTotalCaloriesBurned );
	pNode->GetChildValue( "LastPlayedMachineGuid",			m_sLastPlayedMachineGuid );
	pNode->GetChildValue( "TotalDancePoints",				m_iTotalDancePoints );
	pNode->GetChildValue( "NumExtraStagesPassed",			m_iNumExtraStagesPassed );
	pNode->GetChildValue( "NumExtraStagesFailed",			m_iNumExtraStagesFailed );
	pNode->GetChildValue( "NumToasties",					m_iNumToasties );
	pNode->GetChildValue( "TotalTapsAndHolds",				m_iTotalTapsAndHolds );
	pNode->GetChildValue( "TotalJumps",						m_iTotalJumps );
	pNode->GetChildValue( "TotalHolds",						m_iTotalHolds );
	pNode->GetChildValue( "TotalMines",						m_iTotalMines );
	pNode->GetChildValue( "TotalHands",						m_iTotalHands );

	{
		XNode* pUnlockedSongs = pNode->GetChild("UnlockedSongs");
		if( pUnlockedSongs )
		{
			FOREACH_Node( pUnlockedSongs, song )
			{
				int iUnlock;
				if( sscanf(song->name.c_str(),"Unlock%d",&iUnlock) == 1 )
					m_UnlockedSongs.insert( iUnlock );
			}
		}
	}

	{
		XNode* pNumSongsPlayedByPlayMode = pNode->GetChild("NumSongsPlayedByPlayMode");
		if( pNumSongsPlayedByPlayMode )
			FOREACH_PlayMode( pm )
				pNumSongsPlayedByPlayMode->GetChildValue( PlayModeToString(pm), m_iNumSongsPlayedByPlayMode[pm] );
	}

	{
		XNode* pNumSongsPlayedByStyle = pNode->GetChild("NumSongsPlayedByStyle");
		if( pNumSongsPlayedByStyle )
		{
			FOREACH_Node( pNumSongsPlayedByStyle, style )
			{
				if( style->name != "Style" )
					continue;

				CString sGame;
				if( !style->GetAttrValue( "Game", sGame ) )
					WARN_AND_CONTINUE;
				Game g = GAMEMAN->StringToGameType( sGame );
				if( g == GAME_INVALID )
					WARN_AND_CONTINUE;

				CString sStyle;
				if( !style->GetAttrValue( "Style", sStyle ) )
					WARN_AND_CONTINUE;
				Style s = GAMEMAN->GameAndStringToStyle( g, sStyle );
				if( s == STYLE_INVALID )
					WARN_AND_CONTINUE;

				style->GetValue( m_iNumSongsPlayedByStyle[s] );
			}
		}
	}

	{
		XNode* pNumSongsPlayedByDifficulty = pNode->GetChild("NumSongsPlayedByDifficulty");
		if( pNumSongsPlayedByDifficulty )
			FOREACH_Difficulty( dc )
				pNumSongsPlayedByDifficulty->GetChildValue( DifficultyToString(dc), m_iNumSongsPlayedByDifficulty[dc] );
	}

	{
		XNode* pNumSongsPlayedByMeter = pNode->GetChild("NumSongsPlayedByMeter");
		if( pNumSongsPlayedByMeter )
			for( int i=0; i<MAX_METER+1; i++ )
				pNumSongsPlayedByMeter->GetChildValue( ssprintf("Meter%d",i), m_iNumSongsPlayedByMeter[i] );
	}

	{
		XNode* pNumSongsPassedByGrade = pNode->GetChild("NumSongsPassedByGrade");
		if( pNumSongsPassedByGrade )
			FOREACH_Grade( g )
				pNumSongsPassedByGrade->GetChildValue( GradeToString(g), m_iNumSongsPassedByGrade[g] );
	}

	{
		XNode* pNumSongsPassedByPlayMode = pNode->GetChild("NumSongsPassedByPlayMode");
		if( pNumSongsPassedByPlayMode )
			FOREACH_PlayMode( pm )
				pNumSongsPassedByPlayMode->GetChildValue( PlayModeToString(pm), m_iNumSongsPassedByPlayMode[pm] );
	
	}

}

void Profile::AddStepTotals( int iTotalTapsAndHolds, int iTotalJumps, int iTotalHolds, int iTotalMines, int iTotalHands )
{
	m_iTotalTapsAndHolds += iTotalTapsAndHolds;
	m_iTotalJumps += iTotalJumps;
	m_iTotalHolds += iTotalHolds;
	m_iTotalMines += iTotalMines;
	m_iTotalHands += iTotalHands;

	if( m_iWeightPounds != 0 )
	{
		float fCals = 
			SCALE( m_iWeightPounds, 100.f, 200.f, 0.029f, 0.052f ) * iTotalTapsAndHolds +
			SCALE( m_iWeightPounds, 100.f, 200.f, 0.111f, 0.193f ) * iTotalJumps +
			SCALE( m_iWeightPounds, 100.f, 200.f, 0.029f, 0.052f ) * iTotalHolds +
			SCALE( m_iWeightPounds, 100.f, 200.f, 0.000f, 0.000f ) * iTotalMines +
			SCALE( m_iWeightPounds, 100.f, 200.f, 0.222f, 0.386f ) * iTotalHands;
		m_fTotalCaloriesBurned += fCals;

		tm cur_tm = GetLocalTime();
		Day day = { cur_tm.tm_yday, cur_tm.tm_year+1900 };
		m_mapDayToCaloriesBurned[day] += fCals;
	}
}

XNode* Profile::SaveSongScoresCreateNode() const
{
	CHECKPOINT;

	const Profile* pProfile = this;
	ASSERT( pProfile );

	XNode* pNode = new XNode;
	pNode->name = "SongScores";

	for( std::map<SongID,HighScoresForASong>::const_iterator i = m_SongHighScores.begin();
		i != m_SongHighScores.end();
		i++ )
	{	
		const SongID &songID = i->first;
		const HighScoresForASong &hsSong = i->second;

		// skip songs that have never been played
		if( pProfile->GetSongNumTimesPlayed(songID) == 0 )
			continue;

		XNode* pSongNode = pNode->AppendChild( songID.CreateNode() );

		for( std::map<StepsID,HighScoresForASteps>::const_iterator j = hsSong.m_StepsHighScores.begin();
			j != hsSong.m_StepsHighScores.end();
			j++ )
		{	
			const StepsID &stepsID = j->first;
			const HighScoresForASteps &hsSteps = j->second;

			const HighScoreList &hsl = hsSteps.hs;

			// skip steps that have never been played
			if( hsl.iNumTimesPlayed == 0 )
				continue;

			XNode* pStepsNode = pSongNode->AppendChild( stepsID.CreateNode() );

			pStepsNode->AppendChild( hsl.CreateNode() );
		}
	}
	
	return pNode;
}

void Profile::LoadSongScoresFromNode( const XNode* pNode )
{
	CHECKPOINT;

	ASSERT( pNode->name == "SongScores" );

	FOREACH_CONST( XNode*, pNode->childs, song )
	{
		if( (*song)->name != "Song" )
			continue;

		SongID songID;
		songID.LoadFromNode( *song );
		if( !songID.IsValid() )
			WARN_AND_CONTINUE;

		FOREACH_CONST( XNode*, (*song)->childs, steps )
		{
			if( (*steps)->name != "Steps" )
				continue;

			StepsID stepsID;
			stepsID.LoadFromNode( *steps );
			if( !stepsID.IsValid() )
				WARN_AND_CONTINUE;

			XNode *pHighScoreListNode = (*steps)->GetChild("HighScoreList");
			if( pHighScoreListNode == NULL )
				WARN_AND_CONTINUE;
			
			HighScoreList &hsl = m_SongHighScores[songID].m_StepsHighScores[stepsID].hs;
			hsl.LoadFromNode( pHighScoreListNode );
		}
	}
}


XNode* Profile::SaveCourseScoresCreateNode() const
{
	CHECKPOINT;

	const Profile* pProfile = this;
	ASSERT( pProfile );

	XNode* pNode = new XNode;
	pNode->name = "CourseScores";

	
	for( std::map<CourseID,HighScoresForACourse>::const_iterator i = m_CourseHighScores.begin();
		i != m_CourseHighScores.end();
		i++ )
	{
		const CourseID &courseID = i->first;
		const HighScoresForACourse &hsCourse = i->second;

		// skip courses that have never been played
		if( pProfile->GetCourseNumTimesPlayed(courseID) == 0 )
			continue;

		XNode* pCourseNode = pNode->AppendChild( courseID.CreateNode() );

		for( std::map<TrailID,HighScoresForATrail>::const_iterator j = hsCourse.m_TrailHighScores.begin();
			j != hsCourse.m_TrailHighScores.end();
			j++ )
		{
			const TrailID &trailID = j->first;
			const HighScoresForATrail &hsTrail = j->second;

			const HighScoreList &hsl = hsTrail.hs;

			// skip steps that have never been played
			if( hsl.iNumTimesPlayed == 0 )
				continue;

			XNode* pTrailNode = pCourseNode->AppendChild( trailID.CreateNode() );

			pTrailNode->AppendChild( hsl.CreateNode() );
		}
	}

	return pNode;
}

void Profile::LoadCourseScoresFromNode( const XNode* pNode )
{
	CHECKPOINT;

	ASSERT( pNode->name == "CourseScores" );

	FOREACH_CONST( XNode*, pNode->childs, course )
	{
		if( (*course)->name != "Course" )
			continue;

		CourseID courseID;
		courseID.LoadFromNode( *course );
		if( !courseID.IsValid() )
			WARN_AND_CONTINUE;

		FOREACH_CONST( XNode*, (*course)->childs, trail )
		{
			if( (*trail)->name != "Trail" )
				continue;
			
			TrailID trailID;
			trailID.LoadFromNode( *trail );
			if( !trailID.IsValid() )
				WARN_AND_CONTINUE;

			XNode *pHighScoreListNode = (*trail)->GetChild("HighScoreList");
			if( pHighScoreListNode == NULL )
				WARN_AND_CONTINUE;
			
			HighScoreList &hsl = m_CourseHighScores[courseID].m_TrailHighScores[trailID].hs;
			hsl.LoadFromNode( pHighScoreListNode );
		}
	}
}

XNode* Profile::SaveCategoryScoresCreateNode() const
{
	CHECKPOINT;

	const Profile* pProfile = this;
	ASSERT( pProfile );

	XNode* pNode = new XNode;
	pNode->name = "CategoryScores";

	FOREACH_StepsType( st )
	{
		// skip steps types that have never been played
		if( pProfile->GetCategoryNumTimesPlayed( st ) == 0 )
			continue;

		XNode* pStepsTypeNode = pNode->AppendChild( "StepsType" );
		pStepsTypeNode->AppendAttr( "Type", GameManager::NotesTypeToString(st) );

		FOREACH_RankingCategory( rc )
		{
			// skip steps types/categories that have never been played
			if( pProfile->GetCategoryHighScoreList(st,rc).iNumTimesPlayed == 0 )
				continue;

			XNode* pRankingCategoryNode = pStepsTypeNode->AppendChild( "RankingCategory" );
			pRankingCategoryNode->AppendAttr( "Type", RankingCategoryToString(rc) );

			const HighScoreList &hsl = pProfile->GetCategoryHighScoreList( (StepsType)st, (RankingCategory)rc );

			pRankingCategoryNode->AppendChild( hsl.CreateNode() );
		}
	}

	return pNode;
}

void Profile::LoadCategoryScoresFromNode( const XNode* pNode )
{
	CHECKPOINT;

	ASSERT( pNode->name == "CategoryScores" );

	for( XNodes::const_iterator stepsType = pNode->childs.begin(); 
		stepsType != pNode->childs.end(); 
		stepsType++ )
	{
		if( (*stepsType)->name != "StepsType" )
			continue;

		CString str;
		if( !(*stepsType)->GetAttrValue( "Type", str ) )
			WARN_AND_CONTINUE;
		StepsType st = GameManager::StringToNotesType( str );
		if( st == STEPS_TYPE_INVALID )
			WARN_AND_CONTINUE;

		for( XNodes::iterator radarCategory = (*stepsType)->childs.begin(); 
			radarCategory != (*stepsType)->childs.end(); 
			radarCategory++ )
		{
			if( (*radarCategory)->name != "RankingCategory" )
				continue;

			if( !(*radarCategory)->GetAttrValue( "Type", str ) )
				WARN_AND_CONTINUE;
			RankingCategory rc = StringToRankingCategory( str );
			if( rc == RANKING_INVALID )
				WARN_AND_CONTINUE;

			XNode *pHighScoreListNode = (*radarCategory)->GetChild("HighScoreList");
			if( pHighScoreListNode == NULL )
				WARN_AND_CONTINUE;
			
			HighScoreList &hsl = this->GetCategoryHighScoreList( st, rc );
			hsl.LoadFromNode( pHighScoreListNode );
		}
	}
}

void Profile::SaveStatsWebPageToDir( CString sDir ) const
{
	ASSERT( PROFILEMAN );

	SaveStatsWebPage( 
		sDir,
		this,
		PROFILEMAN->GetMachineProfile(),
		IsMachine() ? HTML_TYPE_MACHINE : HTML_TYPE_PLAYER
		);
}

void Profile::SaveMachinePublicKeyToDir( CString sDir ) const
{
	if( PREFSMAN->m_bSignProfileData && IsAFile(CRYPTMAN->GetPublicKeyFileName()) )
		FileCopy( CRYPTMAN->GetPublicKeyFileName(), sDir+PUBLIC_KEY_FILE );
}

void Profile::AddScreenshot( const Screenshot &screenshot )
{
	m_vScreenshots.push_back( screenshot );
}

void Profile::LoadScreenshotDataFromNode( const XNode* pNode )
{
	CHECKPOINT;

	ASSERT( pNode->name == "ScreenshotData" );
	FOREACH_CONST( XNode*, pNode->childs, screenshot )
	{
		if( (*screenshot)->name != "Screenshot" )
			WARN_AND_CONTINUE;

		Screenshot ss;
		ss.LoadFromNode( *screenshot );

		m_vScreenshots.push_back( ss );
	}	
}

XNode* Profile::SaveScreenshotDataCreateNode() const
{
	CHECKPOINT;

	const Profile* pProfile = this;
	ASSERT( pProfile );

	XNode* pNode = new XNode;
	pNode->name = "ScreenshotData";

	FOREACH_CONST( Screenshot, m_vScreenshots, ss )
	{
		pNode->AppendChild( ss->CreateNode() );
	}

	return pNode;
}

void Profile::LoadCalorieDataFromNode( const XNode* pNode )
{
	CHECKPOINT;

	ASSERT( pNode->name == "CalorieData" );
	FOREACH_CONST( XNode*, pNode->childs, pDay )
	{
		if( (*pDay)->name != "Day" )
			WARN_AND_CONTINUE;

		Day day;
		
		if( !(*pDay)->GetAttrValue("DayInYear",day.iDayInYear) )
			WARN_AND_CONTINUE;

		if( !(*pDay)->GetAttrValue("Year",day.iYear) )
			WARN_AND_CONTINUE;

		float fCaloriesBurned = 0;

		if( !(*pDay)->GetChildValue("CaloriesBurned",fCaloriesBurned) )
			WARN_AND_CONTINUE;

		m_mapDayToCaloriesBurned[day] = fCaloriesBurned;
	}	
}

XNode* Profile::SaveCalorieDataCreateNode() const
{
	CHECKPOINT;

	const Profile* pProfile = this;
	ASSERT( pProfile );

	XNode* pNode = new XNode;
	pNode->name = "CalorieData";

	for( map<Day,float>::const_iterator i = m_mapDayToCaloriesBurned.begin();
		i != m_mapDayToCaloriesBurned.end();
		i++ )
	{
		XNode* pDay = pNode->AppendChild( "Day" );

		pDay->AppendAttr( "DayInYear", i->first.iDayInYear );
		pDay->AppendAttr( "Year", i->first.iYear );

		pDay->AppendChild( "CaloriesBurned", i->second );
	}

	return pNode;
}

float Profile::GetCaloriesBurnedForDay( Day day ) const
{
	map<Day,float>::const_iterator i = m_mapDayToCaloriesBurned.find( day );
	if( i == m_mapDayToCaloriesBurned.end() )
		return 0;
	else
		return i->second;
}

XNode* Profile::AwardRecord::CreateNode() const
{
	XNode* pNode = new XNode;
	pNode->name = "AwardRecord";

	pNode->AppendChild( "FirstTime", int(first) );
	pNode->AppendChild( "LastTime",	int(last) );
	pNode->AppendChild( "Count",	iCount );

	return pNode;
}

void Profile::AwardRecord::LoadFromNode( const XNode* pNode )
{
	Unset();

	ASSERT( pNode->name == "AwardRecord" );
	pNode->GetChildValue( "FirstTime", (int&)first );	// time_t is a signed long in Win32.  Is this OK on other platforms?
	pNode->GetChildValue( "LastTime", (int&)last );
	pNode->GetChildValue( "Count", iCount );
}

void Profile::LoadAwardsFromNode( const XNode* pNode )
{
	CHECKPOINT;

	ASSERT( pNode->name == "Awards" );
	for( XNodes::const_iterator pAward = pNode->childs.begin(); 
		pAward != pNode->childs.end(); 
		pAward++ )
	{
		if( (*pAward)->name == "PerDifficultyAward" )
		{
			CString sStepsType;
			if( !(*pAward)->GetAttrValue( "StepsType", sStepsType ) )
				WARN_AND_CONTINUE;
			StepsType st = GameManager::StringToNotesType( sStepsType );
			if( st == STEPS_TYPE_INVALID )
				WARN_AND_CONTINUE;

			CString sDifficulty;
			if( !(*pAward)->GetAttrValue( "Difficulty", sDifficulty ) )
				WARN_AND_CONTINUE;
			Difficulty dc = StringToDifficulty( sDifficulty );
			if( dc == DIFFICULTY_INVALID )
				WARN_AND_CONTINUE;

			CString sPerDifficultyAward;
			if( !(*pAward)->GetAttrValue( "PerDifficultyAward", sPerDifficultyAward ) )
				WARN_AND_CONTINUE;
			PerDifficultyAward pda = StringToPerDifficultyAward( sPerDifficultyAward );
			if( pda == PER_DIFFICULTY_AWARD_INVALID )
				WARN_AND_CONTINUE;

			AwardRecord &ar = m_PerDifficultyAwards[st][dc][pda];
			
			XNode* pAwardRecord = (*pAward)->GetChild("AwardRecord");
			if( pAwardRecord == NULL )
				WARN_AND_CONTINUE;

			ar.LoadFromNode( pAwardRecord );
		}
		else if( (*pAward)->name == "PeakComboAward" )
		{
			CString sPeakComboAward;
			if( !(*pAward)->GetAttrValue( "PeakComboAward", sPeakComboAward ) )
				WARN_AND_CONTINUE;
			PeakComboAward pca = StringToPeakComboAward( sPeakComboAward );
			if( pca == PEAK_COMBO_AWARD_INVALID )
				WARN_AND_CONTINUE;

			AwardRecord &ar = m_PeakComboAwards[pca];
			
			XNode* pAwardRecord = (*pAward)->GetChild("AwardRecord");
			if( pAwardRecord == NULL )
				WARN_AND_CONTINUE;

			ar.LoadFromNode( pAwardRecord );
		}
		else
			WARN_AND_CONTINUE;
	}	
}

XNode* Profile::SaveAwardsCreateNode() const
{
	CHECKPOINT;

	const Profile* pProfile = this;
	ASSERT( pProfile );

	XNode* pNode = new XNode;
	pNode->name = "Awards";

	FOREACH_StepsType( st )
	{
		FOREACH_Difficulty( dc )
		{
			FOREACH_PerDifficultyAward( pda )
			{
				const AwardRecord &ar = m_PerDifficultyAwards[st][dc][pda];
				if( !ar.IsSet() )
					continue;

				XNode* pAward = pNode->AppendChild( "PerDifficultyAward" );

				pAward->AppendAttr( "StepsType", GameManager::NotesTypeToString(st) );
				pAward->AppendAttr( "Difficulty", DifficultyToString(dc) );
				pAward->AppendAttr( "PerDifficultyAward", PerDifficultyAwardToString(pda) );
				
				pAward->AppendChild( ar.CreateNode() );
			}
		}
	}

	FOREACH_PeakComboAward( pca )
	{
		const AwardRecord &ar = m_PeakComboAwards[pca];
		if( !ar.IsSet() )
			continue;

		XNode* pAward = pNode->AppendChild( "PeakComboAward" );

		pAward->AppendAttr( "PeakComboAward", PeakComboAwardToString(pca) );
		
		pAward->AppendChild( ar.CreateNode() );
	}

	return pNode;
}

void Profile::AddPerDifficultyAward( StepsType st, Difficulty dc, PerDifficultyAward pda )
{
	m_PerDifficultyAwards[st][dc][pda].Set( time(NULL) );
}

void Profile::AddPeakComboAward( PeakComboAward pca )
{
	m_PeakComboAwards[pca].Set( time(NULL) );
}

bool Profile::HasPerDifficultyAward( StepsType st, Difficulty dc, PerDifficultyAward pda )
{
	return m_PerDifficultyAwards[st][dc][pda].IsSet();
}

bool Profile::HasPeakComboAward( PeakComboAward pca )
{
	return m_PeakComboAwards[pca].IsSet();
}


XNode* Profile::HighScoreForASongAndSteps::CreateNode() const
{
	XNode* pNode = new XNode;
	pNode->name = "HighScoreForASongAndSteps";

	pNode->AppendChild( songID.CreateNode() );
	pNode->AppendChild( stepsID.CreateNode() );
	pNode->AppendChild( hs.CreateNode() );

	return pNode;
}

void Profile::HighScoreForASongAndSteps::LoadFromNode( const XNode* pNode )
{
	Unset();

	ASSERT( pNode->name == "HighScoreForASongAndSteps" );
	XNode* p;
	if( (p = pNode->GetChild("Song")) )
		songID.LoadFromNode( p );
	if( (p = pNode->GetChild("Steps")) )
		stepsID.LoadFromNode( p );
	if( (p = pNode->GetChild("HighScore")) )
		hs.LoadFromNode( p );
}

void Profile::LoadRecentSongScoresFromNode( const XNode* pNode )
{
	CHECKPOINT;

	ASSERT( pNode->name == "RecentSongScores" );
	for( XNodes::const_iterator p = pNode->childs.begin(); 
		p != pNode->childs.end(); 
		p++ )
	{
		if( (*p)->name == "HighScoreForASongAndSteps" )
		{
			HighScoreForASongAndSteps h;
			h.LoadFromNode( *p );

			m_vRecentStepsScores.push_back( h );
		}
		else
			WARN_AND_CONTINUE;
	}	
}

XNode* Profile::SaveRecentSongScoresCreateNode() const
{
	CHECKPOINT;

	const Profile* pProfile = this;
	ASSERT( pProfile );

	XNode* pNode = new XNode;
	pNode->name = "RecentSongScores";

	unsigned uNumToSave = min( m_vRecentStepsScores.size(), (unsigned)MAX_RECENT_SCORES_TO_SAVE );

	for( unsigned i=0; i<uNumToSave; i++ )
	{
		pNode->AppendChild( m_vRecentStepsScores[i].CreateNode() );
	}

	return pNode;
}

void Profile::AddStepsRecentScore( const Song* pSong, const Steps* pSteps, HighScore hs )
{
	HighScoreForASongAndSteps h;
	h.songID.FromSong( pSong );
	h.stepsID.FromSteps( pSteps );
	h.hs = hs;
	m_vRecentStepsScores.push_back( h );
}


XNode* Profile::HighScoreForACourse::CreateNode() const
{
	XNode* pNode = new XNode;
	pNode->name = "HighScoreForACourse";

	pNode->AppendChild( courseID.CreateNode() );
	pNode->AppendChild( hs.CreateNode() );

	return pNode;
}

void Profile::HighScoreForACourse::LoadFromNode( const XNode* pNode )
{
	Unset();

	ASSERT( pNode->name == "HighScoreForACourse" );
	XNode* p;
	if( (p = pNode->GetChild("Course")) )
		courseID.LoadFromNode( p );
	if( (p = pNode->GetChild("HighScore")) )
		hs.LoadFromNode( p );
}

void Profile::LoadRecentCourseScoresFromNode( const XNode* pNode )
{
	CHECKPOINT;

	ASSERT( pNode->name == "RecentCourseScores" );
	for( XNodes::const_iterator p = pNode->childs.begin(); 
		p != pNode->childs.end(); 
		p++ )
	{
		if( (*p)->name == "HighScoreForACourse" )
		{
			HighScoreForACourse h;
			h.LoadFromNode( *p );

			m_vRecentCourseScores.push_back( h );
		}
		else
			WARN_AND_CONTINUE;
	}	
}

XNode* Profile::SaveRecentCourseScoresCreateNode() const
{
	CHECKPOINT;

	const Profile* pProfile = this;
	ASSERT( pProfile );

	XNode* pNode = new XNode;
	pNode->name = "RecentCourseScores";

	unsigned uNumToSave = min( m_vRecentCourseScores.size(), (unsigned)MAX_RECENT_SCORES_TO_SAVE );

	for( unsigned i=0; i<uNumToSave; i++ )
	{
		pNode->AppendChild( m_vRecentCourseScores[i].CreateNode() );
	}

	return pNode;
}

void Profile::AddCourseRecentScore( const Course* pCourse, const Trail* pTrail, HighScore hs )
{
	HighScoreForACourse h;
	h.courseID.FromCourse( pCourse );
	h.trailID.FromTrail( pTrail );
	h.hs = hs;
	m_vRecentCourseScores.push_back( h );
}

const Profile::HighScoresForASong *Profile::GetHighScoresForASong( const SongID& songID ) const
{
	std::map<SongID,HighScoresForASong>::const_iterator it;
	it = m_SongHighScores.find( songID );
	if( it == m_SongHighScores.end() )
		return NULL;
	return &it->second;
}

const Profile::HighScoresForACourse *Profile::GetHighScoresForACourse( const CourseID& courseID ) const
{
	std::map<CourseID,HighScoresForACourse>::const_iterator it;
	it = m_CourseHighScores.find( courseID );
	if( it == m_CourseHighScores.end() )
		return NULL;
	return &it->second;
}

bool Profile::IsMachine() const
{
	// TODO: Think of a better way to handle this
	return this == PROFILEMAN->GetMachineProfile();
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
