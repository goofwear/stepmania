/* SongManager - Holder for all Songs and Steps. */

#ifndef SONGMANAGER_H
#define SONGMANAGER_H

class LoadingWindow;
class Song;
class Style;
class Steps;
class PlayerOptions;
struct lua_State;

#include "RageTypes.h"
#include "GameConstantsAndTypes.h"
#include "SongOptions.h"
#include "PlayerOptions.h"
#include "PlayerNumber.h"
#include "Difficulty.h"
#include "Course.h"
#include "ThemeMetric.h"
#include "RageTexturePreloader.h"
#include "RageUtil.h"

const int MAX_EDIT_STEPS_PER_PROFILE	= 200;
const int MAX_EDIT_COURSES_PER_PROFILE	= 20;

class SongManager
{
public:
	SongManager();
	~SongManager();

	void InitSongsFromDisk( LoadingWindow *ld );
	void FreeSongs();
	void Cleanup();

	void Invalidate( const Song *pStaleSong );

	void RegenerateNonFixedCourses();
	void SetPreferences();
	void SaveEnabledSongsToPref();
	void LoadEnabledSongsFromPref();

	void LoadStepEditsFromProfileDir( const RString &sProfileDir, ProfileSlot slot );
	void LoadCourseEditsFromProfileDir( const RString &sProfileDir, ProfileSlot slot );
	int GetNumStepsLoadedFromProfile();
	void FreeAllLoadedFromProfile( ProfileSlot slot = ProfileSlot_Invalid );

	void LoadGroupSymLinks( RString sDir, RString sGroupFolder );

	void InitCoursesFromDisk( LoadingWindow *ld );
	void InitAutogenCourses();
	void InitRandomAttacks();
	void FreeCourses();
	void AddCourse( Course *pCourse );	// transfers ownership of pCourse
	void DeleteCourse( Course *pCourse );	// transfers ownership of pCourse
	void DeleteAutogenCourses();		// deletes every autogen course
	void InvalidateCachedTrails();

	void InitAll( LoadingWindow *ld );	// songs, courses, groups - everything.
	void Reload( bool bAllowFastLoad, LoadingWindow *ld=NULL );	// songs, courses, groups - everything.
	void PreloadSongImages();

	RString GetSongGroupBannerPath( RString sSongGroup ) const;
	void GetSongGroupNames( vector<RString> &AddTo ) const;
	bool DoesSongGroupExist( RString sSongGroup ) const;
	RageColor GetSongGroupColor( const RString &sSongGroupName ) const;
	RageColor GetSongColor( const Song* pSong ) const;

	RString GetCourseGroupBannerPath( const RString &sCourseGroup ) const;
	void GetCourseGroupNames( vector<RString> &AddTo ) const;
	bool DoesCourseGroupExist( const RString &sCourseGroup ) const;
	RageColor GetCourseGroupColor( const RString &sCourseGroupName ) const;
	RageColor GetCourseColor( const Course* pCourse ) const;
	
	static RString ShortenGroupName( RString sLongGroupName );


	// Lookup
	const vector<Song*> &GetSongs( const RString &sGroupName ) const;
	const vector<Song*> &GetAllSongs() const { return GetSongs(GROUP_ALL); }
	const vector<Song*> &GetPopularSongs() const { return m_pPopularSongs; }
	void GetPreferredSortSongs( vector<Song*> &AddTo ) const;
	RString SongToPreferredSortSectionName( const Song *pSong ) const;
	const vector<Course*> &GetPopularCourses( CourseType ct ) const { return m_pPopularCourses[ct]; }
	Song *FindSong( RString sPath ) const;
	Song *FindSong( RString sGroup, RString sSong ) const;
	Course *FindCourse( RString sPath ) const;
	Course *FindCourse( RString sGroup, RString sName ) const;
	int GetNumSongs() const;
	int GetNumUnlockedSongs() const;
	int GetNumSelectableAndUnlockedSongs() const;
	int GetNumAdditionalSongs() const;
	int GetNumSongGroups() const;
	int GetNumCourses() const;
	int GetNumAdditionalCourses() const;
	int GetNumCourseGroups() const;
	Song* GetRandomSong();
	Course* GetRandomCourse();

	void GetStepsLoadedFromProfile( vector<Steps*> &AddTo, ProfileSlot slot ) const;
	Song *GetSongFromSteps( Steps *pSteps ) const;
	void DeleteSteps( Steps *pSteps );	// transfers ownership of pSteps
	bool WasLoadedFromAdditionalSongs( const Song *pSong ) const;
	bool WasLoadedFromAdditionalCourses( const Course *pCourse ) const;

	void GetAllCourses( vector<Course*> &AddTo, bool bIncludeAutogen ) const;
	void GetCourses( CourseType ct, vector<Course*> &AddTo, bool bIncludeAutogen ) const;
	void GetCoursesInGroup( vector<Course*> &AddTo, const RString &sCourseGroup, bool bIncludeAutogen ) const;
	void GetPreferredSortCourses( CourseType ct, vector<Course*> &AddTo, bool bIncludeAutogen ) const;

	void GetExtraStageInfo( bool bExtra2, const Style *s, Song*& pSongOut, Steps*& pStepsOut );
	Song* GetSongFromDir( RString sDir ) const;
	Course* GetCourseFromPath( RString sPath ) const;	// path to .crs file, or path to song group dir
	Course* GetCourseFromName( RString sName ) const;


	void UpdatePopular();
	void UpdateShuffled();		// re-shuffle songs and courses
	void UpdatePreferredSort(); 
	void SortSongs();		// sort m_pSongs by CompareSongPointersByTitle

	void UpdateRankingCourses();	// courses shown on the ranking screen
	void RefreshCourseGroupInfo();

	// Lua
	void PushSelf( lua_State *L );

protected:
	void LoadStepManiaSongDir( RString sDir, LoadingWindow *ld );
	void LoadDWISongDir( RString sDir );
	bool GetExtraStageInfoFromCourse( bool bExtra2, RString sPreferredGroup, Song*& pSongOut, Steps*& pStepsOut );
	void SanityCheckGroupDir( RString sDir ) const;
	void AddGroup( RString sDir, RString sGroupDirName );
	int GetNumEditsLoadedFromProfile( ProfileSlot slot ) const;

	vector<Song*>		m_pSongs;	// all songs that can be played
	vector<Song*>		m_pPopularSongs;
	vector<Song*>		m_pShuffledSongs;	// used by GetRandomSong
	struct PreferredSortSection
	{
		RString sName;
		vector<Song*> vpSongs;
	};
	vector<PreferredSortSection> m_vPreferredSongSort;
	vector<RString>		m_sSongGroupNames;
	vector<RString>		m_sSongGroupBannerPaths; // each song group may have a banner associated with it
	
	struct Comp { bool operator()(const RString& s, const RString &t) const { return CompareRStringsAsc(s,t); } };
	typedef vector<Song*> SongPointerVector;
	map<RString,SongPointerVector,Comp> m_mapSongGroupIndex;

	vector<Course*>		m_pCourses;
	vector<Course*>		m_pPopularCourses[NUM_CourseType];
	vector<Course*>		m_pShuffledCourses;	// used by GetRandomCourse
	struct CourseGroupInfo
	{
		RString m_sBannerPath;
	};
	map<RString,CourseGroupInfo> m_mapCourseGroupToInfo;
	typedef vector<Course*> CoursePointerVector;
	vector<CoursePointerVector> m_vPreferredCourseSort;

	RageTexturePreloader m_TexturePreload;

	ThemeMetric<int>		NUM_SONG_GROUP_COLORS;
	ThemeMetric1D<RageColor>	SONG_GROUP_COLOR;
	ThemeMetric<int>		NUM_COURSE_GROUP_COLORS;
	ThemeMetric1D<RageColor>	COURSE_GROUP_COLOR;
};

extern SongManager*	SONGMAN;	// global and accessable from anywhere in our program

#endif

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
