#ifndef MUSICWHEEL_H
#define MUSICWHEEL_H
/*
-----------------------------------------------------------------------------
 Class: MusicWheel

 Desc: A wheel with song names used in the Select Music screen.

 Copyright (c) 2001-2002 by the person(s) listed below.  All rights reserved.
	Chris Danford
-----------------------------------------------------------------------------
*/

#include "Sprite.h"
#include "ActorFrame.h"
#include "RageSound.h"
#include "RandomSample.h"
#include "GameConstantsAndTypes.h"
#include "MusicSortDisplay.h"
#include "ScreenMessage.h"
#include "ScoreDisplayNormal.h"
#include "ScrollBar.h"
#include "RageTimer.h"
#include "MusicWheelItem.h"
class Course;
class Song;


const int MAX_WHEEL_ITEMS	=	25;

const ScreenMessage	SM_SongChanged		=	ScreenMessage(SM_User+47);	// this should be unique!
const ScreenMessage SM_SortOrderChanging=	ScreenMessage(SM_User+48);	
const ScreenMessage SM_SortOrderChanged	=	ScreenMessage(SM_User+49);	


struct CompareSongPointerArrayBySectionName;

class MusicWheel : public ActorFrame
{
	friend struct CompareSongPointerArrayBySectionName;

public:
	MusicWheel();
	~MusicWheel();
	void Load();

	virtual void Update( float fDeltaTime );
	virtual void DrawPrimitives();
	void DrawItem( int index );

	virtual void TweenOnScreen(bool changing_sort);
	virtual void TweenOffScreen(bool changing_sort);
	virtual void TweenOnScreen() { TweenOnScreen(false); }
	virtual void TweenOffScreen() { TweenOffScreen(false); }

	void Move(int n);
	bool ChangeSort( SortOrder new_so );	// return true if change successful
	bool NextSort();		// return true if change successful
	void StartRoulette();
	void StartRandom();
	void StartLeap();
	bool IsRouletting() const;
	/* Return true if we're moving fast automatically. */
	int IsMoving() const;

	void NotesChanged( PlayerNumber pn );	// update grade graphics and top score

	void GetItemPosition( float fPosOffsetsFromMiddle, float& fX_out, float& fY_out, float& fZ_out, float& fRotationX_out );
	void SetItemPosition( Actor &item, float fPosOffsetsFromMiddle );

	bool Select();	// return true if this selection ends the screen
	WheelItemType	GetSelectedType()	{ return m_CurWheelItemData[m_iSelection]->m_Type; };
	Song*			GetSelectedSong()	{ return m_CurWheelItemData[m_iSelection]->m_pSong; };
	Course*			GetSelectedCourse()	{ return m_CurWheelItemData[m_iSelection]->m_pCourse; };
	CString			GetSelectedSection(){ return m_CurWheelItemData[m_iSelection]->m_sSectionName; };

	bool WheelIsLocked() { return (m_WheelState == STATE_LOCKED ? true : false); }
	void RebuildMusicWheelItems();

protected:
	void GetSongList(vector<Song*> &arraySongs, SortOrder so, CString sPreferredGroup );
	void BuildWheelItemDatas( vector<WheelItemData> &arrayWheelItems, SortOrder so );
	void SetOpenGroup(CString group, SortOrder so = SORT_INVALID);
	bool SelectSongOrCourse();
	bool SelectSong( Song *p );
	bool SelectCourse( Course *p );
	bool SelectSort( SortOrder so );
	void ChangeMusic(int dist); /* +1 or -1 */

	ScrollBar			m_ScrollBar;
	Sprite				m_sprSelectionOverlay;

	vector<WheelItemData> m_WheelItemDatas[NUM_SORT_ORDERS];
	vector<WheelItemData *> m_CurWheelItemData;
	
	MusicWheelItem	m_MusicWheelItems[MAX_WHEEL_ITEMS];
	
	int					m_iSelection;		// index into m_CurWheelItemData
	CString				m_sExpandedSectionName;
 	SortOrder		m_LastSortOrder;

	int					m_iSwitchesLeftInSpinDown;		
	float				m_fLockedWheelVelocity;
	/* 0 = none; -1 or 1 = up/down */
	int					m_Moving;
	RageTimer			m_MovingSoundTimer;
	float				m_TimeBeforeMovingBegins;
	float				m_SpinSpeed;
	enum WheelState { 
		STATE_SELECTING_MUSIC,
		STATE_FLYING_OFF_BEFORE_NEXT_SORT, 
		STATE_FLYING_ON_AFTER_NEXT_SORT, 
		STATE_TWEENING_ON_SCREEN, 
		STATE_TWEENING_OFF_SCREEN, 
		STATE_WAITING_OFF_SCREEN,
		STATE_ROULETTE_SPINNING,
		STATE_ROULETTE_SLOWING_DOWN,
		STATE_RANDOM_SPINNING,
		STATE_LOCKED,
	};
	WheelState			m_WheelState;
	float				m_fTimeLeftInState;
	float				m_fPositionOffsetFromSelection;

	RageSound m_soundChangeMusic;
	RageSound m_soundChangeSort;
	RageSound m_soundExpand;
	RageSound m_soundLocked;

	bool WheelItemIsVisible(int n);
	void UpdateScrollbar();
};

#endif
