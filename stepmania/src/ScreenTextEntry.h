/*
-----------------------------------------------------------------------------
 Class: ScreenTextEntry

 Desc: Displays a text entry box over the top of another screen.  Must use by calling
	SCREENMAN->AddScreenToTop( new ScreenTextEntry(...) );

 Copyright (c) 2001-2002 by the person(s) listed below.  All rights reserved.
	Chris Danford
-----------------------------------------------------------------------------
*/

#include "Screen.h"
#include "BitmapText.h"
#include "Transition.h"
#include "Quad.h"
#include "RandomSample.h"




class ScreenTextEntry : public Screen
{
public:
	ScreenTextEntry( CString sName, CString sQuestion, CString sInitialAnswer, void(*OnOK)(CString sAnswer) = NULL, void(*OnCanel)() = NULL );

	virtual void Update( float fDeltaTime );
	virtual void DrawPrimitives();
	virtual void Input( const DeviceInput& DeviceI, const InputEventType type, const GameInput &GameI, const MenuInput &MenuI, const StyleInput &StyleI );
	virtual void HandleScreenMessage( const ScreenMessage SM );

	static CString s_sLastAnswer;
	static bool s_bCancelledLast;

protected:
	virtual void MenuLeft( PlayerNumber pn );
	virtual void MenuRight( PlayerNumber pn );
	virtual void MenuStart( PlayerNumber pn );
	virtual void MenuBack( PlayerNumber pn );

	void UpdateText();

	BGAnimation		m_Background;
	BitmapText		m_textQuestion;
	Quad			m_rectAnswerBox;
	wstring			m_sAnswer;
	BitmapText		m_textAnswer;
	void(*m_pOnOK)( CString sAnswer );
	void(*m_pOnCancel)();
	bool			m_bCancelled;
	Transition		m_In;
	Transition		m_Out;
};

