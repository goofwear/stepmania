#ifndef SCREENOPTIONS_H
#define SCREENOPTIONS_H
/*
-----------------------------------------------------------------------------
 File: ScreenOptions.h

 Desc: A grid of options, and the selected option is drawn with a highlight rectangle.

 Copyright (c) 2001-2002 by the person(s) listed below.  All rights reserved.
	Chris Danford
-----------------------------------------------------------------------------
*/


#include "Screen.h"
#include "Sprite.h"
#include "BitmapText.h"
#include "RandomSample.h"
#include "Quad.h"
#include "MenuElements.h"
#include "OptionsCursor.h"
#include "OptionIcon.h"
#include "DualScrollBar.h"


struct OptionRowData
{
	CString name;
	bool bOneChoiceForAllPlayers;
	bool bMultiSelect;
	vector<CString> choices;

	OptionRowData(): name(""), bOneChoiceForAllPlayers(false), bMultiSelect(false) { }

	OptionRowData( const char *n, int b, const char *c0=NULL, const char *c1=NULL, const char *c2=NULL, const char *c3=NULL, const char *c4=NULL, const char *c5=NULL, const char *c6=NULL, const char *c7=NULL, const char *c8=NULL, const char *c9=NULL, const char *c10=NULL, const char *c11=NULL, const char *c12=NULL, const char *c13=NULL, const char *c14=NULL, const char *c15=NULL, const char *c16=NULL, const char *c17=NULL, const char *c18=NULL, const char *c19=NULL )
	{
		name = n;
		bOneChoiceForAllPlayers = !!b;
		bMultiSelect = false;
#define PUSH( c )	if(c) choices.push_back(c);
		PUSH(c0);PUSH(c1);PUSH(c2);PUSH(c3);PUSH(c4);PUSH(c5);PUSH(c6);PUSH(c7);PUSH(c8);PUSH(c9);PUSH(c10);PUSH(c11);PUSH(c12);PUSH(c13);PUSH(c14);PUSH(c15);PUSH(c16);PUSH(c17);PUSH(c18);PUSH(c19);
#undef PUSH
	}
};

enum InputMode 
{ 
	INPUTMODE_INDIVIDUAL, 	// each player controls their own cursor
	INPUTMODE_TOGETHER		// both players control the same cursor
};


class ScreenOptions : public Screen
{
public:
	ScreenOptions( CString sClassName );
	void Init( InputMode im, OptionRowData OptionRowData[], int iNumOptionLines );
	virtual ~ScreenOptions();
	virtual void Update( float fDeltaTime );
	virtual void DrawPrimitives();
	virtual void Input( const DeviceInput& DeviceI, const InputEventType type, const GameInput &GameI, const MenuInput &MenuI, const StyleInput &StyleI );
	virtual void HandleScreenMessage( const ScreenMessage SM );

protected:
	virtual void ImportOptions() = 0;
	virtual void ExportOptions() = 0;
	void InitOptionsText();
	void GetWidthXY( PlayerNumber pn, int iRow, int iChoiceOnRow, int &iWidthOut, int &iXOut, int &iYOut );
	CString GetExplanationText( int row ) const;
	CString GetExplanationTitle( int row ) const;
	BitmapText &GetTextItemForRow( PlayerNumber pn, int iRow, int iChoiceOnRow );
	void PositionUnderlines();
	void PositionIcons();
	virtual void RefreshIcons();
	void PositionCursors();
	void PositionItems();
	void TweenCursor( PlayerNumber player_no );
	void UpdateText( PlayerNumber player_no, int row );
	void UpdateEnabledDisabled();
	virtual void OnChange( PlayerNumber pn );

	virtual void MenuBack( PlayerNumber pn );
	virtual void MenuStart( PlayerNumber pn, const InputEventType type );

	void StartGoToNextState();

	virtual void GoToNextState() = 0;
	virtual void GoToPrevState() = 0;

	void MenuLeft( PlayerNumber pn, const InputEventType type ) { ChangeValueInRow(pn,-1,type != IET_FIRST_PRESS); }
	void MenuRight( PlayerNumber pn, const InputEventType type ) { ChangeValueInRow(pn,+1,type != IET_FIRST_PRESS); }
	void ChangeValueInRow( PlayerNumber pn, int iDelta, bool Repeat );
	void MenuUp( PlayerNumber pn, const InputEventType type ) { MoveRow( pn, -1, type != IET_FIRST_PRESS ); }
	void MenuDown( PlayerNumber pn, const InputEventType type ) { MoveRow( pn, +1, type != IET_FIRST_PRESS ); }
	void MoveRow( PlayerNumber pn, int dir, bool Repeat );

	/* Returns -1 if on a row with no OptionRowData (eg. EXIT). */
	int GetCurrentRow(PlayerNumber pn = PLAYER_1) const;

	MenuElements	m_Menu;

protected:	// derived classes need access to these
	void LoadOptionIcon( PlayerNumber pn, int iRow, CString sText );
	enum Navigation { NAV_THREE_KEY, NAV_THREE_KEY_MENU, NAV_FIVE_KEY, NAV_FIRST_CHOICE_GOES_DOWN };
	void SetNavigation( Navigation nav ) { m_OptionsNavigation = nav; }

protected:
	/* Map menu lines to m_OptionRow entries. */
	struct Row
	{
		Row();
		~Row();
		OptionRowData				m_RowDef;
		enum { ROW_NORMAL, ROW_EXIT } Type;
		vector<BitmapText *>	m_textItems;				// size depends on m_bRowIsLong and which players are joined
		vector<OptionsCursor *>	m_Underline[NUM_PLAYERS];	// size depends on m_bRowIsLong and which players are joined
		Sprite					m_sprBullet;
		BitmapText				m_textTitle;
		OptionIcon				m_OptionIcons[NUM_PLAYERS];

		float m_fY;
		bool m_bRowIsLong;	// goes off edge of screen
		bool m_bHidden; // currently off screen

		int m_iChoiceWithFocus[NUM_PLAYERS];	// this choice has input focus

		// Only one will true at a time if m_RowDef.bMultiSelect
		vector<bool> m_vbSelected[NUM_PLAYERS];	// size = m_RowDef.choices.size().
		int GetOneSelection( PlayerNumber pn ) const
		{
			for( unsigned i=0; i<(unsigned)m_vbSelected[pn].size(); i++ )
				if( m_vbSelected[pn][i] )
					return i;
			ASSERT(0);	// shouldn't call this if not expecting one to be selected
			return -1;
		}
		int GetOneSharedSelection() const
		{
			return GetOneSelection( (PlayerNumber)0 );
		}
		void SetOneSelection( PlayerNumber pn, int iChoice )
		{
			for( unsigned i=0; i<(unsigned)m_vbSelected[pn].size(); i++ )
				m_vbSelected[pn][i] = false;
			m_vbSelected[pn][iChoice] = true;
		}
		void SetOneSharedSelection( int iChoice )
		{
			SetOneSelection( (PlayerNumber)0, iChoice );
		}
	};
	vector<Row*>		m_Rows;

	Navigation		m_OptionsNavigation;

	int				m_iCurrentRow[NUM_PLAYERS];

	InputMode		m_InputMode;

	ActorFrame		m_framePage;
	AutoActor		m_sprPage;
	AutoActor		m_sprFrame;

	OptionsCursor	m_Highlight[NUM_PLAYERS];
	Sprite			m_sprLineHighlight[NUM_PLAYERS];

	BitmapText		m_textPlayerName[NUM_PLAYERS];
	BitmapText		m_textExplanation[NUM_PLAYERS];
	DualScrollBar	m_ScrollBar;

	AutoActor		m_sprMore;
	bool			m_bMoreShown, m_bWasOnExit[NUM_PLAYERS];
	
	// TRICKY: People hold Start to get to PlayerOptions, then 
	// the repeat events cause them to zip to the bottom.  So, ignore
	// Start repeat events until we've seen one first pressed event.
	bool			m_bGotAtLeastOneStartPressed[NUM_PLAYERS];

	RageSound		m_SoundChangeCol;
	RageSound		m_SoundNextRow;
	RageSound		m_SoundPrevRow;
	RageSound		m_SoundStart;
	RageSound		m_SoundToggleOn;
	RageSound		m_SoundToggleOff;
};


#endif
