#ifndef MUSICWHEELITEM_H
#define MUSICWHEELITEM_H
/*
-----------------------------------------------------------------------------
 Class: MusicWheelItem

 Desc: An item on the music wheel

 Copyright (c) 2001-2002 by the person(s) listed below.  All rights reserved.
	Chris Danford
-----------------------------------------------------------------------------
*/

#include "ActorFrame.h"
#include "GradeDisplay.h"
#include "BitmapText.h"
#include "WheelNotifyIcon.h"
#include "TextBanner.h"
#include "GameConstantsAndTypes.h"
#include "ModeChoice.h"
class Course;
class Song;

struct WheelItemData;

class MusicWheelItem : public ActorFrame
{
public:
	MusicWheelItem();

	virtual void Update( float fDeltaTime );
	virtual void DrawPrimitives();
	virtual void SetZTestMode( ZTestMode mode );
	virtual void SetZWrite( bool b );
	
	void LoadFromWheelItemData( WheelItemData* pWID );
	void RefreshGrades();

	WheelItemData *data;
	float				m_fPercentGray;

	// for TYPE_SECTION and TYPE_ROULETTE
	Sprite				m_sprSectionBar;
	// for TYPE_SECTION
	BitmapText			m_textSectionName;
	// for TYPE_ROULETTE
	BitmapText			m_textRoulette;

	// for a TYPE_MUSIC
	Sprite				m_sprSongBar;
	WheelNotifyIcon		m_WheelNotifyIcon;
	TextBanner			m_TextBanner;
	GradeDisplay		m_GradeDisplay[NUM_PLAYERS];

	// for TYPE_COURSE
	BitmapText			m_textCourse;

	// for TYPE_SORT
	BitmapText			m_textSort;
	ActorFrame m_All;
};

enum WheelItemType 
{
	TYPE_SECTION, 
	TYPE_SONG, 
	TYPE_ROULETTE, 
	TYPE_RANDOM, 
	TYPE_PORTAL, 
	TYPE_COURSE, 
	TYPE_SORT 
};

struct WheelItemData
{
	WheelItemData() {}
	WheelItemData( WheelItemType wit, Song* pSong, CString sSectionName, Course* pCourse, RageColor color, SortOrder so );

	WheelItemType	m_Type;
	CString			m_sSectionName;
	Course*			m_pCourse;
	Song*			m_pSong;
	RageColor		m_color;	// either text color or section background color
	WheelNotifyIcon::Flags  m_Flags;

	// for TYPE_SORT
	CString			m_sLabel;
	ModeChoice		m_Action;
	SortOrder	m_SortOrder;
};

#endif
