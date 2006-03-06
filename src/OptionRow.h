/* OptionRow - One line in ScreenOptions. */

#ifndef OptionRow_H
#define OptionRow_H

#include "ActorFrame.h"
#include "BitmapText.h"
#include "OptionsCursor.h"
#include "OptionIcon.h"
#include "ThemeMetric.h"
#include "AutoActor.h"

class OptionRowHandler;
class GameCommand;

enum SelectType
{
	SELECT_ONE,
	SELECT_MULTIPLE,
	SELECT_NONE,
	NUM_SELECT_TYPES,
	SELECT_INVALID
};
const RString& SelectTypeToString( SelectType pm );
SelectType StringToSelectType( const RString& s );

enum LayoutType
{
	LAYOUT_SHOW_ALL_IN_ROW,
	LAYOUT_SHOW_ONE_IN_ROW,
	NUM_LAYOUT_TYPES,
	LAYOUT_INVALID
};
const RString& LayoutTypeToString( LayoutType pm );
LayoutType StringToLayoutType( const RString& s );

struct OptionRowDefinition
{
	RString m_sName;
	RString m_sExplanationName;
	bool m_bOneChoiceForAllPlayers;
	SelectType m_selectType;
	LayoutType m_layoutType;
	vector<RString> m_vsChoices;
	set<PlayerNumber> m_vEnabledForPlayers;	// only players in this set may change focus to this row
	bool	m_bExportOnChange;
	bool	m_bAllowThemeItems;	// Should be true for dynamic strings.
	bool	m_bAllowThemeTitle;	// Should be true for dynamic strings.
	bool	m_bAllowExplanation;	// if false, ignores ScreenOptions::SHOW_EXPLANATIONS.  Should be true for dynamic strings.
	bool	m_bShowChoicesListOnSelect;

	bool IsEnabledForPlayer( PlayerNumber pn ) const 
	{
		return m_vEnabledForPlayers.find(pn) != m_vEnabledForPlayers.end(); 
	}

	OptionRowDefinition() { Init(); }
	void Init()
	{
		m_sName = "";
		m_sExplanationName = "";
		m_bOneChoiceForAllPlayers = false;
		m_selectType = SELECT_ONE;
		m_layoutType = LAYOUT_SHOW_ALL_IN_ROW;
		m_vsChoices.clear();
		m_vEnabledForPlayers.clear();
		FOREACH_PlayerNumber( pn )
			m_vEnabledForPlayers.insert( pn );
		m_bExportOnChange = false;
		m_bAllowThemeItems = true;
		m_bAllowThemeTitle = true;
		m_bAllowExplanation = true;
		m_bShowChoicesListOnSelect = false;
	}

	OptionRowDefinition( const char *n, bool b, const char *c0=NULL, const char *c1=NULL, const char *c2=NULL, const char *c3=NULL, const char *c4=NULL, const char *c5=NULL, const char *c6=NULL, const char *c7=NULL, const char *c8=NULL, const char *c9=NULL, const char *c10=NULL, const char *c11=NULL, const char *c12=NULL, const char *c13=NULL, const char *c14=NULL, const char *c15=NULL, const char *c16=NULL, const char *c17=NULL, const char *c18=NULL, const char *c19=NULL )
	{
		Init();
		m_sName=n;
		m_bOneChoiceForAllPlayers=b;
#define PUSH( c )	if(c) m_vsChoices.push_back(c);
		PUSH(c0);PUSH(c1);PUSH(c2);PUSH(c3);PUSH(c4);PUSH(c5);PUSH(c6);PUSH(c7);PUSH(c8);PUSH(c9);PUSH(c10);PUSH(c11);PUSH(c12);PUSH(c13);PUSH(c14);PUSH(c15);PUSH(c16);PUSH(c17);PUSH(c18);PUSH(c19);
#undef PUSH
	}
};

class OptionRowType
{
public:
	void Load( const RString &sType );

private:
	RString				m_sType;

	BitmapText			m_textItemParent;
	OptionsCursor			m_UnderlineParent;
	Actor				*m_sprBullet;
	BitmapText			m_textTitle;
	OptionIcon			m_OptionIcon;

	// metrics
	ThemeMetric<float>		BULLET_X;
	ThemeMetric<apActorCommands>	BULLET_ON_COMMAND;
	ThemeMetric<float>		TITLE_X;
	ThemeMetric<apActorCommands>	TITLE_ON_COMMAND;
	ThemeMetric<apActorCommands>	TITLE_GAIN_FOCUS_COMMAND;
	ThemeMetric<apActorCommands>	TITLE_LOSE_FOCUS_COMMAND;
	ThemeMetric<float>		ITEMS_START_X;
	ThemeMetric<float>		ITEMS_END_X;
	ThemeMetric<float>		ITEMS_GAP_X;
	ThemeMetric<float>		ITEMS_MIN_BASE_ZOOM;
	ThemeMetric1D<float>		ITEMS_LONG_ROW_X;
	ThemeMetric<float>		ITEMS_LONG_ROW_SHARED_X;
	ThemeMetric<apActorCommands>	ITEMS_ON_COMMAND;
	ThemeMetric<apActorCommands>	ITEM_GAIN_FOCUS_COMMAND;
	ThemeMetric<apActorCommands>	ITEM_LOSE_FOCUS_COMMAND;
	ThemeMetric1D<float>		ICONS_X;
	ThemeMetric<apActorCommands>	ICONS_ON_COMMAND;
	ThemeMetric<RageColor>		COLOR_SELECTED;
	ThemeMetric<RageColor>		COLOR_NOT_SELECTED;
	ThemeMetric<RageColor>		COLOR_DISABLED;
	ThemeMetric<bool>		CAPITALIZE_ALL_OPTION_NAMES;
	ThemeMetric<float>		TWEEN_SECONDS;
	ThemeMetric<bool>		SHOW_BPM_IN_SPEED_TITLE;
	ThemeMetric<bool>		SHOW_OPTION_ICONS;
	ThemeMetric<bool>		SHOW_UNDERLINES;

	friend class OptionRow;
};

class OptionRow : public ActorFrame
{
public:
	OptionRow( const OptionRowType *pType );
	~OptionRow();
	void Update( float fDeltaTime );

	void Clear();
	void LoadNormal( OptionRowHandler *pHand, bool bFirstItemGoesDown );
	void LoadExit();

	void SetOptionIcon( PlayerNumber pn, const RString &sText, GameCommand &gc );

	void ImportOptions( const vector<PlayerNumber> &vpns );
	int ExportOptions( const vector<PlayerNumber> &vpns, bool bRowHasFocus[NUM_PLAYERS] );

	void InitText();
	void AfterImportOptions( PlayerNumber pn );

	void ChoicesChanged();
	void PositionUnderlines( PlayerNumber pn );
	void PositionIcons( PlayerNumber pn );
	void UpdateText( PlayerNumber pn );
	void SetRowHasFocus( PlayerNumber pn, bool bRowHasFocus );
	void UpdateEnabledDisabled();

	int GetOneSelection( PlayerNumber pn, bool bAllowFail=false ) const;
	int GetOneSharedSelection( bool bAllowFail=false ) const;
	void SetOneSelection( PlayerNumber pn, int iChoice );
	void SetOneSharedSelection( int iChoice );
	void SetOneSharedSelectionIfPresent( const RString &sChoice );

	int GetChoiceInRowWithFocus( PlayerNumber pn ) const;
	int GetChoiceInRowWithFocusShared() const;
	void SetChoiceInRowWithFocus( PlayerNumber pn, int iChoice );
	void ResetFocusFromSelection( PlayerNumber pn );

	bool GetSelected( PlayerNumber pn, int iChoice ) const;
	void SetSelected( PlayerNumber pn, int iChoice, bool b );

	enum RowType
	{
		RowType_Normal,
		RowType_Exit
	};
	const OptionRowDefinition &GetRowDef() const;
	OptionRowDefinition &GetRowDef();
	RowType GetRowType() const { return m_RowType; }
	const OptionRowHandler *GetHandler() const { return m_pHand; }

	const BitmapText &GetTextItemForRow( PlayerNumber pn, int iChoiceOnRow ) const;
	void GetWidthXY( PlayerNumber pn, int iChoiceOnRow, int &iWidthOut, int &iXOut, int &iYOut ) const;

	// ScreenOptions calls positions m_FrameDestination, then m_Frame tween to that same TweenState.
	unsigned GetTextItemsSize() const { return m_textItems.size(); }
	bool GetFirstItemGoesDown() const { return m_bFirstItemGoesDown; }

	void PrepareItemText( RString &s ) const;
	RString OptionTitle( RString s ) const;

	void SetExitText( RString sExitText );

	void Reload();

	//
	// Messages
	//
	virtual void HandleMessage( const RString& sMessage );

protected:
	RString GetRowTitle() const;

	const OptionRowType		*m_pParentType;
	RowType				m_RowType;
	OptionRowHandler*		m_pHand;

	ActorFrame			m_Frame;

	vector<BitmapText *>		m_textItems;			// size depends on m_bRowIsLong and which players are joined
	vector<OptionsCursor *>		m_Underline[NUM_PLAYERS];	// size depends on m_bRowIsLong and which players are joined

	Actor				*m_sprBullet;
	BitmapText			*m_textTitle;
	OptionIcon			*m_OptionIcons[NUM_PLAYERS];

	bool				m_bFirstItemGoesDown;
	bool				m_bRowHasFocus[NUM_PLAYERS];

	int				m_iChoiceInRowWithFocus[NUM_PLAYERS];	// this choice has input focus
	// Only one will true at a time if m_pHand->m_Def.bMultiSelect
	vector<bool>			m_vbSelected[NUM_PLAYERS];	// size = m_pHand->m_Def.choices.size()
	Actor::TweenState m_tsDestination;	// this should approach m_tsDestination.

public:
	void SetDestination( Actor::TweenState &ts, bool bTween );
};

#endif

/*
 * (c) 2001-2004 Chris Danford
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
