#include "global.h"
#include "Profile.h"
#include "RageUtil.h"
#include "PrefsManager.h"
#include "XmlFile.h"
#include "IniFile.h"
#include "GameManager.h"
#include "RageLog.h"
#include "song.h"
#include "SongManager.h"
#include "Steps.h"
#include "Course.h"
#include "ThemeManager.h"
#include "CryptManager.h"
#include "ProfileManager.h"
#include "RageFileManager.h"
#include "LuaManager.h"
#include "crypto/CryptRand.h"
#include "UnlockManager.h"
#include "XmlFile.h"
#include "XmlFileUtil.h"
#include "Foreach.h"
#include "CatalogXml.h"
#include "Bookkeeper.h"
#include "Game.h"
#include "CharacterManager.h"
#include "Character.h"

const RString STATS_XSL            = "Stats.xsl";
const RString COMMON_XSL           = "Common.xsl";
const RString STATS_XML            = "Stats.xml";
const RString EDITABLE_INI         = "Editable.ini";
const RString DONT_SHARE_SIG       = "DontShare.sig";
const RString PUBLIC_KEY_FILE      = "public.key";
const RString SCREENSHOTS_SUBDIR   = "Screenshots/";
const RString EDIT_STEPS_SUBDIR    = "Edits/";
const RString EDIT_COURSES_SUBDIR  = "EditCourses/";

ThemeMetric<bool> SHOW_COIN_DATA( "Profile", "ShowCoinData" );

#define GUID_SIZE_BYTES 8

#define MAX_EDITABLE_INI_SIZE_BYTES			2*1024		// 2KB
#define MAX_PLAYER_STATS_XML_SIZE_BYTES	\
	100 /* Songs */						\
	* 3 /* Steps per Song */			\
	* 10 /* HighScores per Steps */		\
	* 1024 /* size in bytes of a HighScores XNode */

const unsigned int DEFAULT_WEIGHT_POUNDS	= 120;

#if defined(_MSC_VER)
#pragma warning (disable : 4706) // assignment within conditional expression
#endif


int Profile::HighScoresForASong::GetNumTimesPlayed() const
{
	int iCount = 0;
	FOREACHM_CONST( StepsID, HighScoresForASteps, m_StepsHighScores, i )
	{
		iCount += i->second.hsl.GetNumTimesPlayed();
	}
	return iCount;
}

int Profile::HighScoresForACourse::GetNumTimesPlayed() const
{
	int iCount = 0;
	FOREACHM_CONST( TrailID, HighScoresForATrail, m_TrailHighScores, i )
	{
		iCount += i->second.hsl.GetNumTimesPlayed();
	}
	return iCount;
}


void Profile::InitEditableData()
{
	m_sDisplayName = "";
	m_sCharacterID = "";
	m_sLastUsedHighScoreName = "";
	m_iWeightPounds = 0;
}

RString Profile::MakeGuid()
{
	// Does the RNG need to be inited and seeded every time?
	random_init();
	random_add_noise( "ai8049ujr3odusj" );
	
	RString s;
	for( unsigned i=0; i<GUID_SIZE_BYTES; i++ )
		s += ssprintf( "%02x", random_byte() );
	return s;
}

void Profile::InitGeneralData()
{
	m_sGuid = MakeGuid();

	m_SortOrder = SORT_INVALID;
	m_LastDifficulty = DIFFICULTY_INVALID;
	m_LastCourseDifficulty = DIFFICULTY_INVALID;
	m_lastSong.Unset();
	m_lastCourse.Unset();
	m_iTotalPlays = 0;
	m_iTotalPlaySeconds = 0;
	m_iTotalGameplaySeconds = 0;
	m_iCurrentCombo = 0;
	m_fTotalCaloriesBurned = 0;
	m_GoalType = (GoalType)0;
	m_iGoalCalories = 0;
	m_iGoalSeconds = 0;
	m_iTotalDancePoints = 0;
	m_iNumExtraStagesPassed = 0;
	m_iNumExtraStagesFailed = 0;
	m_iNumToasties = 0;
	m_UnlockedEntryIDs.clear();
	m_sLastPlayedMachineGuid = "";
	m_LastPlayedDate.Init();
	m_iTotalTapsAndHolds = 0;
	m_iTotalJumps = 0;
	m_iTotalHolds = 0;
	m_iTotalRolls = 0;
	m_iTotalMines = 0;
	m_iTotalHands = 0;

	FOREACH_PlayMode( i )
		m_iNumSongsPlayedByPlayMode[i] = 0;
	m_iNumSongsPlayedByStyle.clear();
	FOREACH_Difficulty( i )
		m_iNumSongsPlayedByDifficulty[i] = 0;
	for( int i=0; i<MAX_METER+1; i++ )
		m_iNumSongsPlayedByMeter[i] = 0;
	m_iNumTotalSongsPlayed = 0;
	ZERO( m_iNumStagesPassedByPlayMode );
	ZERO( m_iNumStagesPassedByGrade );

	Lua *L = LUA->Get();
	lua_newtable( L );
	m_SavedLuaData.SetFromStack( L );
	LUA->Release( L );
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

void Profile::InitRecentSongScores()
{
	m_vRecentStepsScores.clear();
}

void Profile::InitRecentCourseScores()
{
	m_vRecentCourseScores.clear();
}

RString Profile::GetDisplayNameOrHighScoreName() const
{
	if( !m_sDisplayName.empty() )
		return m_sDisplayName;
	else if( !m_sLastUsedHighScoreName.empty() )
		return m_sLastUsedHighScoreName;
	else
		return RString();
}

Character *Profile::GetCharacter() const
{
	vector<Character*> vpCharacters;
	CHARMAN->GetCharacters( vpCharacters );
	FOREACH_CONST( Character*, vpCharacters, c )
	{
		if( (*c)->m_sCharacterID.CompareNoCase(m_sCharacterID)==0 )
			return *c;
	}
	return CHARMAN->GetDefaultCharacter();
}

static RString FormatCalories( float fCals )
{
	return Commify((int)fCals) + " Cal";
}

int Profile::GetCalculatedWeightPounds() const
{
	if( m_iWeightPounds == 0 )	// weight not entered
		return DEFAULT_WEIGHT_POUNDS;
	else 
		return m_iWeightPounds;
}

RString Profile::GetDisplayTotalCaloriesBurned() const
{
	return FormatCalories( m_fTotalCaloriesBurned );
}

RString Profile::GetDisplayTotalCaloriesBurnedToday() const
{
	float fCals = GetCaloriesBurnedToday();
	return FormatCalories( fCals );
}

float Profile::GetCaloriesBurnedToday() const
{
	DateTime now = DateTime::GetNowDate();
	return GetCaloriesBurnedForDay(now);
}

int Profile::GetTotalNumSongsPassed() const
{
	int iTotal = 0;
	FOREACH_PlayMode( i )
		iTotal += m_iNumStagesPassedByPlayMode[i];
	return iTotal;
}

int Profile::GetTotalStepsWithTopGrade( StepsType st, Difficulty d, Grade g ) const
{
	int iCount = 0;

	FOREACH_CONST( Song*, SONGMAN->GetAllSongs(), pSong )
	{
		if( (*pSong)->m_SelectionDisplay == Song::SHOW_NEVER )
			continue;	// skip

		FOREACH_CONST( Steps*, (*pSong)->GetAllSteps(), pSteps )
		{
			if( (*pSteps)->m_StepsType != st )
				continue;	// skip

			if( (*pSteps)->GetDifficulty() != d )
				continue;	// skip

			const HighScoreList &hsl = GetStepsHighScoreList( *pSong, *pSteps );
			if( hsl.vHighScores.empty() )
				continue;	// skip

			if( hsl.vHighScores[0].GetGrade() == g )
				iCount++;
		}
	}

	return iCount;
}

int Profile::GetTotalTrailsWithTopGrade( StepsType st, CourseDifficulty d, Grade g ) const
{
	int iCount = 0;

	// add course high scores
	vector<Course*> vCourses;
	SONGMAN->GetAllCourses( vCourses, false );
	FOREACH_CONST( Course*, vCourses, pCourse )
	{
		// Don't count any course that has any entries that change over time.
		if( !(*pCourse)->AllSongsAreFixed() )
			continue;

		vector<Trail*> vTrails;
		Trail* pTrail = (*pCourse)->GetTrail( st, d );
		if( pTrail == NULL )
			continue;

		const HighScoreList &hsl = GetCourseHighScoreList( *pCourse, pTrail );
		if( hsl.vHighScores.empty() )
			continue;	// skip

		if( hsl.vHighScores[0].GetGrade() == g )
			iCount++;
	}

	return iCount;
}

float Profile::GetSongsPossible( StepsType st, Difficulty dc ) const
{
	int iTotalSteps = 0;

	// add steps high scores
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

			if( pSteps->GetDifficulty() != dc )
				continue;	// skip

			iTotalSteps++;
		}
	}

	return (float) iTotalSteps;
}

float Profile::GetSongsActual( StepsType st, Difficulty dc ) const
{
	CHECKPOINT_M( ssprintf("Profile::GetSongsActual(%d,%d)",st,dc) );
	
	float fTotalPercents = 0;
	
	// add steps high scores
	FOREACHM_CONST( SongID, HighScoresForASong, m_SongHighScores, i )
	{
		const SongID &id = i->first;
		Song* pSong = id.ToSong();
		
		CHECKPOINT_M( ssprintf("Profile::GetSongsActual: %p", pSong) );
		
		// If the Song isn't loaded on the current machine, then we can't 
		// get radar values to compute dance points.
		if( pSong == NULL )
			continue;
		
		if( pSong->m_SelectionDisplay == Song::SHOW_NEVER )
			continue;	// skip
		
		CHECKPOINT_M( ssprintf("Profile::GetSongsActual: song %s", pSong->GetSongDir().c_str()) );
		const HighScoresForASong &hsfas = i->second;
		
		FOREACHM_CONST( StepsID, HighScoresForASteps, hsfas.m_StepsHighScores, j )
		{
			const StepsID &id = j->first;
			Steps* pSteps = id.ToSteps( pSong, true );
			CHECKPOINT_M( ssprintf("Profile::GetSongsActual: song %p, steps %p", pSong, pSteps) );
			
			// If the Steps isn't loaded on the current machine, then we can't 
			// get radar values to compute dance points.
			if( pSteps == NULL )
				continue;
			
			if( pSteps->m_StepsType != st )
				continue;
			
			CHECKPOINT_M( ssprintf("Profile::GetSongsActual: n %s = %p", id.ToString().c_str(), pSteps) );
			if( pSteps->GetDifficulty() != dc )
				continue;	// skip
			CHECKPOINT;
			
			const HighScoresForASteps& h = j->second;
			const HighScoreList& hsl = h.hsl;
			
			fTotalPercents += hsl.GetTopScore().GetPercentDP();
		}
		CHECKPOINT;
	}

	return fTotalPercents;
}

float Profile::GetSongsPercentComplete( StepsType st, Difficulty dc ) const
{
	return GetSongsActual(st,dc) / GetSongsPossible(st,dc);
}

static void GetHighScoreCourses( vector<Course*> &vpCoursesOut )
{
	vpCoursesOut.clear();

	vector<Course*> vpCourses;
	SONGMAN->GetAllCourses( vpCourses, false );
	FOREACH_CONST( Course*, vpCourses, c )
	{
		// Don't count any course that has any entries that change over time.
		if( !(*c)->AllSongsAreFixed() )
			continue;

		vpCoursesOut.push_back( *c );
	}
}

float Profile::GetCoursesPossible( StepsType st, CourseDifficulty cd ) const
{
	int iTotalTrails = 0;

	vector<Course*> vpCourses;
	GetHighScoreCourses( vpCourses );
	FOREACH_CONST( Course*, vpCourses, c )
	{
		Trail* pTrail = (*c)->GetTrail(st,cd);
		if( pTrail == NULL )
			continue;

		iTotalTrails++;
	}
	
	return (float) iTotalTrails;
}
	
float Profile::GetCoursesActual( StepsType st, CourseDifficulty cd ) const
{
	float fTotalPercents = 0;
	
	vector<Course*> vpCourses;
	GetHighScoreCourses( vpCourses );
	FOREACH_CONST( Course*, vpCourses, c )
	{
		Trail *pTrail = (*c)->GetTrail( st, cd );
		if( pTrail == NULL )
			continue;

		const HighScoreList& hsl = GetCourseHighScoreList( *c, pTrail );
		fTotalPercents += hsl.GetTopScore().GetPercentDP();
	}

	return fTotalPercents;
}

float Profile::GetCoursesPercentComplete( StepsType st, CourseDifficulty cd ) const
{
	return GetCoursesActual(st,cd) / GetCoursesPossible(st,cd);
}

float Profile::GetSongsAndCoursesPercentCompleteAllDifficulties( StepsType st ) const
{
	float fActual = 0;
	float fPossible = 0;
	FOREACH_Difficulty( d )
	{
		fActual += GetSongsActual(st,d);
		fPossible += GetSongsPossible(st,d);
	}
	FOREACH_CourseDifficulty( d )
	{
		fActual += GetCoursesActual(st,d);
		fPossible += GetCoursesPossible(st,d);
	}
	return fActual / fPossible;
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
	FOREACHM_CONST( StepsID, HighScoresForASteps, hsSong->m_StepsHighScores, j )
	{
		const HighScoresForASteps &hsSteps = j->second;

		iTotalNumTimesPlayed += hsSteps.hsl.GetNumTimesPlayed();
	}
	return iTotalNumTimesPlayed;
}

/*
 * Get the profile default modifiers.  Return true if set, in which case sModifiersOut
 * will be set.  Return false if no modifier string is set, in which case the theme
 * defaults should be used.  Note that the null string means "no modifiers are active",
 * which is distinct from no modifier string being set at all.
 *
 * In practice, we get the default modifiers from the theme the first time a game
 * is played, and from the profile every time thereafter.
 */
bool Profile::GetDefaultModifiers( const Game* pGameType, RString &sModifiersOut ) const
{
	map<RString,RString>::const_iterator it;
	it = m_sDefaultModifiers.find( pGameType->m_szName );
	if( it == m_sDefaultModifiers.end() )
		return false;
	sModifiersOut = it->second;
	return true;
}

void Profile::SetDefaultModifiers( const Game* pGameType, const RString &sModifiers )
{
	if( sModifiers == "" )
		m_sDefaultModifiers.erase( pGameType->m_szName );
	else
		m_sDefaultModifiers[pGameType->m_szName] = sModifiers;
}

bool Profile::IsCodeUnlocked( RString sUnlockEntryID ) const
{
	return m_UnlockedEntryIDs.find( sUnlockEntryID ) != m_UnlockedEntryIDs.end();
}


Song *Profile::GetMostPopularSong() const
{
	int iMaxNumTimesPlayed = 0;
	SongID id;
	FOREACHM_CONST( SongID, HighScoresForASong, m_SongHighScores, i )
	{
		int iNumTimesPlayed = i->second.GetNumTimesPlayed();
		if( iNumTimesPlayed > iMaxNumTimesPlayed )
		{
			id = i->first;
			iMaxNumTimesPlayed = iNumTimesPlayed;
		}
	}

	return id.ToSong();
}

Course *Profile::GetMostPopularCourse() const
{
	int iMaxNumTimesPlayed = 0;
	CourseID id;
	FOREACHM_CONST( CourseID, HighScoresForACourse, m_CourseHighScores, i )
	{
		int iNumTimesPlayed = i->second.GetNumTimesPlayed();
		if( iNumTimesPlayed > iMaxNumTimesPlayed )
		{
			id = i->first;
			iMaxNumTimesPlayed = iNumTimesPlayed;
		}
	}

	return id.ToCourse();
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

	return hsSteps.hsl;
}

int Profile::GetStepsNumTimesPlayed( const Song* pSong, const Steps* pSteps ) const
{
	return GetStepsHighScoreList(pSong,pSteps).GetNumTimesPlayed();
}

DateTime Profile::GetSongLastPlayedDateTime( const Song* pSong ) const
{
	SongID id;
	id.FromSong( pSong );
	std::map<SongID,HighScoresForASong>::const_iterator iter = m_SongHighScores.find( id );

	// don't call this unless has been played once
	ASSERT( iter != m_SongHighScores.end() );
	ASSERT( !iter->second.m_StepsHighScores.empty() );

	DateTime dtLatest;	// starts out zeroed
	FOREACHM_CONST( StepsID, HighScoresForASteps, iter->second.m_StepsHighScores, i )
	{
		const HighScoreList &hsl = i->second.hsl;
		if( hsl.GetNumTimesPlayed() == 0 )
			continue;
		if( dtLatest < hsl.GetLastPlayed() )
			dtLatest = hsl.GetLastPlayed();
	}
	return dtLatest;
}

bool Profile::HasPassedSteps( const Song* pSong, const Steps* pSteps ) const
{
	const HighScoreList &hsl = GetStepsHighScoreList( pSong, pSteps );
	Grade grade = hsl.GetTopScore().GetGrade();
	switch( grade )
	{
	case Grade_Failed:
	case Grade_NoData:
		return false;
	default:
		return true;
	}
}

bool Profile::HasPassedAnyStepsInSong( const Song* pSong ) const
{
	FOREACH_CONST( Steps*, pSong->GetAllSteps(), steps )
	{
		if( HasPassedSteps( pSong, *steps ) )
			return true;
	}
	return false;
}

void Profile::IncrementStepsPlayCount( const Song* pSong, const Steps* pSteps )
{
	DateTime now = DateTime::GetNowDate();
	GetStepsHighScoreList(pSong,pSteps).IncrementPlayCount( now );
}

void Profile::GetGrades( const Song* pSong, StepsType st, int iCounts[NUM_Grade] ) const
{
	SongID songID;
	songID.FromSong( pSong );

	
	memset( iCounts, 0, sizeof(int)*NUM_Grade );
	const HighScoresForASong *hsSong = GetHighScoresForASong( songID );
	if( hsSong == NULL )
		return;

	FOREACH_Grade(g)
	{
		FOREACHM_CONST( StepsID, HighScoresForASteps, hsSong->m_StepsHighScores, it )
		{
			const StepsID &id = it->first;
			if( !id.MatchesStepsType(st) )
				continue;

			const HighScoresForASteps &hsSteps = it->second;
			if( hsSteps.hsl.GetTopScore().GetGrade() == g )
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

	return hsTrail.hsl;
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
	FOREACHM_CONST( TrailID, HighScoresForATrail, hsCourse->m_TrailHighScores, j )
	{
		const HighScoresForATrail &hsTrail = j->second;

		iTotalNumTimesPlayed += hsTrail.hsl.GetNumTimesPlayed();
	}
	return iTotalNumTimesPlayed;
}

DateTime Profile::GetCourseLastPlayedDateTime( const Course* pCourse ) const
{
	CourseID id;
	id.FromCourse( pCourse );
	std::map<CourseID,HighScoresForACourse>::const_iterator iter = m_CourseHighScores.find( id );

	// don't call this unless has been played once
	ASSERT( iter != m_CourseHighScores.end() );
	ASSERT( !iter->second.m_TrailHighScores.empty() );

	DateTime dtLatest;	// starts out zeroed
	FOREACHM_CONST( TrailID, HighScoresForATrail, iter->second.m_TrailHighScores, i )
	{
		const HighScoreList &hsl = i->second.hsl;
		if( hsl.GetNumTimesPlayed() == 0 )
			continue;
		if( dtLatest < hsl.GetLastPlayed() )
			dtLatest = hsl.GetLastPlayed();
	}
	return dtLatest;
}

void Profile::IncrementCoursePlayCount( const Course* pCourse, const Trail* pTrail )
{
	DateTime now = DateTime::GetNowDate();
	GetCourseHighScoreList(pCourse,pTrail).IncrementPlayCount( now );
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
		iNumTimesPlayed += m_CategoryHighScores[st][rc].GetNumTimesPlayed();
	return iNumTimesPlayed;
}

void Profile::IncrementCategoryPlayCount( StepsType st, RankingCategory rc )
{
	DateTime now = DateTime::GetNowDate();
	m_CategoryHighScores[st][rc].IncrementPlayCount( now );
}


//
// Loading and saving
//

#define WARN_PARSER	LOG->Warn("Error parsing file at %s:%d",__FILE__,__LINE__);
#define WARN_AND_RETURN { WARN_PARSER; return; }
#define WARN_AND_CONTINUE { WARN_PARSER; continue; }
#define WARN_AND_BREAK { WARN_PARSER; break; }
#define WARN_M(m)	LOG->Warn("Error parsing file at %s:%d: %s",__FILE__,__LINE__, (const char*) (m) );
#define WARN_AND_RETURN_M(m) { WARN_M(m); return; }
#define WARN_AND_CONTINUE_M(m) { WARN_M(m); continue; }
#define WARN_AND_BREAK_M(m) { WARN_M(m); break; }
#define LOAD_NODE(X)	{ \
	const XNode* X = xml->GetChild(#X); \
	if( X==NULL ) LOG->Warn("Failed to read section " #X); \
	else Load##X##FromNode(X); }

ProfileLoadResult Profile::LoadAllFromDir( RString sDir, bool bRequireSignature )
{
	CHECKPOINT;

	LOG->Trace( "Profile::LoadAllFromDir( %s )", sDir.c_str() );

	ASSERT( sDir.Right(1) == "/" );

	InitAll();

	// Not critical if this fails
	LoadEditableDataFromDir( sDir );
	
	// Check for the existance of stats.xml
	RString fn = sDir + STATS_XML;
	if( !IsAFile(fn) )
		return ProfileLoadResult_FailedNoProfile;

	//
	// Don't unreasonably large stats.xml files.
	//
	if( !IsMachine() )	// only check stats coming from the player
	{
		int iBytes = FILEMAN->GetFileSizeInBytes( fn );
		if( iBytes > MAX_PLAYER_STATS_XML_SIZE_BYTES )
		{
			LOG->Warn( "The file '%s' is unreasonably large.  It won't be loaded.", fn.c_str() );
			return ProfileLoadResult_FailedTampered;
		}
	}

	if( bRequireSignature )
	{ 
		RString sStatsXmlSigFile = fn+SIGNATURE_APPEND;
		RString sDontShareFile = sDir + DONT_SHARE_SIG;

		LOG->Trace( "Verifying don't share signature" );
		// verify the stats.xml signature with the "don't share" file
		if( !CryptManager::VerifyFileWithFile(sStatsXmlSigFile, sDontShareFile) )
		{
			LOG->Warn( "The don't share check for '%s' failed.  Data will be ignored.", sStatsXmlSigFile.c_str() );
			return ProfileLoadResult_FailedTampered;
		}
		LOG->Trace( "Done." );

		// verify stats.xml
		LOG->Trace( "Verifying stats.xml signature" );
		if( !CryptManager::VerifyFileWithFile(fn, sStatsXmlSigFile) )
		{
			LOG->Warn( "The signature check for '%s' failed.  Data will be ignored.", fn.c_str() );
			return ProfileLoadResult_FailedTampered;
		}
		LOG->Trace( "Done." );
	}

	LOG->Trace( "Loading %s", fn.c_str() );
	XNode xml;
	if( !XmlFileUtil::LoadFromFileShowErrors(xml, fn) )
		return ProfileLoadResult_FailedTampered;
	LOG->Trace( "Done." );

	return LoadStatsXmlFromNode( &xml );
}

ProfileLoadResult Profile::LoadStatsXmlFromNode( const XNode *xml, bool bIgnoreEditable )
{
	/* The placeholder stats.xml file has an <html> tag.  Don't load it, but don't
	 * warn about it. */
	if( xml->m_sName == "html" )
		return ProfileLoadResult_FailedNoProfile;

	if( xml->m_sName != "Stats" )
	{
		WARN_M( xml->m_sName );
		return ProfileLoadResult_FailedTampered;
	}

	/* These are loaded from Editable, so we usually want to ignore them
	 * here. */
	RString sName = m_sDisplayName;
	RString sCharacterID = m_sCharacterID;
	RString sLastUsedHighScoreName = m_sLastUsedHighScoreName;
	int iWeightPounds = m_iWeightPounds;

	LOAD_NODE( GeneralData );
	LOAD_NODE( SongScores );
	LOAD_NODE( CourseScores );
	LOAD_NODE( CategoryScores );
	LOAD_NODE( ScreenshotData );
	LOAD_NODE( CalorieData );
	LOAD_NODE( RecentSongScores );
	LOAD_NODE( RecentCourseScores );
		
	if( bIgnoreEditable )
	{
		m_sDisplayName = sName;
		m_sCharacterID = sCharacterID;
		m_sLastUsedHighScoreName = sLastUsedHighScoreName;
		m_iWeightPounds = iWeightPounds;
	}

	return ProfileLoadResult_Success;
}

bool Profile::SaveAllToDir( RString sDir, bool bSignData ) const
{
	m_sLastPlayedMachineGuid = PROFILEMAN->GetMachineProfile()->m_sGuid;
	m_LastPlayedDate = DateTime::GetNowDate();

	// Save editable.ini
	SaveEditableDataToDir( sDir );

	bool bSaved = SaveStatsXmlToDir( sDir, bSignData );
	
	SaveStatsWebPageToDir( sDir );

	// Empty directories if none exist.
	FILEMAN->CreateDir( sDir + EDIT_STEPS_SUBDIR );
	FILEMAN->CreateDir( sDir + EDIT_COURSES_SUBDIR );
	FILEMAN->CreateDir( sDir + SCREENSHOTS_SUBDIR );

	FlushDirCache();

	return bSaved;
}

XNode *Profile::SaveStatsXmlCreateNode() const
{
	XNode *xml = new XNode;

	xml->m_sName = "Stats";
	xml->AppendChild( SaveGeneralDataCreateNode() );
	xml->AppendChild( SaveSongScoresCreateNode() );
	xml->AppendChild( SaveCourseScoresCreateNode() );
	xml->AppendChild( SaveCategoryScoresCreateNode() );
	xml->AppendChild( SaveScreenshotDataCreateNode() );
	xml->AppendChild( SaveCalorieDataCreateNode() );
	xml->AppendChild( SaveRecentSongScoresCreateNode() );
	xml->AppendChild( SaveRecentCourseScoresCreateNode() );
	if( SHOW_COIN_DATA.GetValue() && IsMachine() )
		xml->AppendChild( SaveCoinDataCreateNode() );

	return xml;
}

bool Profile::SaveStatsXmlToDir( RString sDir, bool bSignData ) const
{
	XNode *xml = SaveStatsXmlCreateNode();

	// Save stats.xml
	RString fn = sDir + STATS_XML;

	DISP_OPT opts;
	opts.stylesheet = STATS_XSL;
	opts.write_tabs = false;
	bool bSaved = xml->SaveToFile( fn, opts );

	SAFE_DELETE( xml );
	
	// Update file cache, or else IsAFile in CryptManager won't see this new file.
	FILEMAN->FlushDirCache( sDir );
	
	if( bSaved && bSignData )
	{
		RString sStatsXmlSigFile = fn+SIGNATURE_APPEND;
		CryptManager::SignFileToFile(fn, sStatsXmlSigFile);

		// Update file cache, or else IsAFile in CryptManager won't see sStatsXmlSigFile.
		FILEMAN->FlushDirCache( sDir );

		// Save the "don't share" file
		RString sDontShareFile = sDir + DONT_SHARE_SIG;
		CryptManager::SignFileToFile(sStatsXmlSigFile, sDontShareFile);
	}

	return bSaved;
}

void Profile::SaveEditableDataToDir( RString sDir ) const
{
	IniFile ini;

	ini.SetValue( "Editable", "DisplayName",			m_sDisplayName );
	ini.SetValue( "Editable", "CharacterID",			m_sCharacterID );
	ini.SetValue( "Editable", "LastUsedHighScoreName",		m_sLastUsedHighScoreName );
	ini.SetValue( "Editable", "WeightPounds",			m_iWeightPounds );

	ini.WriteFile( sDir + EDITABLE_INI );
}

XNode* Profile::SaveGeneralDataCreateNode() const
{
	XNode* pGeneralDataNode = new XNode;
	pGeneralDataNode->m_sName = "GeneralData";

	// TRICKY: These are write-only elements that are normally never read again.
	// This data is required by other apps (like internet ranking), but is 
	// redundant to the game app.
	pGeneralDataNode->AppendChild( "DisplayName",			GetDisplayNameOrHighScoreName() );
	pGeneralDataNode->AppendChild( "CharacterID",			m_sCharacterID );
	pGeneralDataNode->AppendChild( "LastUsedHighScoreName",		m_sLastUsedHighScoreName );
	pGeneralDataNode->AppendChild( "WeightPounds",			m_iWeightPounds );
	pGeneralDataNode->AppendChild( "IsMachine",			IsMachine() );
	pGeneralDataNode->AppendChild( "IsWeightSet",			m_iWeightPounds != 0 );

	pGeneralDataNode->AppendChild( "Guid",				m_sGuid );
	pGeneralDataNode->AppendChild( "SortOrder",			SortOrderToString(m_SortOrder) );
	pGeneralDataNode->AppendChild( "LastDifficulty",		DifficultyToString(m_LastDifficulty) );
	pGeneralDataNode->AppendChild( "LastCourseDifficulty",		CourseDifficultyToString(m_LastCourseDifficulty) );
	pGeneralDataNode->AppendChild( m_lastSong.CreateNode() );
	pGeneralDataNode->AppendChild( m_lastCourse.CreateNode() );
	pGeneralDataNode->AppendChild( "TotalPlays",			m_iTotalPlays );
	pGeneralDataNode->AppendChild( "TotalPlaySeconds",		m_iTotalPlaySeconds );
	pGeneralDataNode->AppendChild( "TotalGameplaySeconds",		m_iTotalGameplaySeconds );
	pGeneralDataNode->AppendChild( "CurrentCombo",			m_iCurrentCombo );
	pGeneralDataNode->AppendChild( "TotalCaloriesBurned",		m_fTotalCaloriesBurned );
	pGeneralDataNode->AppendChild( "GoalType",			m_GoalType );
	pGeneralDataNode->AppendChild( "GoalCalories",			m_iGoalCalories );
	pGeneralDataNode->AppendChild( "GoalSeconds",			m_iGoalSeconds );
	pGeneralDataNode->AppendChild( "LastPlayedMachineGuid",		m_sLastPlayedMachineGuid );
	pGeneralDataNode->AppendChild( "LastPlayedDate",		m_LastPlayedDate );
	pGeneralDataNode->AppendChild( "TotalDancePoints",		m_iTotalDancePoints );
	pGeneralDataNode->AppendChild( "NumExtraStagesPassed",		m_iNumExtraStagesPassed );
	pGeneralDataNode->AppendChild( "NumExtraStagesFailed",		m_iNumExtraStagesFailed );
	pGeneralDataNode->AppendChild( "NumToasties",			m_iNumToasties );
	pGeneralDataNode->AppendChild( "TotalTapsAndHolds",		m_iTotalTapsAndHolds );
	pGeneralDataNode->AppendChild( "TotalJumps",			m_iTotalJumps );
	pGeneralDataNode->AppendChild( "TotalHolds",			m_iTotalHolds );
	pGeneralDataNode->AppendChild( "TotalRolls",			m_iTotalRolls );
	pGeneralDataNode->AppendChild( "TotalMines",			m_iTotalMines );
	pGeneralDataNode->AppendChild( "TotalHands",			m_iTotalHands );
	pGeneralDataNode->AppendChild( "Data",				m_SavedLuaData.Serialize() );

	// Keep declared variables in a very local scope so they aren't 
	// accidentally used where they're not intended.  There's a lot of
	// copying and pasting in this code.

	{
		XNode* pDefaultModifiers = pGeneralDataNode->AppendChild("DefaultModifiers");
		FOREACHM_CONST( RString, RString, m_sDefaultModifiers, it )
			pDefaultModifiers->AppendChild( it->first, it->second );
	}

	{
		XNode* pUnlocks = pGeneralDataNode->AppendChild("Unlocks");
		FOREACHS_CONST( RString, m_UnlockedEntryIDs, it )
			pUnlocks->AppendChild("UnlockEntry")->AppendAttr( "UnlockEntryID", it->c_str() );
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
		FOREACHM_CONST( StyleID, int, m_iNumSongsPlayedByStyle, iter )
		{
			const StyleID &s = iter->first;
			int iNumPlays = iter->second;

			XNode *pStyleNode = s.CreateNode();
			pStyleNode->SetValue( iNumPlays );

			pNumSongsPlayedByStyle->AppendChild( pStyleNode );
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

	pGeneralDataNode->AppendChild( "NumTotalSongsPlayed", m_iNumTotalSongsPlayed );

	{
		XNode* pNumStagesPassedByPlayMode = pGeneralDataNode->AppendChild("NumStagesPassedByPlayMode");
		FOREACH_PlayMode( pm )
		{
			/* Don't save unplayed PlayModes. */
			if( !m_iNumStagesPassedByPlayMode[pm] )
				continue;
			pNumStagesPassedByPlayMode->AppendChild( PlayModeToString(pm), m_iNumStagesPassedByPlayMode[pm] );
		}
	}

	{
		XNode* pNumStagesPassedByGrade = pGeneralDataNode->AppendChild("NumStagesPassedByGrade");
		FOREACH_Grade( g )
		{
			if( !m_iNumStagesPassedByGrade[g] )
				continue;
			pNumStagesPassedByGrade->AppendChild( GradeToString(g), m_iNumStagesPassedByGrade[g] );
		}
	}

	return pGeneralDataNode;
}

ProfileLoadResult Profile::LoadEditableDataFromDir( RString sDir )
{
	RString fn = sDir + EDITABLE_INI;

	//
	// Don't load unreasonably large editable.xml files.
	//
	int iBytes = FILEMAN->GetFileSizeInBytes( fn );
	if( iBytes > MAX_EDITABLE_INI_SIZE_BYTES )
	{
		LOG->Warn( "The file '%s' is unreasonably large.  It won't be loaded.", fn.c_str() );
		return ProfileLoadResult_FailedTampered;
	}

	if( !IsAFile(fn) )
		return ProfileLoadResult_FailedNoProfile;

	IniFile ini;
	ini.ReadFile( fn );

	ini.GetValue( "Editable", "DisplayName",			m_sDisplayName );
	ini.GetValue( "Editable", "CharacterID",			m_sCharacterID );
	ini.GetValue( "Editable", "LastUsedHighScoreName",		m_sLastUsedHighScoreName );
	ini.GetValue( "Editable", "WeightPounds",			m_iWeightPounds );

	// This is data that the user can change, so we have to validate it.
	wstring wstr = RStringToWstring(m_sDisplayName);
	if( wstr.size() > PROFILE_MAX_DISPLAY_NAME_LENGTH )
		wstr = wstr.substr(0, PROFILE_MAX_DISPLAY_NAME_LENGTH);
	m_sDisplayName = WStringToRString(wstr);
	// TODO: strip invalid chars?
	if( m_iWeightPounds != 0 )
		CLAMP( m_iWeightPounds, 20, 1000 );

	return ProfileLoadResult_Success;
}

void Profile::LoadGeneralDataFromNode( const XNode* pNode )
{
	ASSERT( pNode->m_sName == "GeneralData" );

	RString s;
	const XNode* pTemp;

	pNode->GetChildValue( "DisplayName",				m_sDisplayName );
	pNode->GetChildValue( "CharacterID",				m_sCharacterID );
	pNode->GetChildValue( "LastUsedHighScoreName",			m_sLastUsedHighScoreName );
	pNode->GetChildValue( "WeightPounds",				m_iWeightPounds );
	pNode->GetChildValue( "Guid",					m_sGuid );
	pNode->GetChildValue( "SortOrder",				s );	m_SortOrder = StringToSortOrder( s );
	pNode->GetChildValue( "LastDifficulty",				s );	m_LastDifficulty = StringToDifficulty( s );
	pNode->GetChildValue( "LastCourseDifficulty",			s );	m_LastCourseDifficulty = StringToCourseDifficulty( s );
	pTemp = pNode->GetChild( "Song" );				if( pTemp ) m_lastSong.LoadFromNode( pTemp );
	pTemp = pNode->GetChild( "Course" );				if( pTemp ) m_lastCourse.LoadFromNode( pTemp );
	pNode->GetChildValue( "TotalPlays",				m_iTotalPlays );
	pNode->GetChildValue( "TotalPlaySeconds",			m_iTotalPlaySeconds );
	pNode->GetChildValue( "TotalGameplaySeconds",			m_iTotalGameplaySeconds );
	pNode->GetChildValue( "CurrentCombo",				m_iCurrentCombo );
	pNode->GetChildValue( "TotalCaloriesBurned",			m_fTotalCaloriesBurned );
	pNode->GetChildValue( "GoalType",				(int&)m_GoalType );
	pNode->GetChildValue( "GoalCalories",				m_iGoalCalories );
	pNode->GetChildValue( "GoalSeconds",				m_iGoalSeconds );
	pNode->GetChildValue( "LastPlayedMachineGuid",			m_sLastPlayedMachineGuid );
	pNode->GetChildValue( "LastPlayedDate",				m_LastPlayedDate );
	pNode->GetChildValue( "TotalDancePoints",			m_iTotalDancePoints );
	pNode->GetChildValue( "NumExtraStagesPassed",			m_iNumExtraStagesPassed );
	pNode->GetChildValue( "NumExtraStagesFailed",			m_iNumExtraStagesFailed );
	pNode->GetChildValue( "NumToasties",				m_iNumToasties );
	pNode->GetChildValue( "TotalTapsAndHolds",			m_iTotalTapsAndHolds );
	pNode->GetChildValue( "TotalJumps",				m_iTotalJumps );
	pNode->GetChildValue( "TotalHolds",				m_iTotalHolds );
	pNode->GetChildValue( "TotalRolls",				m_iTotalRolls );
	pNode->GetChildValue( "TotalMines",				m_iTotalMines );
	pNode->GetChildValue( "TotalHands",				m_iTotalHands );

	{
		RString sData;
		if( pNode->GetChildValue( "Data", sData ) )
		{
			m_SavedLuaData.LoadFromString( sData );
			if( m_SavedLuaData.GetLuaType() != LUA_TTABLE )
			{
				LOG->Warn( "Profile data did not evaluate to a table" );
				Lua *L = LUA->Get();
				lua_newtable( L );
				m_SavedLuaData.SetFromStack( L );
				LUA->Release( L );
			}
		}
	}

	{
		const XNode* pDefaultModifiers = pNode->GetChild("DefaultModifiers");
		if( pDefaultModifiers )
		{
			FOREACH_CONST_Child( pDefaultModifiers, game_type )
			{
				m_sDefaultModifiers[game_type->m_sName] = game_type->m_sValue;
			}
		}
	}

	{
		const XNode* pUnlocks = pNode->GetChild("Unlocks");
		if( pUnlocks )
		{
			FOREACH_CONST_Child( pUnlocks, unlock )
			{
				RString sUnlockEntryID;
				if( unlock->GetAttrValue("UnlockEntryID",sUnlockEntryID) )
					m_UnlockedEntryIDs.insert( sUnlockEntryID );
			}
		}
	}

	{
		const XNode* pNumSongsPlayedByPlayMode = pNode->GetChild("NumSongsPlayedByPlayMode");
		if( pNumSongsPlayedByPlayMode )
			FOREACH_PlayMode( pm )
				pNumSongsPlayedByPlayMode->GetChildValue( PlayModeToString(pm), m_iNumSongsPlayedByPlayMode[pm] );
	}

	{
		const XNode* pNumSongsPlayedByStyle = pNode->GetChild("NumSongsPlayedByStyle");
		if( pNumSongsPlayedByStyle )
		{
			FOREACH_CONST_Child( pNumSongsPlayedByStyle, style )
			{
				if( style->m_sName != "Style" )
					continue;

				StyleID s;
				s.LoadFromNode( style );

				if( !s.IsValid() )
					WARN_AND_CONTINUE;

				style->GetValue( m_iNumSongsPlayedByStyle[s] );
			}
		}
	}

	{
		const XNode* pNumSongsPlayedByDifficulty = pNode->GetChild("NumSongsPlayedByDifficulty");
		if( pNumSongsPlayedByDifficulty )
			FOREACH_Difficulty( dc )
				pNumSongsPlayedByDifficulty->GetChildValue( DifficultyToString(dc), m_iNumSongsPlayedByDifficulty[dc] );
	}

	{
		const XNode* pNumSongsPlayedByMeter = pNode->GetChild("NumSongsPlayedByMeter");
		if( pNumSongsPlayedByMeter )
			for( int i=0; i<MAX_METER+1; i++ )
				pNumSongsPlayedByMeter->GetChildValue( ssprintf("Meter%d",i), m_iNumSongsPlayedByMeter[i] );
	}

	pNode->GetChildValue("NumTotalSongsPlayed", m_iNumTotalSongsPlayed );

	{
		const XNode* pNumStagesPassedByGrade = pNode->GetChild("NumStagesPassedByGrade");
		if( pNumStagesPassedByGrade )
			FOREACH_Grade( g )
				pNumStagesPassedByGrade->GetChildValue( GradeToString(g), m_iNumStagesPassedByGrade[g] );
	}

	{
		const XNode* pNumStagesPassedByPlayMode = pNode->GetChild("NumStagesPassedByPlayMode");
		if( pNumStagesPassedByPlayMode )
			FOREACH_PlayMode( pm )
				pNumStagesPassedByPlayMode->GetChildValue( PlayModeToString(pm), m_iNumStagesPassedByPlayMode[pm] );
	
	}

}

void Profile::AddStepTotals( int iTotalTapsAndHolds, int iTotalJumps, int iTotalHolds, int iTotalRolls, int iTotalMines, int iTotalHands, float fCaloriesBurned )
{
	m_iTotalTapsAndHolds += iTotalTapsAndHolds;
	m_iTotalJumps += iTotalJumps;
	m_iTotalHolds += iTotalHolds;
	m_iTotalRolls += iTotalRolls;
	m_iTotalMines += iTotalMines;
	m_iTotalHands += iTotalHands;

	m_fTotalCaloriesBurned += fCaloriesBurned;

	DateTime date = DateTime::GetNowDate();
	m_mapDayToCaloriesBurned[date].fCals += fCaloriesBurned;
}

XNode* Profile::SaveSongScoresCreateNode() const
{
	CHECKPOINT;

	const Profile* pProfile = this;
	ASSERT( pProfile );

	XNode* pNode = new XNode;
	pNode->m_sName = "SongScores";

	FOREACHM_CONST( SongID, HighScoresForASong, m_SongHighScores, i )
	{	
		const SongID &songID = i->first;
		const HighScoresForASong &hsSong = i->second;

		// skip songs that have never been played
		if( pProfile->GetSongNumTimesPlayed(songID) == 0 )
			continue;

		XNode* pSongNode = pNode->AppendChild( songID.CreateNode() );

		int jCheck2 = hsSong.m_StepsHighScores.size();
		int jCheck1 = 0;
		FOREACHM_CONST( StepsID, HighScoresForASteps, hsSong.m_StepsHighScores, j )
		{	
			jCheck1++;
			ASSERT( jCheck1 <= jCheck2 );
			const StepsID &stepsID = j->first;
			const HighScoresForASteps &hsSteps = j->second;

			const HighScoreList &hsl = hsSteps.hsl;

			// skip steps that have never been played
			if( hsl.GetNumTimesPlayed() == 0 )
				continue;

			XNode* pStepsNode = pSongNode->AppendChild( stepsID.CreateNode() );

			pStepsNode->AppendChild( hsl.CreateNode() );
		}
	}
	
	return pNode;
}

void Profile::LoadSongScoresFromNode( const XNode* pSongScores )
{
	CHECKPOINT;

	ASSERT( pSongScores->m_sName == "SongScores" );

	FOREACH_CONST_Child( pSongScores, pSong )
	{
		if( pSong->m_sName != "Song" )
			continue;

		SongID songID;
		songID.LoadFromNode( pSong );
		if( !songID.IsValid() )
			WARN_AND_CONTINUE;

		FOREACH_CONST_Child( pSong, pSteps )
		{
			if( pSteps->m_sName != "Steps" )
				continue;

			StepsID stepsID;
			stepsID.LoadFromNode( pSteps );
			if( !stepsID.IsValid() )
				WARN_AND_CONTINUE;

			const XNode *pHighScoreListNode = pSteps->GetChild("HighScoreList");
			if( pHighScoreListNode == NULL )
				WARN_AND_CONTINUE;
			
			HighScoreList &hsl = m_SongHighScores[songID].m_StepsHighScores[stepsID].hsl;
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
	pNode->m_sName = "CourseScores";

	
	FOREACHM_CONST( CourseID, HighScoresForACourse, m_CourseHighScores, i )
	{
		const CourseID &courseID = i->first;
		const HighScoresForACourse &hsCourse = i->second;

		// skip courses that have never been played
		if( pProfile->GetCourseNumTimesPlayed(courseID) == 0 )
			continue;

		XNode* pCourseNode = pNode->AppendChild( courseID.CreateNode() );

		FOREACHM_CONST( TrailID, HighScoresForATrail, hsCourse.m_TrailHighScores, j )
		{
			const TrailID &trailID = j->first;
			const HighScoresForATrail &hsTrail = j->second;

			const HighScoreList &hsl = hsTrail.hsl;

			// skip steps that have never been played
			if( hsl.GetNumTimesPlayed() == 0 )
				continue;

			XNode* pTrailNode = pCourseNode->AppendChild( trailID.CreateNode() );

			pTrailNode->AppendChild( hsl.CreateNode() );
		}
	}

	return pNode;
}

void Profile::LoadCourseScoresFromNode( const XNode* pCourseScores )
{
	CHECKPOINT;

	ASSERT( pCourseScores->m_sName == "CourseScores" );

	vector<Course*> vpAllCourses;
	SONGMAN->GetAllCourses( vpAllCourses, true );

	FOREACH_CONST_Child( pCourseScores, pCourse )
	{
		if( pCourse->m_sName != "Course" )
			continue;

		CourseID courseID;
		courseID.LoadFromNode( pCourse );
		if( !courseID.IsValid() )
			WARN_AND_CONTINUE;
		

		// Backward compatability hack to fix importing scores of old style 
		// courses that weren't in group folder but have now been moved into
		// a group folder: 
		// If the courseID doesn't resolve, then take the file name part of sPath
		// and search for matches of just the file name.
		{
			Course *pC = courseID.ToCourse();
			if( pC == NULL )
			{
				RString sDir, sFName, sExt;
				splitpath( courseID.sPath, sDir, sFName, sExt );
				RString sFullFileName = sFName + sExt;

				FOREACH_CONST( Course*, vpAllCourses, c )
				{
					RString sOther = (*c)->m_sPath.Right(sFullFileName.size());

					if( sFullFileName.CompareNoCase(sOther) == 0 )
					{
						pC = *c;
						courseID.FromCourse( pC );
						break;
					}
				}
			}
		}


		FOREACH_CONST_Child( pCourse, pTrail )
		{
			if( pTrail->m_sName != "Trail" )
				continue;
			
			TrailID trailID;
			trailID.LoadFromNode( pTrail );
			if( !trailID.IsValid() )
				WARN_AND_CONTINUE;

			const XNode *pHighScoreListNode = pTrail->GetChild("HighScoreList");
			if( pHighScoreListNode == NULL )
				WARN_AND_CONTINUE;
			
			HighScoreList &hsl = m_CourseHighScores[courseID].m_TrailHighScores[trailID].hsl;
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
	pNode->m_sName = "CategoryScores";

	FOREACH_StepsType( st )
	{
		// skip steps types that have never been played
		if( pProfile->GetCategoryNumTimesPlayed( st ) == 0 )
			continue;

		XNode* pStepsTypeNode = pNode->AppendChild( "StepsType" );
		pStepsTypeNode->AppendAttr( "Type", GameManager::StepsTypeToString(st) );

		FOREACH_RankingCategory( rc )
		{
			// skip steps types/categories that have never been played
			if( pProfile->GetCategoryHighScoreList(st,rc).GetNumTimesPlayed() == 0 )
				continue;

			XNode* pRankingCategoryNode = pStepsTypeNode->AppendChild( "RankingCategory" );
			pRankingCategoryNode->AppendAttr( "Type", RankingCategoryToString(rc) );

			const HighScoreList &hsl = pProfile->GetCategoryHighScoreList( (StepsType)st, (RankingCategory)rc );

			pRankingCategoryNode->AppendChild( hsl.CreateNode() );
		}
	}

	return pNode;
}

void Profile::LoadCategoryScoresFromNode( const XNode* pCategoryScores )
{
	CHECKPOINT;

	ASSERT( pCategoryScores->m_sName == "CategoryScores" );

	FOREACH_CONST_Child( pCategoryScores, pStepsType )
	{
		if( pStepsType->m_sName != "StepsType" )
			continue;

		RString str;
		if( !pStepsType->GetAttrValue( "Type", str ) )
			WARN_AND_CONTINUE;
		StepsType st = GameManager::StringToStepsType( str );
		if( st == STEPS_TYPE_INVALID )
			WARN_AND_CONTINUE_M( str );

		FOREACH_CONST_Child( pStepsType, pRadarCategory )
		{
			if( pRadarCategory->m_sName != "RankingCategory" )
				continue;

			if( !pRadarCategory->GetAttrValue( "Type", str ) )
				WARN_AND_CONTINUE;
			RankingCategory rc = StringToRankingCategory( str );
			if( rc == RANKING_INVALID )
				WARN_AND_CONTINUE_M( str );

			const XNode *pHighScoreListNode = pRadarCategory->GetChild("HighScoreList");
			if( pHighScoreListNode == NULL )
				WARN_AND_CONTINUE;
			
			HighScoreList &hsl = this->GetCategoryHighScoreList( st, rc );
			hsl.LoadFromNode( pHighScoreListNode );
		}
	}
}

void Profile::SaveStatsWebPageToDir( RString sDir ) const
{
	ASSERT( PROFILEMAN );

	FileCopy( THEME->GetPathO("Profile",STATS_XSL), sDir+STATS_XSL );
	FileCopy( THEME->GetPathO("Profile",CATALOG_XSL), sDir+CATALOG_XSL );
	FileCopy( THEME->GetPathO("Profile",COMMON_XSL), sDir+COMMON_XSL );
	FileCopy( CATALOG_XML_FILE, sDir+CATALOG_XML );
}

void Profile::SaveMachinePublicKeyToDir( RString sDir ) const
{
	if( PREFSMAN->m_bSignProfileData && IsAFile(CRYPTMAN->GetPublicKeyFileName()) )
		FileCopy( CRYPTMAN->GetPublicKeyFileName(), sDir+PUBLIC_KEY_FILE );
}

void Profile::AddScreenshot( const Screenshot &screenshot )
{
	m_vScreenshots.push_back( screenshot );
}

void Profile::LoadScreenshotDataFromNode( const XNode* pScreenshotData )
{
	CHECKPOINT;

	ASSERT( pScreenshotData->m_sName == "ScreenshotData" );
	FOREACH_CONST_Child( pScreenshotData, pScreenshot )
	{
		if( pScreenshot->m_sName != "Screenshot" )
			WARN_AND_CONTINUE_M( pScreenshot->m_sName );

		Screenshot ss;
		ss.LoadFromNode( pScreenshot );

		m_vScreenshots.push_back( ss );
	}	
}

XNode* Profile::SaveScreenshotDataCreateNode() const
{
	CHECKPOINT;

	const Profile* pProfile = this;
	ASSERT( pProfile );

	XNode* pNode = new XNode;
	pNode->m_sName = "ScreenshotData";

	FOREACH_CONST( Screenshot, m_vScreenshots, ss )
	{
		pNode->AppendChild( ss->CreateNode() );
	}

	return pNode;
}

void Profile::LoadCalorieDataFromNode( const XNode* pCalorieData )
{
	CHECKPOINT;

	ASSERT( pCalorieData->m_sName == "CalorieData" );
	FOREACH_CONST_Child( pCalorieData, pCaloriesBurned )
	{
		if( pCaloriesBurned->m_sName != "CaloriesBurned" )
			WARN_AND_CONTINUE_M( pCaloriesBurned->m_sName );

		RString sDate;
		if( !pCaloriesBurned->GetAttrValue("Date",sDate) )
			WARN_AND_CONTINUE;
		DateTime date;
		if( !date.FromString(sDate) )
			WARN_AND_CONTINUE_M( sDate );

		float fCaloriesBurned = 0;

		pCaloriesBurned->GetValue(fCaloriesBurned);

		m_mapDayToCaloriesBurned[date].fCals = fCaloriesBurned;
	}	
}

XNode* Profile::SaveCalorieDataCreateNode() const
{
	CHECKPOINT;

	const Profile* pProfile = this;
	ASSERT( pProfile );

	XNode* pNode = new XNode;
	pNode->m_sName = "CalorieData";

	FOREACHM_CONST( DateTime, Calories, m_mapDayToCaloriesBurned, i )
	{
		XNode* pCaloriesBurned = pNode->AppendChild( "CaloriesBurned", i->second.fCals );

		pCaloriesBurned->AppendAttr( "Date", i->first.GetString() );
	}

	return pNode;
}

float Profile::GetCaloriesBurnedForDay( DateTime day ) const
{
	day.StripTime();
	map<DateTime,Calories>::const_iterator i = m_mapDayToCaloriesBurned.find( day );
	if( i == m_mapDayToCaloriesBurned.end() )
		return 0;
	else
		return i->second.fCals;
}

XNode* Profile::HighScoreForASongAndSteps::CreateNode() const
{
	XNode* pNode = new XNode;
	pNode->m_sName = "HighScoreForASongAndSteps";

	pNode->AppendChild( songID.CreateNode() );
	pNode->AppendChild( stepsID.CreateNode() );
	pNode->AppendChild( hs.CreateNode() );

	return pNode;
}

void Profile::HighScoreForASongAndSteps::LoadFromNode( const XNode* pNode )
{
	Unset();

	ASSERT( pNode->m_sName == "HighScoreForASongAndSteps" );
	const XNode* p;
	if( (p = pNode->GetChild("Song")) )
		songID.LoadFromNode( p );
	if( (p = pNode->GetChild("Steps")) )
		stepsID.LoadFromNode( p );
	if( (p = pNode->GetChild("HighScore")) )
		hs.LoadFromNode( p );
}

void Profile::LoadRecentSongScoresFromNode( const XNode* pRecentSongScores )
{
	CHECKPOINT;

	ASSERT( pRecentSongScores->m_sName == "RecentSongScores" );
	FOREACH_CONST_Child( pRecentSongScores, p )
	{
		if( p->m_sName == "HighScoreForASongAndSteps" )
		{
			HighScoreForASongAndSteps h;
			h.LoadFromNode( p );

			m_vRecentStepsScores.push_back( h );
		}
		else
		{
			WARN_AND_CONTINUE_M( p->m_sName );
		}
	}	
}

XNode* Profile::SaveRecentSongScoresCreateNode() const
{
	CHECKPOINT;

	const Profile* pProfile = this;
	ASSERT( pProfile );

	XNode* pNode = new XNode;
	pNode->m_sName = "RecentSongScores";

	FOREACHD_CONST( HighScoreForASongAndSteps, m_vRecentStepsScores, i )
		pNode->AppendChild( i->CreateNode() );

	return pNode;
}

void Profile::AddStepsRecentScore( const Song* pSong, const Steps* pSteps, HighScore hs )
{
	HighScoreForASongAndSteps h;
	h.songID.FromSong( pSong );
	h.stepsID.FromSteps( pSteps );
	h.hs = hs;
	m_vRecentStepsScores.push_back( h );

	int iMaxRecentScoresToSave = IsMachine() ? PREFSMAN->m_iMaxRecentScoresForMachine : PREFSMAN->m_iMaxRecentScoresForPlayer;
	int iNumToErase = m_vRecentStepsScores.size() - iMaxRecentScoresToSave;
	if( iNumToErase > 0 )
		m_vRecentStepsScores.erase( m_vRecentStepsScores.begin(), m_vRecentStepsScores.begin() +  iNumToErase );
}


XNode* Profile::HighScoreForACourseAndTrail::CreateNode() const
{
	XNode* pNode = new XNode;
	pNode->m_sName = "HighScoreForACourseAndTrail";

	pNode->AppendChild( courseID.CreateNode() );
	pNode->AppendChild( trailID.CreateNode() );
	pNode->AppendChild( hs.CreateNode() );

	return pNode;
}

void Profile::HighScoreForACourseAndTrail::LoadFromNode( const XNode* pNode )
{
	Unset();

	ASSERT( pNode->m_sName == "HighScoreForACourseAndTrail" );
	const XNode* p;
	if( (p = pNode->GetChild("Course")) )
		courseID.LoadFromNode( p );
	if( (p = pNode->GetChild("Trail")) )
		trailID.LoadFromNode( p );
	if( (p = pNode->GetChild("HighScore")) )
		hs.LoadFromNode( p );
}

void Profile::LoadRecentCourseScoresFromNode( const XNode* pRecentCourseScores )
{
	CHECKPOINT;

	ASSERT( pRecentCourseScores->m_sName == "RecentCourseScores" );
	FOREACH_CONST_Child( pRecentCourseScores, p )
	{
		if( p->m_sName == "HighScoreForACourseAndTrail" )
		{
			HighScoreForACourseAndTrail h;
			h.LoadFromNode( p );

			m_vRecentCourseScores.push_back( h );
		}
		else
		{
			WARN_AND_CONTINUE_M( p->m_sName );
		}
	}	
}

XNode* Profile::SaveRecentCourseScoresCreateNode() const
{
	CHECKPOINT;

	const Profile* pProfile = this;
	ASSERT( pProfile );

	XNode* pNode = new XNode;
	pNode->m_sName = "RecentCourseScores";

	FOREACHD_CONST( HighScoreForACourseAndTrail, m_vRecentCourseScores, i )
		pNode->AppendChild( i->CreateNode() );

	return pNode;
}

void Profile::AddCourseRecentScore( const Course* pCourse, const Trail* pTrail, HighScore hs )
{
	HighScoreForACourseAndTrail h;
	h.courseID.FromCourse( pCourse );
	h.trailID.FromTrail( pTrail );
	h.hs = hs;
	m_vRecentCourseScores.push_back( h );
	
	int iMaxRecentScoresToSave = IsMachine() ? PREFSMAN->m_iMaxRecentScoresForMachine : PREFSMAN->m_iMaxRecentScoresForPlayer;
	int iNumToErase = m_vRecentCourseScores.size() - iMaxRecentScoresToSave;
	if( iNumToErase > 0 )
		m_vRecentCourseScores.erase( m_vRecentCourseScores.begin(), m_vRecentCourseScores.begin() +  iNumToErase );
}

StepsType Profile::GetLastPlayedStepsType() const
{
	if( m_vRecentStepsScores.empty() )
		return STEPS_TYPE_INVALID;
	const HighScoreForASongAndSteps &h = m_vRecentStepsScores.back();
	return h.stepsID.GetStepsType();
}

const Profile::HighScoresForASong *Profile::GetHighScoresForASong( const SongID& songID ) const
{
	map<SongID,HighScoresForASong>::const_iterator it;
	it = m_SongHighScores.find( songID );
	if( it == m_SongHighScores.end() )
		return NULL;
	return &it->second;
}

const Profile::HighScoresForACourse *Profile::GetHighScoresForACourse( const CourseID& courseID ) const
{
	map<CourseID,HighScoresForACourse>::const_iterator it;
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


XNode* Profile::SaveCoinDataCreateNode() const
{
	CHECKPOINT;

	const Profile* pProfile = this;
	ASSERT( pProfile );

	XNode* pNode = new XNode;
	pNode->m_sName = "CoinData";

	{
		int coins[NUM_LAST_DAYS];
		BOOKKEEPER->GetCoinsLastDays( coins );
		XNode* p = pNode->AppendChild( "LastDays" );
		for( int i=0; i<NUM_LAST_DAYS; i++ )
			p->AppendChild( LastDayToString(i), coins[i] );
	}
	{
		int coins[NUM_LAST_WEEKS];
		BOOKKEEPER->GetCoinsLastWeeks( coins );
		XNode* p = pNode->AppendChild( "LastWeeks" );
		for( int i=0; i<NUM_LAST_WEEKS; i++ )
			p->AppendChild( LastWeekToString(i), coins[i] );
	}
	{
		int coins[DAYS_IN_WEEK];
		BOOKKEEPER->GetCoinsByDayOfWeek( coins );
		XNode* p = pNode->AppendChild( "DayOfWeek" );
		for( int i=0; i<DAYS_IN_WEEK; i++ )
			p->AppendChild( DayOfWeekToString(i), coins[i] );
	}
	{
		int coins[HOURS_IN_DAY];
		BOOKKEEPER->GetCoinsByHour( coins );
		XNode* p = pNode->AppendChild( "Hour" );
		for( int i=0; i<HOURS_IN_DAY; i++ )
			p->AppendChild( HourInDayToString(i), coins[i] );
	}

	return pNode;
}

void Profile::MoveBackupToDir( RString sFromDir, RString sToDir )
{
	FILEMAN->Move( sFromDir+EDITABLE_INI,				sToDir+EDITABLE_INI );
	FILEMAN->Move( sFromDir+STATS_XML,					sToDir+STATS_XML );
	FILEMAN->Move( sFromDir+STATS_XML+SIGNATURE_APPEND,	sToDir+STATS_XML+SIGNATURE_APPEND );
	FILEMAN->Move( sFromDir+DONT_SHARE_SIG,				sToDir+DONT_SHARE_SIG );
}

// lua start
#include "LuaBinding.h"

class LunaProfile: public Luna<Profile>
{
public:
	LunaProfile() { LUA->Register( Register ); }

	static int GetDisplayName( T* p, lua_State *L )			{ lua_pushstring(L, p->m_sDisplayName ); return 1; }
	static int GetCharacter( T* p, lua_State *L )			{ p->GetCharacter()->PushSelf(L); return 1; }
	static int GetWeightPounds( T* p, lua_State *L )		{ lua_pushnumber(L, p->m_iWeightPounds ); return 1; }
	static int SetWeightPounds( T* p, lua_State *L )		{ p->m_iWeightPounds = IArg(1); return 0; }
	static int GetGoalType( T* p, lua_State *L )			{ lua_pushnumber(L, p->m_GoalType ); return 1; }
	static int SetGoalType( T* p, lua_State *L )			{ p->m_GoalType = (GoalType)IArg(1); return 0; }
	static int GetGoalCalories( T* p, lua_State *L )		{ lua_pushnumber(L, p->m_iGoalCalories ); return 1; }
	static int SetGoalCalories( T* p, lua_State *L )		{ p->m_iGoalCalories = IArg(1); return 0; }
	static int GetGoalSeconds( T* p, lua_State *L )			{ lua_pushnumber(L, p->m_iGoalSeconds ); return 1; }
	static int SetGoalSeconds( T* p, lua_State *L )			{ p->m_iGoalSeconds = IArg(1); return 0; }
	static int GetCaloriesBurnedToday( T* p, lua_State *L )	{ lua_pushnumber(L, p->GetCaloriesBurnedToday() ); return 1; }
	static int GetSaved( T* p, lua_State *L )			{ p->m_SavedLuaData.PushSelf(L); return 1; }
	static int GetTotalNumSongsPlayed( T* p, lua_State *L )	{ lua_pushnumber(L, p->m_iNumTotalSongsPlayed ); return 1; }
	static int IsCodeUnlocked( T* p, lua_State *L )			{ lua_pushboolean(L, p->IsCodeUnlocked(SArg(1)) ); return 1; }
	static int GetSongsActual( T* p, lua_State *L )			{ lua_pushnumber(L, p->GetSongsActual((StepsType)IArg(1),(Difficulty)IArg(2)) ); return 1; }
	static int GetCoursesActual( T* p, lua_State *L )		{ lua_pushnumber(L, p->GetCoursesActual((StepsType)IArg(1),(CourseDifficulty)IArg(2)) ); return 1; }
	static int GetSongsPossible( T* p, lua_State *L )		{ lua_pushnumber(L, p->GetSongsPossible((StepsType)IArg(1),(Difficulty)IArg(2)) ); return 1; }
	static int GetCoursesPossible( T* p, lua_State *L )		{ lua_pushnumber(L, p->GetCoursesPossible((StepsType)IArg(1),(CourseDifficulty)IArg(2)) ); return 1; }
	static int GetSongsPercentComplete( T* p, lua_State *L )	{ lua_pushnumber(L, p->GetSongsPercentComplete((StepsType)IArg(1),(Difficulty)IArg(2)) ); return 1; }
	static int GetCoursesPercentComplete( T* p, lua_State *L )	{ lua_pushnumber(L, p->GetCoursesPercentComplete((StepsType)IArg(1),(CourseDifficulty)IArg(2)) ); return 1; }
	static int GetTotalStepsWithTopGrade( T* p, lua_State *L )	{ lua_pushnumber(L, p->GetTotalStepsWithTopGrade((StepsType)IArg(1),(Difficulty)IArg(2),(Grade)IArg(3)) ); return 1; }
	static int GetTotalTrailsWithTopGrade( T* p, lua_State *L )	{ lua_pushnumber(L, p->GetTotalTrailsWithTopGrade((StepsType)IArg(1),(CourseDifficulty)IArg(2),(Grade)IArg(3)) ); return 1; }
	static int GetNumTotalSongsPlayed( T* p, lua_State *L )		{ lua_pushnumber(L, p->m_iNumTotalSongsPlayed ); return 1; }
	static int GetLastPlayedStepsType( T* p, lua_State *L )		{ lua_pushnumber(L, p->GetLastPlayedStepsType() ); return 1; }
	static int GetSongsAndCoursesPercentCompleteAllDifficulties( T* p, lua_State *L )		{ lua_pushnumber(L, p->GetSongsAndCoursesPercentCompleteAllDifficulties((StepsType)IArg(1)) ); return 1; }
	static int GetDisplayTotalCaloriesBurned( T* p, lua_State *L )	{ lua_pushstring(L, p->GetDisplayTotalCaloriesBurned() ); return 1; }
	static int GetMostPopularSong( T* p, lua_State *L )
	{
		Song *p2 = p->GetMostPopularSong();
		if( p2 )
			p2->PushSelf(L);
		else
			lua_pushnil( L );
		return 1;
	}
	static int GetMostPopularCourse( T* p, lua_State *L )
	{
		Course *p2 = p->GetMostPopularCourse();
		if( p2 )
			p2->PushSelf(L);
		else
			lua_pushnil( L );
		return 1;
	}
	static int GetSongNumTimesPlayed( T* p, lua_State *L )
	{
		ASSERT( !lua_isnil(L,1) );
		Song *pS = Luna<Song>::check(L,1);
		lua_pushnumber( L, p->GetSongNumTimesPlayed(pS) );
		return 1;
	}
	static int HasPassedAnyStepsInSong( T* p, lua_State *L )
	{
		ASSERT( !lua_isnil(L,1) );
		Song *pS = Luna<Song>::check(L,1);
		lua_pushboolean( L, p->HasPassedAnyStepsInSong(pS) );
		return 1;
	}

	static void Register(lua_State *L)
	{
		ADD_METHOD( GetDisplayName );
		ADD_METHOD( GetCharacter );
		ADD_METHOD( GetWeightPounds );
		ADD_METHOD( SetWeightPounds );
		ADD_METHOD( GetGoalType );
		ADD_METHOD( SetGoalType );
		ADD_METHOD( GetGoalCalories );
		ADD_METHOD( SetGoalCalories );
		ADD_METHOD( GetGoalSeconds );
		ADD_METHOD( SetGoalSeconds );
		ADD_METHOD( GetCaloriesBurnedToday );
		ADD_METHOD( GetSaved );
		ADD_METHOD( GetTotalNumSongsPlayed );
		ADD_METHOD( IsCodeUnlocked );
		ADD_METHOD( GetSongsActual );
		ADD_METHOD( GetCoursesActual );
		ADD_METHOD( GetSongsPossible );
		ADD_METHOD( GetCoursesPossible );
		ADD_METHOD( GetSongsPercentComplete );
		ADD_METHOD( GetCoursesPercentComplete );
		ADD_METHOD( GetTotalStepsWithTopGrade );
		ADD_METHOD( GetTotalTrailsWithTopGrade );
		ADD_METHOD( GetNumTotalSongsPlayed );
		ADD_METHOD( GetLastPlayedStepsType );
		ADD_METHOD( GetSongsAndCoursesPercentCompleteAllDifficulties );
		ADD_METHOD( GetDisplayTotalCaloriesBurned );
		ADD_METHOD( GetMostPopularSong );
		ADD_METHOD( GetMostPopularCourse );
		ADD_METHOD( GetSongNumTimesPlayed );
		ADD_METHOD( HasPassedAnyStepsInSong );

		Luna<T>::Register( L );
	}
};

LUA_REGISTER_CLASS( Profile )
// lua end


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
