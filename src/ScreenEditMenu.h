#ifndef SCREEN_EDIT_MENU_H
#define SCREEN_EDIT_MENU_H

/*
-----------------------------------------------------------------------------
 Class: ScreenEditMenu

 Desc: See header.

 Copyright (c) 2001-2002 by the person(s) listed below.  All rights reserved.
	Chris Danford
-----------------------------------------------------------------------------
*/

#include "Screen.h"
#include "EditMenu.h"
#include "BitmapText.h"
#include "MenuElements.h"


class ScreenEditMenu : public Screen
{
public:
	ScreenEditMenu( CString sName );
	virtual ~ScreenEditMenu();

	virtual void DrawPrimitives();
	virtual void Input( const DeviceInput& DeviceI, const InputEventType type, const GameInput &GameI, const MenuInput &MenuI, const StyleInput &StyleI );
	virtual void HandleScreenMessage( const ScreenMessage SM );

private:

	void MenuUp( PlayerNumber pn );
	void MenuDown( PlayerNumber pn );
	void MenuLeft( PlayerNumber pn, const InputEventType type );
	void MenuRight( PlayerNumber pn, const InputEventType type );
	void MenuBack( PlayerNumber pn );
	void MenuStart( PlayerNumber pn );

	EditMenu		m_Selector;

	BitmapText		m_textExplanation;

	MenuElements	m_Menu;
};

#endif
