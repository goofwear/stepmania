/*
-----------------------------------------------------------------------------
 Class: ScreenMiniMenu

 Desc: Displays a prompt over the top of another screen.  Must use by calling
	SCREENMAN->AddScreenToTop( new ScreenMiniMenu(...) );

 Copyright (c) 2001-2002 by the person(s) listed below.  All rights reserved.
	Chris Danford
-----------------------------------------------------------------------------
*/

#include "Screen.h"
#include "BitmapText.h"
#include "Transition.h"
#include "Quad.h"
#include "RandomSample.h"


#define MAX_MENU_ROWS  40


struct MenuRow
{
	CString name;
	bool	enabled;
	int		defaultChoice;
	vector<CString> choices;

	MenuRow()
	{
		enabled = true;
		defaultChoice = 0;
	}

	MenuRow( const char * n, bool e, int d=0, const char * c0=NULL, const char * c1=NULL, const char * c2=NULL, const char * c3=NULL, const char * c4=NULL, const char * c5=NULL, const char * c6=NULL, const char * c7=NULL, const char * c8=NULL, const char * c9=NULL, const char * c10=NULL, const char * c11=NULL, const char * c12=NULL, const char * c13=NULL, const char * c14=NULL, const char * c15=NULL, const char * c16=NULL, const char * c17=NULL, const char * c18=NULL, const char * c19=NULL, const char * c20=NULL, const char * c21=NULL, const char * c22=NULL, const char * c23=NULL, const char * c24=NULL )
	{
		name = n;
		enabled = e;
		defaultChoice = d;
#define PUSH( c )	if(c!=NULL) choices.push_back(c);
		PUSH(c0);PUSH(c1);PUSH(c2);PUSH(c3);PUSH(c4);PUSH(c5);PUSH(c6);PUSH(c7);PUSH(c8);PUSH(c9);PUSH(c10);PUSH(c11);PUSH(c12);PUSH(c13);PUSH(c14);PUSH(c15);PUSH(c16);PUSH(c17);PUSH(c18);PUSH(c19);PUSH(c20);PUSH(c21);PUSH(c22);PUSH(c23);PUSH(c24);
#undef PUSH
//		printf( "choices.size = %u", choices.size() );
	}
};


struct Menu
{
	CString title;
	vector<MenuRow> rows;

	Menu() {}

	Menu( CString t, MenuRow r0, MenuRow r1=MenuRow(), MenuRow r2=MenuRow(), MenuRow r3=MenuRow(), MenuRow r4=MenuRow(), MenuRow r5=MenuRow(), MenuRow r6=MenuRow(), MenuRow r7=MenuRow(), MenuRow r8=MenuRow(), MenuRow r9=MenuRow(), MenuRow r10=MenuRow(), MenuRow r11=MenuRow(), MenuRow r12=MenuRow(), MenuRow r13=MenuRow(), MenuRow r14=MenuRow(), MenuRow r15=MenuRow(), MenuRow r16=MenuRow(), MenuRow r17=MenuRow(), MenuRow r18=MenuRow(), MenuRow r19=MenuRow(), MenuRow r20=MenuRow(), MenuRow r21=MenuRow() )
	{
		title = t;
#define PUSH( r )	if(r.name!="" || !r.choices.empty()) rows.push_back(r);
		PUSH(r0);PUSH(r1);PUSH(r2);PUSH(r3);PUSH(r4);PUSH(r5);PUSH(r6);PUSH(r7);PUSH(r8);PUSH(r9);PUSH(r10);PUSH(r11);PUSH(r12);PUSH(r13);PUSH(r14);PUSH(r15);PUSH(r16);PUSH(r17);PUSH(r18);PUSH(r19);PUSH(r20);PUSH(r21);
#undef PUSH
	}
};


class ScreenMiniMenu : public Screen
{
public:
	ScreenMiniMenu( CString sName );
	ScreenMiniMenu( Menu* pDef, ScreenMessage SM_SendOnOK, ScreenMessage SM_SendOnCancel );

	virtual void Update( float fDeltaTime );
	virtual void DrawPrimitives();
	virtual void Input( const DeviceInput& DeviceI, const InputEventType type, const GameInput &GameI, const MenuInput &MenuI, const StyleInput &StyleI );
	virtual void HandleScreenMessage( const ScreenMessage SM );

protected:
	void MenuUp( PlayerNumber pn, const InputEventType type );
	void MenuDown( PlayerNumber pn, const InputEventType type );
	void MenuLeft( PlayerNumber pn, const InputEventType type );
	void MenuRight( PlayerNumber pn, const InputEventType type );
	void MenuBack( PlayerNumber pn );
	void MenuStart( PlayerNumber pn, const InputEventType type );

	int GetGoUpSpot();		// return -1 if can't go up
	int GetGoDownSpot();	// return -1 if can't go down
	bool CanGoLeft();
	bool CanGoRight();


	void BeforeLineChanged();
	void AfterLineChanged();
	void AfterAnswerChanged();

	BGAnimation			m_Background;
	Menu				m_Def;
	BitmapText			m_textTitle;
	BitmapText			m_textLabel[MAX_MENU_ROWS];
	BitmapText			m_textAnswer[MAX_MENU_ROWS];
	int					m_iCurLine;
	int					m_iCurAnswers[MAX_MENU_ROWS];
	ScreenMessage		m_SMSendOnOK, m_SMSendOnCancel;
	Transition		m_In;
	Transition		m_Out;

public:
	static int	s_iLastLine;
	static int	s_iLastAnswers[MAX_MENU_ROWS];

};
