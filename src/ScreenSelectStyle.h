#ifndef ScreenSelectStyle_H
#define ScreenSelectStyle_H
/*
-----------------------------------------------------------------------------
 Class: ScreenSelectStyle

 Desc: 

 Copyright (c) 2001-2002 by the person(s) listed below.  All rights reserved.
	Chris Danford
-----------------------------------------------------------------------------
*/


#include "ScreenSelect.h"
#include "Sprite.h"
#include "BitmapText.h"
#include "RageSound.h"

#define MAX_MODE_CHOICES 30

class ScreenSelectStyle : public ScreenSelect
{
public:
	ScreenSelectStyle( CString sName );

	virtual void MenuLeft( PlayerNumber pn );
	virtual void MenuRight( PlayerNumber pn );
	virtual void MenuStart( PlayerNumber pn );

protected:
	virtual int GetSelectionIndex( PlayerNumber pn );
	virtual void UpdateSelectableChoices();

	void BeforeChange();
	void AfterChange();

	Sprite		m_sprIcon[MAX_MODE_CHOICES];
	// Artists don't make graphics for every single Game, so
	// have a text representation if textures are missing.
	BitmapText	m_textIcon[MAX_MODE_CHOICES];
	Sprite		m_sprPicture[MAX_MODE_CHOICES];
	Sprite		m_sprInfo[MAX_MODE_CHOICES];
	Sprite		m_sprExplanation;
	Sprite		m_sprWarning;
	Sprite		m_sprPremium;
	
	RageSound m_soundChange;

	int m_iSelection;
};

#endif
