#include "global.h"
#include "ProfileManager.h"
#include "Profile.h"
#include "RageUtil.h"
#include "PrefsManager.h"
#include "RageLog.h"
#include "RageFile.h"
#include "RageFileManager.h"
#include "GameConstantsAndTypes.h"
#include "SongManager.h"
#include "GameState.h"
#include "song.h"
#include "Steps.h"
#include "Course.h"
#include "GameManager.h"
#include "ProductInfo.h"
#include "RageUtil.h"
#include "ThemeManager.h"
#include "MemoryCardManager.h"
#include "XmlFile.h"
#include "StepsUtil.h"
#include "Style.h"
#include "HighScore.h"
#include "Character.h"
#include "CharacterManager.h"
#include "FileDownload.h"
#include "ScreenManager.h"


ProfileManager*	PROFILEMAN = NULL;	// global and accessable from anywhere in our program

static void DefaultLocalProfileIDInit( size_t /*PlayerNumber*/ i, RString &sNameOut, RString &defaultValueOut )
{
	sNameOut = ssprintf( "DefaultLocalProfileIDP%d", int(i+1) );
	defaultValueOut = "";
}

Preference1D<RString> ProfileManager::m_sDefaultLocalProfileID( DefaultLocalProfileIDInit, NUM_PLAYERS );

const RString NEW_MEM_CARD_NAME	=	"";
const RString USER_PROFILES_DIR	=	"/Save/LocalProfiles/";
const RString MACHINE_PROFILE_DIR =	"/Save/MachineProfile/";
const RString LAST_GOOD_SUBDIR	=	"LastGood/";


// Directories to search for a profile if m_sMemoryCardProfileSubdir doesn't
// exist, separated by ";":
static Preference<RString> g_sMemoryCardProfileImportSubdirs( "MemoryCardProfileImportSubdirs", "" );

static RString LocalProfileIDToDir( const RString &sProfileID ) { return USER_PROFILES_DIR + sProfileID + "/"; }
static RString LocalProfileDirToID( const RString &sDir ) { return Basename( sDir ); }

struct DirAndProfile
{
	RString sDir;
	Profile profile;
};
static vector<DirAndProfile> g_vLocalProfile;


static ThemeMetric<bool>	FIXED_PROFILES		( "ProfileManager", "FixedProfiles" );
static ThemeMetric<int>		NUM_FIXED_PROFILES	( "ProfileManager", "NumFixedProfiles" );
#define FIXED_PROFILE_CHARACTER_ID( i ) THEME->GetMetric( "ProfileManager", ssprintf("FixedProfileCharacterID%d",int(i+1)) )


ProfileManager::ProfileManager()
{
	m_pMachineProfile = new Profile;
	FOREACH_PlayerNumber(pn)
		m_pMemoryCardProfile[pn] = new Profile;
}

ProfileManager::~ProfileManager()
{
	SAFE_DELETE( m_pMachineProfile );
	FOREACH_PlayerNumber(pn)
		SAFE_DELETE( m_pMemoryCardProfile[pn] );
}

void ProfileManager::Init()
{
	FOREACH_PlayerNumber( p )
	{
		m_bWasLoadedFromMemoryCard[p] = false;
		m_bLastLoadWasTamperedOrCorrupt[p] = false;
		m_bLastLoadWasFromLastGood[p] = false;
		m_bNeedToBackUpLastLoad[p] = false;
	}

	LoadMachineProfile();

	RefreshLocalProfilesFromDisk();

	if( FIXED_PROFILES )
	{
		// resize to the fixed number
		if( (int)g_vLocalProfile.size() > NUM_FIXED_PROFILES )
			g_vLocalProfile.erase( g_vLocalProfile.begin()+NUM_FIXED_PROFILES, g_vLocalProfile.end() );
		
		for( int i=g_vLocalProfile.size(); i<NUM_FIXED_PROFILES; i++ )
		{
			RString sCharacterID = FIXED_PROFILE_CHARACTER_ID( i );
			Character *pCharacter = CHARMAN->GetCharacterFromID( sCharacterID );
			ASSERT_M( pCharacter, sCharacterID );
			RString sProfileID;
			bool b = CreateLocalProfile( pCharacter->GetDisplayName(), sProfileID );
			ASSERT( b );
			Profile* pProfile = GetLocalProfile( sProfileID );
			ASSERT_M( pProfile, sProfileID );
			pProfile->m_sCharacterID = sCharacterID;
			SaveLocalProfile( sProfileID );
		}

		ASSERT( (int)g_vLocalProfile.size() == NUM_FIXED_PROFILES );
	}
}

bool ProfileManager::FixedProfiles() const
{
	return FIXED_PROFILES;
}

ProfileLoadResult ProfileManager::LoadProfile( PlayerNumber pn, RString sProfileDir, bool bIsMemCard )
{
	LOG->Trace( "LoadingProfile P%d, %s, %d", pn+1, sProfileDir.c_str(), bIsMemCard );

	ASSERT( !sProfileDir.empty() );
	ASSERT( sProfileDir.Right(1) == "/" );


	m_sProfileDir[pn] = sProfileDir;
	m_bWasLoadedFromMemoryCard[pn] = bIsMemCard;
	m_bLastLoadWasFromLastGood[pn] = false;
	m_bNeedToBackUpLastLoad[pn] = false;

	// Try to load the original, non-backup data.
	ProfileLoadResult lr = GetProfile(pn)->LoadAllFromDir( m_sProfileDir[pn], PREFSMAN->m_bSignProfileData );
	
	RString sBackupDir = m_sProfileDir[pn] + LAST_GOOD_SUBDIR;

	if( lr == ProfileLoadResult_Success )
	{
		/* Next time the profile is written, move this good profile into LastGood. */
		m_bNeedToBackUpLastLoad[pn] = true;
	}

	m_bLastLoadWasTamperedOrCorrupt[pn] = lr == ProfileLoadResult_FailedTampered;

	//
	// Try to load from the backup if the original data fails to load
	//
	if( lr == ProfileLoadResult_FailedTampered )
	{
		lr = GetProfile(pn)->LoadAllFromDir( sBackupDir, PREFSMAN->m_bSignProfileData );
		m_bLastLoadWasFromLastGood[pn] = lr == ProfileLoadResult_Success;

		/* If the LastGood profile doesn't exist at all, and the actual profile was failed_tampered,
		 * then the error should be failed_tampered and not failed_no_profile. */
		if( lr == ProfileLoadResult_FailedNoProfile )
		{
			LOG->Trace( "Profile was corrupt and LastGood for %s doesn't exist; error is ProfileLoadResult_FailedTampered",
					sProfileDir.c_str() );
			lr = ProfileLoadResult_FailedTampered;
		}
	}

	LOG->Trace( "Done loading profile - result %d", lr );

	return lr;
}

bool ProfileManager::LoadLocalProfileFromMachine( PlayerNumber pn )
{
	RString sProfileID = m_sDefaultLocalProfileID[pn];
	if( sProfileID.empty() )
	{
		m_sProfileDir[pn] = "";
		return false;
	}

	m_sProfileDir[pn] = LocalProfileIDToDir( sProfileID );
	m_bWasLoadedFromMemoryCard[pn] = false;
	m_bLastLoadWasFromLastGood[pn] = false;

	if( GetLocalProfile(sProfileID) == NULL )
	{
		m_sProfileDir[pn] = "";
		return false;
	}

	return true;
}

void ProfileManager::GetMemoryCardProfileDirectoriesToTry( vector<RString> &asDirsToTry )
{
	/* Try to load the preferred profile. */
	asDirsToTry.push_back( PREFSMAN->m_sMemoryCardProfileSubdir );

	/* If that failed, try loading from all fallback directories. */
	split( g_sMemoryCardProfileImportSubdirs, ";", asDirsToTry, true );
}

bool ProfileManager::LoadProfileFromMemoryCard( PlayerNumber pn )
{
	UnloadProfile( pn );

	// mount slot
	if( MEMCARDMAN->GetCardState(pn) != MemoryCardState_Ready )
		return false;

	vector<RString> asDirsToTry;
	GetMemoryCardProfileDirectoriesToTry( asDirsToTry );

	for( unsigned i = 0; i < asDirsToTry.size(); ++i )
	{
		const RString &sSubdir = asDirsToTry[i];
		RString sDir = MEM_CARD_MOUNT_POINT[pn] + sSubdir + "/";

		/* If the load fails with ProfileLoadResult_FailedNoProfile, keep searching.  However,
		 * if it fails with failed_tampered, data existed but couldn't be loaded;
		 * we don't want to mess with it, since it's confusing and may wipe out
		 * recoverable backup data.  The only time we really want to import data
		 * is on the very first use, when the new profile doesn't exist at all,
		 * but we also want to import scores in the case where the player created
		 * a directory for edits before playing, so keep searching if the directory
		 * exists with exists with no scores. */
		ProfileLoadResult res = LoadProfile( pn, sDir, true );
		if( res == ProfileLoadResult_Success )
		{
			/* If importing, store the directory we imported from, for display purposes. */
			if( i > 0 )
				m_sProfileDirImportedFrom[pn] = asDirsToTry[i];
			break;
		}
		
		if( res == ProfileLoadResult_FailedTampered )
			break;
	}

	/* If we imported a profile fallback directory, change the memory card
	 * directory back to the preferred directory: never write over imported
	 * scores. */
	m_sProfileDir[pn] = MEM_CARD_MOUNT_POINT[pn] + (RString)PREFSMAN->m_sMemoryCardProfileSubdir + "/";

	/* Load edits from all fallback directories, newest first. */
	for( unsigned i = 0; i < asDirsToTry.size(); ++i )
	{
		const RString &sSubdir = asDirsToTry[i];
		RString sDir = MEM_CARD_MOUNT_POINT[pn] + sSubdir + "/";

		SONGMAN->LoadAllFromProfileDir( sDir, (ProfileSlot) pn );
	}

	return true; // If a card is inserted, we want to use the memory card to save - even if the Profile load failed.
}
			
bool ProfileManager::LoadFirstAvailableProfile( PlayerNumber pn )
{
	if( LoadProfileFromMemoryCard(pn) )
		return true;

	if( LoadLocalProfileFromMachine(pn) )
		return true;
	
	return false;
}


bool ProfileManager::FastLoadProfileNameFromMemoryCard( RString sRootDir, RString &sName ) const
{
	vector<RString> asDirsToTry;
	GetMemoryCardProfileDirectoriesToTry( asDirsToTry );

	for( unsigned i = 0; i < asDirsToTry.size(); ++i )
	{
		const RString &sSubdir = asDirsToTry[i];
		RString sDir = sRootDir + sSubdir + "/";

		Profile profile;
		ProfileLoadResult res = profile.LoadEditableDataFromDir( sDir );
		if( res == ProfileLoadResult_Success )
		{
			sName = profile.GetDisplayNameOrHighScoreName();
			return true;
		}
		else if( res != ProfileLoadResult_FailedNoProfile )
			break;
	}

	return false;
}


bool ProfileManager::SaveProfile( PlayerNumber pn ) const
{
	if( m_sProfileDir[pn].empty() )
		return false;

	/*
	 * If the profile we're writing was loaded from the primary (non-backup)
	 * data, then we've validated it and know it's good.  Before writing our
	 * new data, move the old, good data to the backup.  (Only do this once;
	 * if we save the profile more than once, we havn't re-validated the
	 * newly written data.)
	 */
	if( m_bNeedToBackUpLastLoad[pn] )
	{
		m_bNeedToBackUpLastLoad[pn] = false;
		RString sBackupDir = m_sProfileDir[pn] + LAST_GOOD_SUBDIR;
		Profile::MoveBackupToDir( m_sProfileDir[pn], sBackupDir );
	}

	bool b = GetProfile(pn)->SaveAllToDir( m_sProfileDir[pn], PREFSMAN->m_bSignProfileData );

	return b;
}

bool ProfileManager::SaveLocalProfile( RString sProfileID )
{
	Profile *pProfile = GetLocalProfile( sProfileID );
	ASSERT( pProfile != NULL );
	RString sDir = LocalProfileIDToDir( sProfileID );
	bool b = pProfile->SaveAllToDir( sDir, PREFSMAN->m_bSignProfileData );
	return b;
}

void ProfileManager::UnloadProfile( PlayerNumber pn )
{
	m_sProfileDir[pn] = "";
	m_sProfileDirImportedFrom[pn] = "";
	m_bWasLoadedFromMemoryCard[pn] = false;
	m_bLastLoadWasTamperedOrCorrupt[pn] = false;
	m_bLastLoadWasFromLastGood[pn] = false;
	m_bNeedToBackUpLastLoad[pn] = false;
	m_pMemoryCardProfile[pn]->InitAll();
	SONGMAN->FreeAllLoadedFromProfile( (ProfileSlot) pn );
}

const Profile* ProfileManager::GetProfile( PlayerNumber pn ) const
{
	ASSERT( pn >= 0 && pn < NUM_PLAYERS );

	if( m_sProfileDir[pn].empty() )
	{
		// return an empty profile
		return m_pMemoryCardProfile[pn];
	}
	else if( ProfileWasLoadedFromMemoryCard(pn) )
	{
		return m_pMemoryCardProfile[pn];
	}
	else
	{
		RString sProfileID = LocalProfileDirToID( m_sProfileDir[pn] );
		return GetLocalProfile( sProfileID );
	}
}

RString ProfileManager::GetPlayerName( PlayerNumber pn ) const
{
	const Profile *prof = GetProfile( pn );
	return prof ? prof->GetDisplayNameOrHighScoreName() : RString();
}


void ProfileManager::UnloadAllLocalProfiles()
{
	g_vLocalProfile.clear();
}

void ProfileManager::RefreshLocalProfilesFromDisk()
{
	UnloadAllLocalProfiles();

	vector<RString> vsProfileID;
	GetDirListing( USER_PROFILES_DIR + "*", vsProfileID, true, true );
	FOREACH_CONST( RString, vsProfileID, p )
	{
		g_vLocalProfile.push_back( DirAndProfile() );
		DirAndProfile &dap = g_vLocalProfile.back();
		dap.sDir = *p + "/";
		dap.profile.LoadAllFromDir( dap.sDir, PREFSMAN->m_bSignProfileData );
	}
}

const Profile *ProfileManager::GetLocalProfile( const RString &sProfileID ) const
{
	RString sDir = LocalProfileIDToDir( sProfileID );
	FOREACH_CONST( DirAndProfile, g_vLocalProfile, dap )
	{
		const RString &sOther = dap->sDir;
		if( sOther == sDir )
			return &dap->profile;
	}

	return NULL;
}

bool ProfileManager::CreateLocalProfile( RString sName, RString &sProfileIDOut )
{
	ASSERT( !sName.empty() );

	//
	// Find a directory directory name that's a number greater than all 
	// existing numbers.  This preserves the "order by create date".
	//
	int iMaxProfileNumber = -1;
	vector<RString> vs;
	GetLocalProfileIDs( vs );
	FOREACH_CONST( RString, vs, s )
		iMaxProfileNumber = atoi( *s );

	int iProfileNumber = iMaxProfileNumber + 1;
	RString sProfileID = ssprintf( "%08d", iProfileNumber );

	//
	// Create the new profile.
	//
	Profile *pProfile = new Profile;
	pProfile->m_sDisplayName = sName;
	pProfile->m_sCharacterID = CHARMAN->GetRandomCharacter()->m_sCharacterID;

	//
	// Save it to disk.
	//
	RString sProfileDir = LocalProfileIDToDir( sProfileID );
	if( !pProfile->SaveAllToDir(sProfileDir, PREFSMAN->m_bSignProfileData) )
	{
		delete pProfile;
		sProfileIDOut = "";
		return false;
	}

	AddLocalProfileByID( pProfile, sProfileID );

	sProfileIDOut = sProfileID;
	return true;
}

void ProfileManager::AddLocalProfileByID( Profile *pProfile, RString sProfileID )
{
	// make sure this id doesn't already exist
	ASSERT_M( GetLocalProfile(sProfileID) == NULL,
		ssprintf("creating \"%s\" \"%s\" that already exists",
		pProfile->m_sDisplayName.c_str(), sProfileID.c_str()) );

	// insert
	g_vLocalProfile.push_back( DirAndProfile() );
	DirAndProfile &dap = g_vLocalProfile.back();
	dap.sDir = LocalProfileIDToDir( sProfileID );
	dap.profile = *pProfile;
}

bool ProfileManager::RenameLocalProfile( RString sProfileID, RString sNewName )
{
	ASSERT( !sProfileID.empty() );

	Profile *pProfile = ProfileManager::GetLocalProfile( sProfileID );
	ASSERT( pProfile );
	pProfile->m_sDisplayName = sNewName;

	RString sProfileDir = LocalProfileIDToDir( sProfileID );
	return pProfile->SaveAllToDir( sProfileDir, PREFSMAN->m_bSignProfileData );
}

bool ProfileManager::DeleteLocalProfile( RString sProfileID )
{
	Profile *pProfile = ProfileManager::GetLocalProfile( sProfileID );
	ASSERT( pProfile );
	RString sProfileDir = LocalProfileIDToDir( sProfileID );

	FOREACH( DirAndProfile, g_vLocalProfile, i )
	{
		if( i->sDir == sProfileDir )
		{
			if( DeleteRecursive(sProfileDir) )
			{
				g_vLocalProfile.erase( i );

				// Delete all references to this profileID
				FOREACH_CONST( Preference<RString>*, PROFILEMAN->m_sDefaultLocalProfileID.m_v, i )
				{
					if( (*i)->Get() == sProfileID )
						(*i)->Set( "" );
				}

				return true;
			}
			else
			{
				return false;
			}
		}
	}



	LOG->Warn( "DeleteLocalProfile: ProfileID '%s' doesn't exist", sProfileID.c_str() );
	return false;
}

void ProfileManager::SaveMachineProfile() const
{
	// If the machine name has changed, make sure we use the new name.
	// It's important that this name be applied before the Player profiles 
	// are saved, so that the Player's profiles show the right machine name.
	const_cast<ProfileManager *> (this)->m_pMachineProfile->m_sDisplayName = PREFSMAN->m_sMachineName;

	m_pMachineProfile->SaveAllToDir( MACHINE_PROFILE_DIR, false ); /* don't sign machine profiles */
}

void ProfileManager::UploadMachineProfie() const
{
	//TODO: Copy the stats file to temp before uploaing
	RString sFile = MACHINE_PROFILE_DIR+"Stats.json.gz";
	RString sUrl = "unknown url";
	{
		FileTransfer ft;
		// TODO: Don't stream file because it will block future writes.  
		ft.StartUpload( "http://www.stepmania.com/api.php?action=upload_stats", sFile, "" );
		ft.BlockUntilFinished();
		if( ft.GetResponseCode() != 200 )
			SCREENMAN->SystemMessage( "Bad response code upload_stats " + ft.GetResponseCode() );
	}
}

void ProfileManager::LoadMachineProfile()
{
	ProfileLoadResult lr = m_pMachineProfile->LoadAllFromDir(MACHINE_PROFILE_DIR, false);
	if( lr == ProfileLoadResult_FailedNoProfile )
	{
		m_pMachineProfile->InitAll();
		m_pMachineProfile->SaveAllToDir( MACHINE_PROFILE_DIR, PREFSMAN->m_bSignProfileData );
	}

	// If the machine name has changed, make sure we use the new name
	m_pMachineProfile->m_sDisplayName = PREFSMAN->m_sMachineName;

	SONGMAN->FreeAllLoadedFromProfile( ProfileSlot_Machine );
	SONGMAN->LoadAllFromProfileDir( MACHINE_PROFILE_DIR, ProfileSlot_Machine );
}

bool ProfileManager::ProfileWasLoadedFromMemoryCard( PlayerNumber pn ) const
{
	return !m_sProfileDir[pn].empty() && m_bWasLoadedFromMemoryCard[pn];
}

bool ProfileManager::LastLoadWasTamperedOrCorrupt( PlayerNumber pn ) const
{
	return !m_sProfileDir[pn].empty() && m_bLastLoadWasTamperedOrCorrupt[pn];
}

bool ProfileManager::LastLoadWasFromLastGood( PlayerNumber pn ) const
{
	return !m_sProfileDir[pn].empty() && m_bLastLoadWasFromLastGood[pn];
}

const RString& ProfileManager::GetProfileDir( ProfileSlot slot ) const
{
	switch( slot )
	{
	case ProfileSlot_Player1:
	case ProfileSlot_Player2:
		return m_sProfileDir[slot];
	case ProfileSlot_Machine:
		return MACHINE_PROFILE_DIR;
	default:
		ASSERT(0);
	}
}

RString ProfileManager::GetProfileDirImportedFrom( ProfileSlot slot ) const
{
	switch( slot )
	{
	case ProfileSlot_Player1:
	case ProfileSlot_Player2:
		return m_sProfileDirImportedFrom[slot];
	case ProfileSlot_Machine:
		return NULL;
	default:
		ASSERT(0);
	}
}

const Profile* ProfileManager::GetProfile( ProfileSlot slot ) const
{
	switch( slot )
	{
	case ProfileSlot_Player1:
	case ProfileSlot_Player2:
		return GetProfile( (PlayerNumber)slot );
	case ProfileSlot_Machine:
		return m_pMachineProfile;
	default:
		ASSERT(0);
	}
}

//
// General
//
void ProfileManager::IncrementToastiesCount( PlayerNumber pn )
{
	if( PROFILEMAN->IsPersistentProfile(pn) )
		++PROFILEMAN->GetProfile(pn)->m_iNumToasties;
	++PROFILEMAN->GetMachineProfile()->m_iNumToasties;
}

void ProfileManager::AddStepTotals( PlayerNumber pn, int iNumTapsAndHolds, int iNumJumps, int iNumHolds, int iNumRolls, int iNumMines, int iNumHands, float fCaloriesBurned )
{
	if( PROFILEMAN->IsPersistentProfile(pn) )
		PROFILEMAN->GetProfile(pn)->AddStepTotals( iNumTapsAndHolds, iNumJumps, iNumHolds, iNumRolls, iNumMines, iNumHands, fCaloriesBurned );
	PROFILEMAN->GetMachineProfile()->AddStepTotals( iNumTapsAndHolds, iNumJumps, iNumHolds, iNumRolls, iNumMines, iNumHands, fCaloriesBurned );
}

//
// Song stats
//
int ProfileManager::GetSongNumTimesPlayed( const Song* pSong, ProfileSlot slot ) const
{
	return GetProfile(slot)->GetSongNumTimesPlayed( pSong );
}

void ProfileManager::AddStepsScore( const Song* pSong, const Steps* pSteps, PlayerNumber pn, const HighScore &hs_, int &iPersonalIndexOut, int &iMachineIndexOut )
{
	HighScore hs = hs_;
	hs.SetPercentDP( max(0, hs.GetPercentDP()) ); // bump up negative scores

	iPersonalIndexOut = -1;
	iMachineIndexOut = -1;

	// In event mode, set the score's name immediately to the Profile's last
	// used name.  If no profile last used name exists, use "EVNT".
	if( GAMESTATE->IsEventMode() )
	{
		Profile* pProfile = PROFILEMAN->GetProfile(pn);
		if( pProfile && !pProfile->m_sLastUsedHighScoreName.empty() )
			hs.SetName( pProfile->m_sLastUsedHighScoreName );
		else
			hs.SetName( "EVNT" );
	}
	else
	{
		hs.SetName( RANKING_TO_FILL_IN_MARKER[pn] );
	}

	//
	// save high score	
	//
	if( PROFILEMAN->IsPersistentProfile(pn) )
		PROFILEMAN->GetProfile(pn)->AddStepsHighScore( pSong, pSteps, hs, iPersonalIndexOut );

	if( hs.GetPercentDP() >= PREFSMAN->m_fMinPercentageForMachineSongHighScore )
	{
		// don't leave machine high scores for edits loaded from the player's card
		if( !pSteps->IsAPlayerEdit() )
			PROFILEMAN->GetMachineProfile()->AddStepsHighScore( pSong, pSteps, hs, iMachineIndexOut );
	}

	//
	// save recent score	
	//
	if( PROFILEMAN->IsPersistentProfile(pn) )
		PROFILEMAN->GetProfile(pn)->AddStepsRecentScore( pSong, pSteps, hs );
	PROFILEMAN->GetMachineProfile()->AddStepsRecentScore( pSong, pSteps, hs );
}

void ProfileManager::IncrementStepsPlayCount( const Song* pSong, const Steps* pSteps, PlayerNumber pn )
{
	if( PROFILEMAN->IsPersistentProfile(pn) )
		PROFILEMAN->GetProfile(pn)->IncrementStepsPlayCount( pSong, pSteps );
	PROFILEMAN->GetMachineProfile()->IncrementStepsPlayCount( pSong, pSteps );
}

void ProfileManager::GetHighScoreForDifficulty( const Song *s, const Style *st, ProfileSlot slot, Difficulty dc, HighScore &hsOut ) const
{
	// return max grade of notes in difficulty class
	vector<Steps*> aNotes;
	SongUtil::GetSteps( s, aNotes, st->m_StepsType );
	StepsUtil::SortNotesArrayByDifficulty( aNotes );

	const Steps* pSteps = SongUtil::GetStepsByDifficulty( s, st->m_StepsType, dc );

	if( pSteps && PROFILEMAN->IsPersistentProfile(slot) )
		hsOut = PROFILEMAN->GetProfile(slot)->GetStepsHighScoreList(s,pSteps).GetTopScore();
	else
		hsOut = HighScore();
}


//
// Course stats
//
void ProfileManager::AddCourseScore( const Course* pCourse, const Trail* pTrail, PlayerNumber pn, const HighScore &hs_, int &iPersonalIndexOut, int &iMachineIndexOut )
{
	HighScore hs = hs_;
	hs.SetPercentDP(max( 0, hs.GetPercentDP()) ); // bump up negative scores

	iPersonalIndexOut = -1;
	iMachineIndexOut = -1;

	// In event mode, set the score's name immediately to the Profile's last
	// used name.  If no profile last used name exists, use "EVNT".
	if( GAMESTATE->IsEventMode() )
	{
		Profile* pProfile = PROFILEMAN->GetProfile(pn);
		if( pProfile && !pProfile->m_sLastUsedHighScoreName.empty() )
			hs.SetName(  pProfile->m_sLastUsedHighScoreName );
		else
			hs.SetName( "EVNT" );
	}
	else
	{
		hs.SetName( RANKING_TO_FILL_IN_MARKER[pn] );
	}


	//
	// save high score	
	//
	if( PROFILEMAN->IsPersistentProfile(pn) )
		PROFILEMAN->GetProfile(pn)->AddCourseHighScore( pCourse, pTrail, hs, iPersonalIndexOut );
	if( hs.GetPercentDP() >= PREFSMAN->m_fMinPercentageForMachineCourseHighScore )
		PROFILEMAN->GetMachineProfile()->AddCourseHighScore( pCourse, pTrail, hs, iMachineIndexOut );

	//
	// save recent score	
	//
	if( PROFILEMAN->IsPersistentProfile(pn) )
		PROFILEMAN->GetProfile(pn)->AddCourseRecentScore( pCourse, pTrail, hs );
	PROFILEMAN->GetMachineProfile()->AddCourseRecentScore( pCourse, pTrail, hs );
}

void ProfileManager::IncrementCoursePlayCount( const Course* pCourse, const Trail* pTrail, PlayerNumber pn )
{
	if( PROFILEMAN->IsPersistentProfile(pn) )
		PROFILEMAN->GetProfile(pn)->IncrementCoursePlayCount( pCourse, pTrail );
	PROFILEMAN->GetMachineProfile()->IncrementCoursePlayCount( pCourse, pTrail );
}


//
// Category stats
//
void ProfileManager::AddCategoryScore( StepsType st, RankingCategory rc, PlayerNumber pn, const HighScore &hs_, int &iPersonalIndexOut, int &iMachineIndexOut )
{
	HighScore hs = hs_;
	hs.SetName( RANKING_TO_FILL_IN_MARKER[pn] );
	if( PROFILEMAN->IsPersistentProfile(pn) )
		PROFILEMAN->GetProfile(pn)->AddCategoryHighScore( st, rc, hs, iPersonalIndexOut );
	if( hs.GetPercentDP() > PREFSMAN->m_fMinPercentageForMachineSongHighScore )
		PROFILEMAN->GetMachineProfile()->AddCategoryHighScore( st, rc, hs, iMachineIndexOut );
}

void ProfileManager::IncrementCategoryPlayCount( StepsType st, RankingCategory rc, PlayerNumber pn )
{
	if( PROFILEMAN->IsPersistentProfile(pn) )
		PROFILEMAN->GetProfile(pn)->IncrementCategoryPlayCount( st, rc );
	PROFILEMAN->GetMachineProfile()->IncrementCategoryPlayCount( st, rc );
}

bool ProfileManager::IsPersistentProfile( ProfileSlot slot ) const
{
	switch( slot )
	{
	case ProfileSlot_Player1:
	case ProfileSlot_Player2:
		return GAMESTATE->IsHumanPlayer((PlayerNumber)slot) && !m_sProfileDir[slot].empty(); 
	case ProfileSlot_Machine:
		return true;
	default:
		ASSERT(0);
		return false;
	}
}

void ProfileManager::GetLocalProfileIDs( vector<RString> &vsProfileIDsOut ) const
{
	vsProfileIDsOut.clear();
	FOREACH_CONST( DirAndProfile, g_vLocalProfile, i)
	{
		RString sID = LocalProfileDirToID( i->sDir );
		vsProfileIDsOut.push_back( sID );
	}
}

void ProfileManager::GetLocalProfileDisplayNames( vector<RString> &vsProfileDisplayNamesOut ) const
{
	vsProfileDisplayNamesOut.clear();
	FOREACH_CONST( DirAndProfile, g_vLocalProfile, i)
		vsProfileDisplayNamesOut.push_back( i->profile.m_sDisplayName );
}

int ProfileManager::GetLocalProfileIndexFromID( RString sProfileID ) const
{
	RString sDir = LocalProfileIDToDir( sProfileID );
	FOREACH_CONST( DirAndProfile, g_vLocalProfile, i )
	{
		if( i->sDir == sProfileID )
			return i - g_vLocalProfile.begin();
	}
	return -1;
}

RString ProfileManager::GetLocalProfileIDFromIndex( int iIndex )
{
	RString sID = LocalProfileDirToID( g_vLocalProfile[iIndex].sDir );
	return sID;
}

Profile *ProfileManager::GetLocalProfileFromIndex( int iIndex )
{
	return &g_vLocalProfile[iIndex].profile;
}

int ProfileManager::GetNumLocalProfiles() const
{
	return g_vLocalProfile.size();
}


// lua start
#include "LuaBinding.h"

class LunaProfileManager: public Luna<ProfileManager>
{
public:
	LunaProfileManager() { LUA->Register( Register ); }

	static int IsPersistentProfile( T* p, lua_State *L )	{ lua_pushboolean(L, p->IsPersistentProfile((PlayerNumber)IArg(1)) ); return 1; }
	static int GetProfile( T* p, lua_State *L )				{ PlayerNumber pn = (PlayerNumber)IArg(1); Profile* pP = p->GetProfile(pn); ASSERT(pP); pP->PushSelf(L); return 1; }
	static int GetMachineProfile( T* p, lua_State *L )		{ p->GetMachineProfile()->PushSelf(L); return 1; }
	static int SaveMachineProfile( T* p, lua_State *L )		{ p->SaveMachineProfile(); return 0; }
	static int GetLocalProfile( T* p, lua_State *L )
	{
		Profile *pProfile = p->GetLocalProfile(SArg(1));
		if( pProfile ) 
			pProfile->PushSelf(L);
		else
			lua_pushnil(L );
		return 1;
	}
	static int GetLocalProfileFromIndex( T* p, lua_State *L ) { Profile *pProfile = p->GetLocalProfileFromIndex(IArg(1)); ASSERT(pProfile); pProfile->PushSelf(L); return 1; }
	static int GetLocalProfileIDFromIndex( T* p, lua_State *L )	{ lua_pushstring(L, p->GetLocalProfileIDFromIndex(IArg(1)) ); return 1; }
	static int GetLocalProfileIndexFromID( T* p, lua_State *L )	{ lua_pushnumber(L, p->GetLocalProfileIndexFromID(SArg(1)) ); return 1; }
	static int GetNumLocalProfiles( T* p, lua_State *L )	{ lua_pushnumber(L, p->GetNumLocalProfiles() ); return 1; }

	static void Register(lua_State *L)
	{
		ADD_METHOD( IsPersistentProfile );
		ADD_METHOD( GetProfile );
		ADD_METHOD( GetMachineProfile );
		ADD_METHOD( SaveMachineProfile );
		ADD_METHOD( GetLocalProfile );
		ADD_METHOD( GetLocalProfileFromIndex );
		ADD_METHOD( GetLocalProfileIDFromIndex );
		ADD_METHOD( GetLocalProfileIndexFromID );
		ADD_METHOD( GetNumLocalProfiles );

		Luna<T>::Register( L );

		// Add global singleton if constructed already.  If it's not constructed yet,
		// then we'll register it later when we reinit Lua just before 
		// initializing the display.
		if( PROFILEMAN )
		{
			lua_pushstring(L, "PROFILEMAN");
			PROFILEMAN->PushSelf( L );
			lua_settable(L, LUA_GLOBALSINDEX);
		}
	}
};

LUA_REGISTER_CLASS( ProfileManager )
// lua end

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
