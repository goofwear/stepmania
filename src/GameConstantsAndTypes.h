#ifndef GAME_CONSTANTS_AND_TYPES_H
#define GAME_CONSTANTS_AND_TYPES_H

/*
-----------------------------------------------------------------------------
 File: GameConstantsAndTypes.h

 Desc: Things that are used in many places and don't change often.

 Copyright (c) 2001-2003 by the person(s) listed below.  All rights reserved.
	Chris Danford
	Chris Gomez
-----------------------------------------------------------------------------
*/

#include "PlayerNumber.h"	// TODO: Get rid of this dependency.  -Chris

//
// Screen Dimensions
//
#define	SCREEN_WIDTH	(640)
#define	SCREEN_HEIGHT	(480)

#define	SCREEN_LEFT		(0)
#define	SCREEN_RIGHT	(SCREEN_WIDTH)
#define	SCREEN_TOP		(0)
#define	SCREEN_BOTTOM	(SCREEN_HEIGHT)

#define	CENTER_X		(SCREEN_LEFT + (SCREEN_RIGHT - SCREEN_LEFT)/2.0f)
#define	CENTER_Y		(SCREEN_TOP + (SCREEN_BOTTOM - SCREEN_TOP)/2.0f)

#define	SCREEN_NEAR		(-1000)
#define	SCREEN_FAR		(1000)

#define	ARROW_SIZE		(64)

//
// Note definitions
//
const int MAX_METER = 12;

enum RadarCategory	// starting from 12-o'clock rotating clockwise
{
	RADAR_STREAM = 0,
	RADAR_VOLTAGE,
	RADAR_AIR,
	RADAR_FREEZE,
	RADAR_CHAOS,
	NUM_RADAR_CATEGORIES	// leave this at the end
};

CString CategoryToString( RadarCategory cat );

enum Difficulty 
{
	DIFFICULTY_BEGINNER,	// corresponds to DDREX Beginner
	DIFFICULTY_EASY,		// corresponds to Basic, Easy
	DIFFICULTY_MEDIUM,		// corresponds to Trick, Another, Standard, Normal
	DIFFICULTY_HARD,		// corresponds to Maniac, SSR, Heavy, Crazy
	DIFFICULTY_CHALLENGE,	// corresponds to 5th SMANIAC, MAX2 Challenge, EX Challenge
	NUM_DIFFICULTIES,
	DIFFICULTY_INVALID
};

CString DifficultyToString( Difficulty dc );
Difficulty StringToDifficulty( CString sDC );


enum StepsType
{
	STEPS_TYPE_DANCE_SINGLE = 0,
	STEPS_TYPE_DANCE_DOUBLE,
	STEPS_TYPE_DANCE_COUPLE,
	STEPS_TYPE_DANCE_SOLO,
	STEPS_TYPE_PUMP_SINGLE,
	STEPS_TYPE_PUMP_HALFDOUBLE,
	STEPS_TYPE_PUMP_DOUBLE,
	STEPS_TYPE_PUMP_COUPLE,
	STEPS_TYPE_EZ2_SINGLE,
	STEPS_TYPE_EZ2_DOUBLE,
	STEPS_TYPE_EZ2_REAL,
	STEPS_TYPE_PARA_SINGLE,
	STEPS_TYPE_DS3DDX_SINGLE,
	STEPS_TYPE_BM_SINGLE,
	STEPS_TYPE_MANIAX_SINGLE,
	STEPS_TYPE_MANIAX_DOUBLE,
	STEPS_TYPE_TECHNO_SINGLE8,
	STEPS_TYPE_PNM_FIVE,
	STEPS_TYPE_PNM_NINE,
	NUM_STEPS_TYPES,		// leave this at the end
	STEPS_TYPE_INVALID,
};

//
// Play mode stuff
//
enum PlayMode
{
	PLAY_MODE_ARCADE,
	PLAY_MODE_NONSTOP,	// DDR EX Nonstop
	PLAY_MODE_ONI,		// DDR EX Challenge
	PLAY_MODE_ENDLESS,	// DDR PlayStation Endless
	PLAY_MODE_BATTLE,	// manually launched attacks
	PLAY_MODE_RAVE,		// automatically launched attacks - DDR Disney Rave "Dance Magic"
	NUM_PLAY_MODES,
	PLAY_MODE_INVALID
};

CString PlayModeToString( PlayMode pm );
PlayMode StringToPlayMode( CString s );




enum SongSortOrder {
	SORT_PREFERRED,
	SORT_GROUP, 
	SORT_TITLE, 
	SORT_BPM, 
	SORT_MOST_PLAYED, 
	SORT_GRADE,
	SORT_ARTIST,
	SORT_EASY_METER,
	SORT_MEDIUM_METER,
	SORT_HARD_METER,
	SORT_MENU,
	SORT_ALL_COURSES,
	SORT_NONSTOP_COURSES,
	SORT_ONI_COURSES,
	SORT_ENDLESS_COURSES,
	SORT_ROULETTE,
	NUM_SORT_ORDERS,
	SORT_INVALID
};
const SongSortOrder MAX_SELECTABLE_SORT = (SongSortOrder)(SORT_ROULETTE-1);

CString SongSortOrderToString( SongSortOrder so );
SongSortOrder StringToSongSortOrder( CString str );


//
// Scoring stuff
//

enum TapNoteScore { 
	TNS_NONE, 
	TNS_MISS,
	TNS_BOO,
	TNS_GOOD,
	TNS_GREAT,
	TNS_PERFECT,
	TNS_MARVELOUS,
	NUM_TAP_NOTE_SCORES
};

CString TapNoteScoreToString( TapNoteScore tns );

//enum TapNoteTiming { 
//	TNT_NONE, 
//	TNT_EARLY, 
//	TNT_LATE 
//};


enum HoldNoteScore 
{ 
	HNS_NONE,	// this HoldNote has not been scored yet
	HNS_OK,		// the HoldNote has passed and was successfully held all the way through
	HNS_NG,		// the HoldNote has passed and they missed it
	NUM_HOLD_NOTE_SCORES
};


//
// MemCard stuff
//
enum MemoryCard
{
	MEMORY_CARD_PLAYER_1,
	MEMORY_CARD_PLAYER_2,
	MEMORY_CARD_MACHINE,
	NUM_MEMORY_CARDS
};


//
// Ranking stuff
//
enum RankingCategory
{
	RANKING_A,	// 1-3 meter per song avg.
	RANKING_B,	// 4-6 meter per song avg.
	RANKING_C,	// 7-9 meter per song avg.
	RANKING_D,	// 10+ meter per song avg.	// doesn't count extra stage!
	NUM_RANKING_CATEGORIES
};

const CString DEFAULT_RANKING_NAME = "";
const CString RANKING_TO_FILL_IN_MARKER[NUM_PLAYERS] = {"#P1#","#P2#"};

RankingCategory AverageMeterToRankingCategory( float fAverageMeter );

const int NUM_RANKING_LINES	= 5;
const int MAX_RANKING_NAME_LENGTH	= 4;


//
// Group stuff
//
#define GROUP_ALL_MUSIC	CString("")


//
//
//
enum PlayerController
{
	PC_HUMAN,
	PC_CPU,
	PC_AUTOPLAY,
	NUM_PLAYER_CONTROLLERS
};

const int MIN_SKILL = 0;
const int MAX_SKILL = 10;


enum StageResult
{
	RESULT_WIN,
	RESULT_LOSE,
	RESULT_DRAW
};


//
// Battle stuff
//
const int NUM_INVENTORY_SLOTS	= 3;
enum AttackLevel
{
	ATTACK_LEVEL_1,
	ATTACK_LEVEL_2,
	ATTACK_LEVEL_3,
	NUM_ATTACK_LEVELS
};
const int NUM_ATTACKS_PER_LEVEL	= 3;
const int ITEM_NONE = -1;


#define BG_ANIMS_DIR		CString(BASE_PATH "BGAnimations" SLASH)
#define VISUALIZATIONS_DIR	CString(BASE_PATH "Visualizations" SLASH)
#define RANDOMMOVIES_DIR	CString(BASE_PATH "RandomMovies" SLASH)


#define PROFILES_DIR		BASE_PATH "Data" SLASH "Profiles" SLASH


//
// Coin stuff
//

enum CoinMode { COIN_HOME, COIN_PAY, COIN_FREE, NUM_COIN_MODES };

CString CoinModeToString( CoinMode cm );


#endif
