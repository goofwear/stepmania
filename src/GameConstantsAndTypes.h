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

//
// Note definitions
//
enum RadarCategory	// starting from 12-o'clock rotating clockwise
{
	RADAR_STREAM = 0,
	RADAR_VOLTAGE,
	RADAR_AIR,
	RADAR_FREEZE,
	RADAR_CHAOS,
	NUM_RADAR_CATEGORIES	// leave this at the end
};

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


enum NotesType
{
	NOTES_TYPE_DANCE_SINGLE = 0,
	NOTES_TYPE_DANCE_DOUBLE,
	NOTES_TYPE_DANCE_COUPLE,
	NOTES_TYPE_DANCE_SOLO,
	NOTES_TYPE_PUMP_SINGLE,
	NOTES_TYPE_PUMP_DOUBLE,
	NOTES_TYPE_PUMP_COUPLE,
	NOTES_TYPE_EZ2_SINGLE,
	NOTES_TYPE_EZ2_DOUBLE,
	NOTES_TYPE_EZ2_REAL,
	NOTES_TYPE_PARA_SINGLE,
	NOTES_TYPE_DS3DDX_SINGLE,
	NOTES_TYPE_BM_SINGLE,
	NUM_NOTES_TYPES,		// leave this at the end
	NOTES_TYPE_INVALID,
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
	PLAY_MODE_BATTLE,	// inventory battle
	PLAY_MODE_RAVE,		// DDR Disney Rave "Dance Magic"
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
	SORT_ROULETTE,
	NUM_SORT_ORDERS
};


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

const CString DEFAULT_RANKING_NAME = "STEP";

RankingCategory AverageMeterToRankingCategory( float fAverageMeter );

const int NUM_RANKING_LINES	= 5;
const int MAX_RANKING_NAME_LENGTH	= 4;


//
// Group stuff
//
const CString GROUP_ALL_MUSIC = "";


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

enum BattleResult
{
	RESULT_WIN,
	RESULT_LOSE,
	RESULT_DRAW
};




const CString BG_ANIMS_DIR = "BGAnimations/";
const CString VISUALIZATIONS_DIR = "Visualizations/";
const CString RANDOMMOVIES_DIR = "RandomMovies/";



//
// Coin stuff
//

enum CoinMode { COIN_HOME, COIN_PAY, COIN_FREE, NUM_COIN_MODES };

CString CoinModeToString( CoinMode cm );


#endif
