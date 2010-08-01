#include "global.h"
#include "OptionRow.h"
#include "RageUtil.h"
#include "RageLog.h"
#include "Foreach.h"
#include "OptionRowHandler.h"
#include "CommonMetrics.h"
#include "GameState.h"
#include "song.h"
#include "Course.h"
#include "Style.h"
#include "ActorUtil.h"

static const char *SelectTypeNames[] = {
	"SelectOne",
	"SelectMultiple",
	"SelectNone",
};
XToString( SelectType, NUM_SELECT_TYPES );
StringToX( SelectType );

static const char *LayoutTypeNames[] = {
	"ShowAllInRow",
	"ShowOneInRow",
};
XToString( LayoutType, NUM_LAYOUT_TYPES );
StringToX( LayoutType );

const RString NEXT_ROW_NAME = "NextRow";
const RString EXIT_NAME = "Exit";

void OptionRow::PrepareItemText( RString &s ) const
{
	if( s == "" )
		return;
	bool bTheme = false;
	
	// HACK: Always theme the NEXT_ROW and EXIT items.
	if( s == NEXT_ROW_NAME )			bTheme = true;
	if( s == EXIT_NAME )				bTheme = true;
	if( m_pHand->m_Def.m_bAllowThemeItems )	bTheme = true;

	// Items beginning with a pipe mean "don't theme".
	// This allows us to disable theming on a per-choice basis for choice names that are just a number
	// and don't need to be localized.
	if( s[0] == '|' )
	{
		s.erase( s.begin() );
		bTheme = false;
	}

	if( bTheme ) 
		s = CommonMetrics::LocalizeOptionItem( s, false ); 
	if( m_pParentType->CAPITALIZE_ALL_OPTION_NAMES )
		s.MakeUpper(); 
}

RString OptionRow::OptionTitle( RString s ) const
{
	bool bTheme = false;
	
	// HACK: Always theme the NEXT_ROW and EXIT items, even if metrics says not to theme.
	if( m_pHand->m_Def.m_bAllowThemeTitle )
		bTheme = true;

	if( s.empty() )
		return s;

	return bTheme ? THEME->GetString("OptionTitles",s) : s;
}

RString ITEMS_LONG_ROW_X_NAME( size_t p )		{ return ssprintf("ItemsLongRowP%dX",int(p+1)); }
RString ICONS_X_NAME( size_t p )				{ return ssprintf("IconsP%dX",int(p+1)); }

OptionRow::OptionRow( const OptionRowType *pSource )
{
	m_pParentType = pSource;
	m_pHand = NULL;

	m_textTitle = NULL;
	ZERO( m_OptionIcons );

	Clear();
	this->AddChild( &m_Frame );

	m_tsDestination.Init();
}

OptionRow::~OptionRow()
{
	Clear();
}

void OptionRow::Clear()
{
	ActorFrame::RemoveAllChildren();

	FOREACH_PlayerNumber( p )
		m_vbSelected[p].clear();

	m_Frame.DeleteAllChildren();
	m_textItems.clear();
	FOREACH_PlayerNumber( p )
		m_Underline[p].clear();

	if( m_pHand != NULL )
	{
		FOREACH_CONST( RString, m_pHand->m_vsReloadRowMessages, m )
			MESSAGEMAN->Unsubscribe( this, *m );
	}
	SAFE_DELETE( m_pHand );

	m_bFirstItemGoesDown = false;
	ZERO( m_bRowHasFocus );
	ZERO( m_iChoiceInRowWithFocus );
}

void OptionRowType::Load( const RString &sType )
{
	m_sType = sType;

	BULLET_X   			.Load(sType,"BulletX");
	BULLET_ON_COMMAND   		.Load(sType,"BulletOnCommand");
	TITLE_X				.Load(sType,"TitleX");
	TITLE_ON_COMMAND		.Load(sType,"TitleOnCommand");
	TITLE_GAIN_FOCUS_COMMAND	.Load(sType,"TitleGainFocusCommand");
	TITLE_LOSE_FOCUS_COMMAND	.Load(sType,"TitleLoseFocusCommand");
	ITEMS_START_X			.Load(sType,"ItemsStartX");
	ITEMS_END_X			.Load(sType,"ItemsEndX");
	ITEMS_GAP_X			.Load(sType,"ItemsGapX");
	ITEMS_MIN_BASE_ZOOM		.Load(sType,"ItemsMinBaseZoom");
	ITEMS_LONG_ROW_X		.Load(sType,ITEMS_LONG_ROW_X_NAME,NUM_PLAYERS);
	ITEMS_LONG_ROW_SHARED_X		.Load(sType,"ItemsLongRowSharedX");
	ITEMS_ON_COMMAND		.Load(sType,"ItemsOnCommand");
	ITEM_GAIN_FOCUS_COMMAND		.Load(sType,"ItemGainFocusCommand");
	ITEM_LOSE_FOCUS_COMMAND		.Load(sType,"ItemLoseFocusCommand");
	ICONS_X				.Load(sType,ICONS_X_NAME,NUM_PLAYERS);
	ICONS_ON_COMMAND		.Load(sType,"IconsOnCommand");
	COLOR_SELECTED			.Load(sType,"ColorSelected");
	COLOR_NOT_SELECTED		.Load(sType,"ColorNotSelected");
	COLOR_DISABLED			.Load(sType,"ColorDisabled");
	CAPITALIZE_ALL_OPTION_NAMES	.Load(sType,"CapitalizeAllOptionNames");
	TWEEN_SECONDS			.Load(sType,"TweenSeconds");
	SHOW_BPM_IN_SPEED_TITLE		.Load(sType,"ShowBpmInSpeedTitle");
	SHOW_OPTION_ICONS		.Load(sType,"ShowOptionIcons");
	SHOW_UNDERLINES			.Load(sType,"ShowUnderlines");

	m_textItemParent.LoadFromFont( THEME->GetPathF(sType,"item") );
	if( SHOW_UNDERLINES )
		m_UnderlineParent.Load( sType, OptionsCursor::underline );
	m_textTitle.LoadFromFont( THEME->GetPathF(sType,"title") );
	m_sprBullet = ActorUtil::MakeActor( THEME->GetPathG(sType,"bullet") );
	if( SHOW_OPTION_ICONS )
		m_OptionIcon.Load( THEME->GetMetric(sType,"OptionIconType") );
}

void OptionRow::LoadNormal( OptionRowHandler *pHand, bool bFirstItemGoesDown )
{
	m_RowType = OptionRow::RowType_Normal;
	m_pHand = pHand;
	m_bFirstItemGoesDown = bFirstItemGoesDown;

	FOREACH_CONST( RString, m_pHand->m_vsReloadRowMessages, m )
		MESSAGEMAN->Subscribe( this, *m );

	ChoicesChanged();
}

void OptionRow::LoadExit()
{
	m_RowType = OptionRow::RowType_Exit;
	OptionRowHandler *pHand = OptionRowHandlerUtil::MakeNull();
	pHand->m_Def.m_selectType  = SELECT_NONE;
	pHand->m_Def.m_sName = EXIT_NAME;
	pHand->m_Def.m_vsChoices.push_back( "Exit" );
	pHand->m_Def.m_layoutType = LAYOUT_SHOW_ONE_IN_ROW;
	pHand->m_Def.m_bOneChoiceForAllPlayers = true;
	m_pHand = pHand;

	ChoicesChanged();
}

void OptionRow::ChoicesChanged()
{
	ASSERT( !m_pHand->m_Def.m_vsChoices.empty() );

	// Remove the NextRow marker before reloading choices
	if( m_pHand->m_Def.m_vsChoices[0] == NEXT_ROW_NAME )
	{
		m_pHand->m_Def.m_vsChoices.erase( m_pHand->m_Def.m_vsChoices.begin() );
		FOREACH_PlayerNumber( p )
			m_vbSelected[p].erase( m_vbSelected[p].begin() );
	}

	FOREACH_PlayerNumber( p )
	{
		vector<bool> &vbSelected = m_vbSelected[p];
		vbSelected.resize( 0 );
		vbSelected.resize( m_pHand->m_Def.m_vsChoices.size(), false );
		
		// set select the first item if a SELECT_ONE row
		if( vbSelected.size() && m_pHand->m_Def.m_selectType == SELECT_ONE )
			vbSelected[0] = true;
	}

	// TRICKY:  Insert a down arrow as the first choice in the row.
	if( m_bFirstItemGoesDown )
	{
		m_pHand->m_Def.m_vsChoices.insert( m_pHand->m_Def.m_vsChoices.begin(), NEXT_ROW_NAME );
		FOREACH_PlayerNumber( p )
			m_vbSelected[p].insert( m_vbSelected[p].begin(), false );
	}

	InitText();

	/* When choices change, the old focus position is meaningless; reset it. */
	FOREACH_PlayerNumber( p )
		SetChoiceInRowWithFocus( p, 0 );

	m_textTitle->SetText( GetRowTitle() );
}

RString OptionRow::GetRowTitle() const
{
	RString sLineName = m_pHand->m_Def.m_sName;
	RString sTitle = OptionTitle(sLineName);

	// HACK: tack the BPM onto the name of the speed line
	if( sLineName.CompareNoCase("speed")==0 )
	{
		bool bShowBpmInSpeedTitle = m_pParentType->SHOW_BPM_IN_SPEED_TITLE;

		if( GAMESTATE->m_pCurCourse )
		{
			const Trail* pTrail = GAMESTATE->m_pCurTrail[GAMESTATE->m_MasterPlayerNumber];
			ASSERT( pTrail != NULL );
			const int iNumCourseEntries = pTrail->m_vEntries.size();
			if( iNumCourseEntries > CommonMetrics::MAX_COURSE_ENTRIES_BEFORE_VARIOUS )
				bShowBpmInSpeedTitle = false;
		}

		if( bShowBpmInSpeedTitle )
		{
			DisplayBpms bpms;
			if( GAMESTATE->m_pCurSong )
			{
				const Song* pSong = GAMESTATE->m_pCurSong;
				pSong->GetDisplayBpms( bpms );
			}
			else if( GAMESTATE->m_pCurCourse )
			{
				const Course *pCourse = GAMESTATE->m_pCurCourse;
				StepsType st = GAMESTATE->GetCurrentStyle()->m_StepsType;
				const Trail* pTrail = pCourse->GetTrail( st );
				ASSERT( pTrail );
				pTrail->GetDisplayBpms( bpms );
			}

			if( bpms.IsSecret() )
				sTitle += ssprintf( " (??" "?)" ); /* split so gcc doesn't think this is a trigraph */
			else if( bpms.BpmIsConstant() )
				sTitle += ssprintf( " (%.0f)", bpms.GetMin() );
			else
				sTitle += ssprintf( " (%.0f-%.0f)", bpms.GetMin(), bpms.GetMax() );
		}
	}

	return sTitle;
}

/* Set up text, underlines and titles for options.  This can be called
 * as soon as m_pHand->m_Def is available. */
void OptionRow::InitText()
{
	/* If we have elements already, we're being updated from a new set of options.
	 * Delete the old ones. */
	m_Frame.DeleteAllChildren();
	m_textItems.clear();
	FOREACH_PlayerNumber( p )
		m_Underline[p].clear();

	m_textTitle = new BitmapText( m_pParentType->m_textTitle );
	m_Frame.AddChild( m_textTitle );

	m_sprBullet = m_pParentType->m_sprBullet->Copy();
	m_sprBullet->SetDrawOrder(-1); // under title
	m_Frame.AddChild( m_sprBullet );

	if( m_pParentType->SHOW_OPTION_ICONS )
	{
		switch( m_RowType )
		{
		case RowType_Normal:
			FOREACH_PlayerNumber( p )
			{
				m_OptionIcons[p] = new OptionIcon( m_pParentType->m_OptionIcon );
				m_OptionIcons[p]->SetDrawOrder(-1); // under title
				m_OptionIcons[p]->RunCommands( m_pParentType->ICONS_ON_COMMAND );
				
				m_Frame.AddChild( m_OptionIcons[p] );

				GameCommand gc;
				SetOptionIcon( p, "", gc );
			}
			break;
		case RowType_Exit:
			break;
		}
	}

	// If the items will go off the edge of the screen, then force LAYOUT_SHOW_ONE_IN_ROW.
	float fBaseZoom = 1.0f;
	{
		BitmapText bt( m_pParentType->m_textItemParent );
		bt.RunCommands( m_pParentType->ITEMS_ON_COMMAND );

		/* Figure out the width of the row. */
		float fWidth = 0;
		for( unsigned c=0; c<m_pHand->m_Def.m_vsChoices.size(); c++ )
		{
			RString sText = m_pHand->m_Def.m_vsChoices[c];
			PrepareItemText( sText );
			bt.SetText( sText );
			
			fWidth += bt.GetZoomedWidth();
			
			if( c != m_pHand->m_Def.m_vsChoices.size()-1 )
				fWidth += m_pParentType->ITEMS_GAP_X;
		}

		/* Try to fit everything on one line. */
		float fTotalWidth = m_pParentType->ITEMS_END_X - m_pParentType->ITEMS_START_X;
		if( fWidth > fTotalWidth ) 
		{
			float fPossibleBaseZoom = fTotalWidth / fWidth;
			if( fPossibleBaseZoom >= m_pParentType->ITEMS_MIN_BASE_ZOOM )
				fBaseZoom = fPossibleBaseZoom;
			else
				m_pHand->m_Def.m_layoutType = LAYOUT_SHOW_ONE_IN_ROW;
		}
	}

	//
	// load m_textItems
	//
	switch( m_pHand->m_Def.m_layoutType )
	{
	case LAYOUT_SHOW_ONE_IN_ROW:
		// init text
		FOREACH_PlayerNumber( p )
		{
			BitmapText *pText = new BitmapText( m_pParentType->m_textItemParent );
			m_textItems.push_back( pText );

			pText->RunCommands( m_pParentType->ITEMS_ON_COMMAND );
			pText->SetShadowLength( 0 );

			if( m_pHand->m_Def.m_bOneChoiceForAllPlayers )
			{
				pText->SetX( m_pParentType->ITEMS_LONG_ROW_SHARED_X );
				break;	// only initialize one item since it's shared
			}
			else
			{
				pText->SetX( m_pParentType->ITEMS_LONG_ROW_X.GetValue(p) );
			}

			// Set the text now, so SetWidthXY below is correct.
			UpdateText( p );

			// init underlines
			if( m_pParentType->SHOW_UNDERLINES && GetRowType() != OptionRow::RowType_Exit )
			{
				OptionsCursor *pCursor = new OptionsCursor( m_pParentType->m_UnderlineParent );
				m_Underline[p].push_back( pCursor );

				pCursor->Set( p );
				int iWidth, iX, iY;
				GetWidthXY( p, 0, iWidth, iX, iY );
				pCursor->SetX( float(iX) );
				pCursor->SetBarWidth( iWidth );
			}
		}
		break;

	case LAYOUT_SHOW_ALL_IN_ROW:
		{
			float fX = m_pParentType->ITEMS_START_X;
			for( unsigned c=0; c<m_pHand->m_Def.m_vsChoices.size(); c++ )
			{
				// init text
				BitmapText *bt = new BitmapText( m_pParentType->m_textItemParent );
				m_textItems.push_back( bt );
				RString sText = m_pHand->m_Def.m_vsChoices[c];
				PrepareItemText( sText );
				bt->SetText( sText );
				bt->SetBaseZoomX( fBaseZoom );
				bt->RunCommands( m_pParentType->ITEMS_ON_COMMAND );
				bt->SetShadowLength( 0 );

				// set the X position of each item in the line
				float fItemWidth = bt->GetZoomedWidth();
				fX += fItemWidth/2;
				bt->SetX( fX );

				// init underlines
				if( m_pParentType->SHOW_UNDERLINES )
				{
					FOREACH_PlayerNumber( p )
					{
						OptionsCursor *ul = new OptionsCursor( m_pParentType->m_UnderlineParent );
						m_Underline[p].push_back( ul );
						ul->Set( p );
						ul->SetX( fX );
						ul->SetBarWidth( int(fItemWidth) );
					}
				}

				fX += fItemWidth/2 + m_pParentType->ITEMS_GAP_X * fBaseZoom;
			}
		}
		break;

	default:
		ASSERT(0);
	}

	for( unsigned c=0; c<m_textItems.size(); c++ )
		m_Frame.AddChild( m_textItems[c] );
	FOREACH_PlayerNumber( p )
		for( unsigned c=0; c<m_Underline[p].size(); c++ )
			m_Frame.AddChild( m_Underline[p][c] );

	// This is set in OptionRow::AfterImportOptions, so if we're reused with a different
	// song selected, SHOW_BPM_IN_SPEED_TITLE will show the new BPM.
	//m_textTitle->SetText( GetRowTitle() );
	m_textTitle->SetX( m_pParentType->TITLE_X );
	m_textTitle->RunCommands( m_pParentType->TITLE_ON_COMMAND );

	switch( GetRowType() )
	{
	case OptionRow::RowType_Normal:
		m_sprBullet->SetX( m_pParentType->BULLET_X );
		m_sprBullet->RunCommands( m_pParentType->BULLET_ON_COMMAND );
		break;
	case OptionRow::RowType_Exit:
		m_sprBullet->SetHidden( true );
		break;
	}

	m_Frame.SortByDrawOrder();
	this->SortByDrawOrder();
}

/* After importing options, choose which item is focused. */
void OptionRow::AfterImportOptions( PlayerNumber pn )
{
	/* We load items for both players on start, since we don't know at that point
	 * which players will be joined when we're displayed.  Hide items for inactive
	 * players. */
	if( m_pHand->m_Def.m_layoutType == LAYOUT_SHOW_ONE_IN_ROW &&
		!m_pHand->m_Def.m_bOneChoiceForAllPlayers )
		m_textItems[pn]->SetHidden( !GAMESTATE->IsHumanPlayer(pn) );

	// Hide underlines for disabled players.
	if( !GAMESTATE->IsHumanPlayer(pn) )
		for( unsigned c=0; c<m_Underline[pn].size(); c++ )
			m_Underline[pn][c]->SetHidden( true );

	// Make all selections the same if bOneChoiceForAllPlayers
	// Hack: we only import active players, so if only player 2 is imported,
	// we need to copy p2 to p1, not p1 to p2.
	if( m_pHand->m_Def.m_bOneChoiceForAllPlayers )
	{
		PlayerNumber pnCopyFrom = GAMESTATE->m_MasterPlayerNumber;
		if( GAMESTATE->m_MasterPlayerNumber == PLAYER_INVALID )
			pnCopyFrom = PLAYER_1;
		FOREACH_PlayerNumber( p )
			m_vbSelected[p] = m_vbSelected[pnCopyFrom];
	}

	switch( m_pHand->m_Def.m_selectType )
	{
	case SELECT_ONE:
		/* Make sure the row actually has a selection. */
		int iSelection = GetOneSelection(pn, true);
		if( iSelection == -1 )
		{
			ASSERT( !m_vbSelected[pn].empty() );
			m_vbSelected[pn][0] = true;
		}
		break;
	}

	ResetFocusFromSelection( pn );

	PositionUnderlines( pn );
}

void OptionRow::PositionUnderlines( PlayerNumber pn )
{
	vector<OptionsCursor*> &vpUnderlines = m_Underline[pn];
	if( vpUnderlines.empty() )
		return;

	PlayerNumber pnTakeSelectedFrom = m_pHand->m_Def.m_bOneChoiceForAllPlayers ? PLAYER_1 : pn;

	for( int i=0; i<(int)vpUnderlines.size(); i++ )
	{
		OptionsCursor& ul = *vpUnderlines[i];

		int iChoiceWithFocus = (m_pHand->m_Def.m_layoutType == LAYOUT_SHOW_ONE_IN_ROW) ? GetChoiceInRowWithFocus(pn) : i;

		float fAlpha = 1.0f;
		if( m_pHand->m_Def.m_layoutType == LAYOUT_SHOW_ONE_IN_ROW )
		{
			bool bRowEnabled = m_pHand->m_Def.m_vEnabledForPlayers.find(pn) != m_pHand->m_Def.m_vEnabledForPlayers.end();

			if( !m_pHand->m_Def.m_bOneChoiceForAllPlayers )
			{
				if( m_bRowHasFocus[pn] )	fAlpha = m_pParentType->COLOR_SELECTED.GetValue().a;
				else if( bRowEnabled )		fAlpha = m_pParentType->COLOR_NOT_SELECTED.GetValue().a;
				else				fAlpha = m_pParentType->COLOR_DISABLED.GetValue().a;
			}
		}

		/* Don't tween X movement and color changes. */
		ul.StopTweening();

		int iWidth, iX, iY;
		GetWidthXY( pn, iChoiceWithFocus, iWidth, iX, iY );
		ul.SetX( (float)iX );
		ul.SetDiffuse( RageColor(1,1,1,fAlpha) );

		ASSERT( m_vbSelected[pnTakeSelectedFrom].size() == m_pHand->m_Def.m_vsChoices.size() );

		bool bSelected = (iChoiceWithFocus==-1) ? false : m_vbSelected[pnTakeSelectedFrom][ iChoiceWithFocus ];
		bool bHidden = !bSelected || !GAMESTATE->IsHumanPlayer(pn);

		ul.BeginTweening( m_pParentType->TWEEN_SECONDS );
		ul.SetHidden( bHidden );
		ul.SetBarWidth( iWidth );
	}
}

void OptionRow::PositionIcons( PlayerNumber pn )
{
	OptionIcon *pIcon = m_OptionIcons[pn];
	if( pIcon == NULL )
		return;

	pIcon->SetX( m_pParentType->ICONS_X.GetValue(pn) );
}

/* This is called when the focus changes, to update "long row" text. */
void OptionRow::UpdateText( PlayerNumber p )
{
	switch( m_pHand->m_Def.m_layoutType )
	{
	case LAYOUT_SHOW_ONE_IN_ROW:
		{
			unsigned pn = m_pHand->m_Def.m_bOneChoiceForAllPlayers ? 0 : p;
			int iChoiceWithFocus = m_iChoiceInRowWithFocus[pn];
			if( iChoiceWithFocus == -1 )
				break;

			RString sText = m_pHand->m_Def.m_vsChoices[iChoiceWithFocus];
			PrepareItemText( sText );

			// If player_no is 2 and there is no player 1:
			int index = min( pn, m_textItems.size()-1 );

			// TODO: Always have one textItem for each player

			m_textItems[index]->SetText( sText );
		}
		break;
	}
}

void OptionRow::SetRowHasFocus( PlayerNumber pn, bool bRowHasFocus )
{
	m_bRowHasFocus[pn] = bRowHasFocus;
}

void OptionRow::SetDestination( Actor::TweenState &ts, bool bTween )
{
	if( m_Frame.DestTweenState() != ts )
	{
		m_Frame.StopTweening();
		if( bTween && m_pParentType->TWEEN_SECONDS != 0 )
			m_Frame.BeginTweening( m_pParentType->TWEEN_SECONDS );
		m_Frame.DestTweenState() = ts;
	}
}

void OptionRow::UpdateEnabledDisabled()
{
	bool bThisRowHasFocusByAny = false;
	FOREACH_HumanPlayer( p )
		bThisRowHasFocusByAny |= m_bRowHasFocus[p];

	bool bThisRowHasFocusByAll = true;
	FOREACH_HumanPlayer( p )
		bThisRowHasFocusByAll &= m_bRowHasFocus[p];
	
	bool bRowEnabled = !m_pHand->m_Def.m_vEnabledForPlayers.empty();
	
	if( bThisRowHasFocusByAny )
		m_textTitle->RunCommands( m_pParentType->TITLE_GAIN_FOCUS_COMMAND );
	else
		m_textTitle->RunCommands( m_pParentType->TITLE_LOSE_FOCUS_COMMAND );

	/* Don't tween selection colors at all. */
	RageColor color;
	if( bThisRowHasFocusByAny )	color = m_pParentType->COLOR_SELECTED;
	else if( bRowEnabled )		color = m_pParentType->COLOR_NOT_SELECTED;
	else				color = m_pParentType->COLOR_DISABLED;

	if( m_sprBullet != NULL )
		m_sprBullet->SetGlobalDiffuseColor( color );
	m_sprBullet->PlayCommand( bThisRowHasFocusByAny ? "GainFocus" : "LoseFocus" );
	m_textTitle->SetGlobalDiffuseColor( color );


	for( unsigned j=0; j<m_textItems.size(); j++ )
	{
		bool bThisItemHasFocusByAny = false;
		FOREACH_HumanPlayer( p )
		{
			if( m_bRowHasFocus[p] )
			{
				if( (int)j == GetChoiceInRowWithFocus(p) )
				{
					bThisItemHasFocusByAny = true;
					break;
				}
			}
		}

		if( bThisItemHasFocusByAny )
			m_textItems[j]->RunCommands( m_pParentType->ITEM_GAIN_FOCUS_COMMAND );
		else
			m_textItems[j]->RunCommands( m_pParentType->ITEM_LOSE_FOCUS_COMMAND );
	}

	switch( m_pHand->m_Def.m_layoutType )
	{
	case LAYOUT_SHOW_ALL_IN_ROW:
		for( unsigned j=0; j<m_textItems.size(); j++ )
		{
			if( m_textItems[j]->DestTweenState().diffuse[0] == color ) 	 
				continue;

			m_textItems[j]->StopTweening();
			m_textItems[j]->BeginTweening( m_pParentType->TWEEN_SECONDS );
			m_textItems[j]->SetDiffuse( color );
		}
		
		break;
	case LAYOUT_SHOW_ONE_IN_ROW:
		FOREACH_HumanPlayer( pn )
		{
			bool bRowEnabled = m_pHand->m_Def.m_vEnabledForPlayers.find(pn) != m_pHand->m_Def.m_vEnabledForPlayers.end();
			
			if( !m_pHand->m_Def.m_bOneChoiceForAllPlayers )
			{
				if( m_bRowHasFocus[pn] )	color = m_pParentType->COLOR_SELECTED;
				else if( bRowEnabled )		color = m_pParentType->COLOR_NOT_SELECTED;
				else				color = m_pParentType->COLOR_DISABLED;
			}

			unsigned item_no = m_pHand->m_Def.m_bOneChoiceForAllPlayers ? 0 : pn;

			// If player_no is 2 and there is no player 1:
			item_no = min( item_no, m_textItems.size()-1 );

			BitmapText &bt = *m_textItems[item_no];

			if( bt.DestTweenState().diffuse[0] != color )
			{
				bt.StopTweening();
				bt.BeginTweening( m_pParentType->TWEEN_SECONDS );
				bt.SetDiffuse( color );
			}
		}
		break;
	default:
		ASSERT(0);
	}

	if( m_RowType == OptionRow::RowType_Exit )
	{
		if( bThisRowHasFocusByAll )
			m_textItems[0]->SetEffectDiffuseShift( 1.0f, m_pParentType->COLOR_SELECTED, m_pParentType->COLOR_NOT_SELECTED );
		else
			m_textItems[0]->StopEffect();
	}
}

void OptionRow::Update( float fDeltaTime )
{
	ActorFrame::Update( fDeltaTime );

	float fAlpha = m_Frame.GetCurrentDiffuseAlpha();
	m_Frame.SetBaseAlpha( fAlpha );
}

void OptionRow::SetOptionIcon( PlayerNumber pn, const RString &sText, GameCommand &gc )
{
	// update bullet
	Lua *L = LUA->Get();
	gc.PushSelf( L );
	lua_setglobal( L, "ThisGameCommand" );
	LUA->Release( L );

	m_sprBullet->PlayCommand( "Refresh" );
	if( m_OptionIcons[pn] != NULL )
		m_OptionIcons[pn]->Set( pn, sText, false );

	LUA->UnsetGlobal( "ThisGameCommand" );
}

const BitmapText &OptionRow::GetTextItemForRow( PlayerNumber pn, int iChoiceOnRow ) const
{
	bool bOneChoice = m_pHand->m_Def.m_bOneChoiceForAllPlayers;
	int index = -1;
	switch( m_pHand->m_Def.m_layoutType )
	{
	case LAYOUT_SHOW_ONE_IN_ROW:
		index = bOneChoice ? 0 : pn;
		/* If only P2 is enabled, his selections will be in index 0. */
		if( m_textItems.size() == 1 )
			index = 0;
		break;
	case LAYOUT_SHOW_ALL_IN_ROW:
		index = iChoiceOnRow;
		break;
	default:
		ASSERT(0);
	}

	ASSERT_M( index < (int)m_textItems.size(), ssprintf("%i < %i", index, (int)m_textItems.size() ) );
	return *m_textItems[index];
}

void OptionRow::GetWidthXY( PlayerNumber pn, int iChoiceOnRow, int &iWidthOut, int &iXOut, int &iYOut ) const
{
	const BitmapText &text = GetTextItemForRow( pn, iChoiceOnRow );

	iWidthOut = int(roundf( text.GetZoomedWidth() ));
	iXOut = int(roundf( text.GetDestX() ));
	iYOut = int( roundf(m_Frame.GetDestY()) );
}


int OptionRow::GetOneSelection( PlayerNumber pn, bool bAllowFail ) const
{
	for( unsigned i=0; i<m_vbSelected[pn].size(); i++ )
		if( m_vbSelected[pn][i] )
			return i;

	ASSERT( bAllowFail );	// shouldn't call this if not expecting one to be selected
	return -1;
}

int OptionRow::GetOneSharedSelection( bool bAllowFail ) const
{
	return GetOneSelection( PLAYER_1, bAllowFail );
}

void OptionRow::SetOneSelection( PlayerNumber pn, int iChoice )
{
	if( m_vbSelected[pn].empty() )
		return;
	for( unsigned i=0; i<(unsigned)m_vbSelected[pn].size(); i++ )
		m_vbSelected[pn][i] = false;
	m_vbSelected[pn][iChoice] = true;
}

void OptionRow::SetOneSharedSelection( int iChoice )
{
	FOREACH_PlayerNumber( pn )
		SetOneSelection( pn, iChoice );
}

void OptionRow::SetOneSharedSelectionIfPresent( const RString &sChoice )
{
	for( unsigned i=0; i<m_pHand->m_Def.m_vsChoices.size(); i++ )
	{
		if( sChoice == m_pHand->m_Def.m_vsChoices[i] )
		{
			SetOneSharedSelection( i );
			break;
		}
	}
}

int OptionRow::GetChoiceInRowWithFocus( PlayerNumber pn ) const
{
	if( m_pHand->m_Def.m_bOneChoiceForAllPlayers )
		pn = PLAYER_1;
	if( m_pHand->m_Def.m_vsChoices.empty() )
		return -1;
	int iChoice = m_iChoiceInRowWithFocus[pn];
	return iChoice; 
}

int OptionRow::GetChoiceInRowWithFocusShared() const
{
	return GetChoiceInRowWithFocus( PLAYER_1 );
}

void OptionRow::SetChoiceInRowWithFocus( PlayerNumber pn, int iChoice )
{
	if( m_pHand->m_Def.m_bOneChoiceForAllPlayers )
		pn = PLAYER_1;
	ASSERT(iChoice >= 0 && iChoice < (int)m_pHand->m_Def.m_vsChoices.size());
	m_iChoiceInRowWithFocus[pn] = iChoice;

	UpdateText( pn );
}

void OptionRow::ResetFocusFromSelection( PlayerNumber pn )
{
	int iSelection = -1;
	switch( m_pHand->m_Def.m_selectType )
	{
	case SELECT_ONE:
		/* Import the focus from the selected option. */
		iSelection = GetOneSelection( pn, true );
		break;
	}

	//
	// HACK: Set focus to one item in the row, which is "go down"
	//
	if( m_bFirstItemGoesDown )
		iSelection = 0;

	if( iSelection != -1 )
		SetChoiceInRowWithFocus( pn, iSelection );
}

bool OptionRow::GetSelected( PlayerNumber pn, int iChoice ) const
{
	if( m_pHand->m_Def.m_bOneChoiceForAllPlayers )
		pn = PLAYER_1;
	return m_vbSelected[pn][iChoice];
}

const OptionRowDefinition &OptionRow::GetRowDef() const
{
	return m_pHand->m_Def;
}

OptionRowDefinition &OptionRow::GetRowDef() 
{
	return m_pHand->m_Def;
}

void OptionRow::SetSelected( PlayerNumber pn, int iChoice, bool b )
{
	if( m_pHand->m_Def.m_bOneChoiceForAllPlayers )
		pn = PLAYER_1;
	m_vbSelected[pn][iChoice] = b;
}

void OptionRow::SetExitText( RString sExitText )
{
	BitmapText *bt = m_textItems.back();
	bt->SetText( sExitText );
}

void OptionRow::Reload()
{
	// TODO: Nothing uses this yet and it causes skips when changing options.
	//if( m_pHand->m_Def.m_bExportOnChange )
	//{
	//	bool bRowHasFocus[NUM_PLAYERS];
	//	ZERO( bRowHasFocus );
	//	ExportOptions( vpns, bRowHasFocus );
	//}

	switch( m_pHand->Reload() )
	{
	case OptionRowHandler::RELOAD_CHANGED_NONE:
		break;

	case OptionRowHandler::RELOAD_CHANGED_ALL:
	{
		ChoicesChanged();

		vector<PlayerNumber> vpns;
		FOREACH_HumanPlayer( p )
			vpns.push_back( p );
		ImportOptions( vpns );
		FOREACH_HumanPlayer( p )
			AfterImportOptions( p );
		// fall through
	}

	case OptionRowHandler::RELOAD_CHANGED_ENABLED:
		UpdateEnabledDisabled();
		FOREACH_HumanPlayer( pn )
			PositionUnderlines( pn );
		break;
	}


	// TODO: Nothing uses this yet and it causes skips when changing options.
	//if( m_pHand->m_Def.m_bExportOnChange )
	//{
	//	bool bRowHasFocus[NUM_PLAYERS];
	//	ZERO( bRowHasFocus );
	//	ExportOptions( vpns, bRowHasFocus );
	//}
}

void OptionRow::HandleMessage( const RString& sMessage )
{
	Reload();
}


/* Hack: the NextRow entry is never set, and should be transparent.  Remove
 * it, and readd it below. */
#define ERASE_ONE_BOOL_AT_FRONT_IF_NEEDED( vbSelected ) \
	if( GetFirstItemGoesDown() ) \
		vbSelected.erase( vbSelected.begin() );
#define INSERT_ONE_BOOL_AT_FRONT_IF_NEEDED( vbSelected ) \
	if( GetFirstItemGoesDown() ) \
		vbSelected.insert( vbSelected.begin(), false );

void OptionRow::ImportOptions( const vector<PlayerNumber> &vpns )
{
	ASSERT( m_pHand->m_Def.m_vsChoices.size() > 0 );

	FOREACH_CONST( PlayerNumber, vpns, iter )
	{
		PlayerNumber p = *iter;

		FOREACH( bool, m_vbSelected[p], b )
			*b = false;

		ASSERT( m_vbSelected[p].size() == m_pHand->m_Def.m_vsChoices.size() );
		ERASE_ONE_BOOL_AT_FRONT_IF_NEEDED( m_vbSelected[p] );
	}
	
	m_pHand->ImportOption( vpns, m_vbSelected );

	FOREACH_CONST( PlayerNumber, vpns, iter )
	{
		PlayerNumber p = *iter;

		INSERT_ONE_BOOL_AT_FRONT_IF_NEEDED( m_vbSelected[p] );
		VerifySelected( m_pHand->m_Def.m_selectType, m_vbSelected[p], m_pHand->m_Def.m_sName );
	}
}

int OptionRow::ExportOptions( const vector<PlayerNumber> &vpns, bool bRowHasFocus[NUM_PLAYERS] )
{
	ASSERT( m_pHand->m_Def.m_vsChoices.size() > 0 );

	int iChangeMask = 0;
	
	FOREACH_CONST( PlayerNumber, vpns, iter )
	{
		PlayerNumber p = *iter;
		bool bFocus = bRowHasFocus[p];

		VerifySelected( m_pHand->m_Def.m_selectType, m_vbSelected[p], m_pHand->m_Def.m_sName );
		ASSERT( m_vbSelected[p].size() == m_pHand->m_Def.m_vsChoices.size() );
		ERASE_ONE_BOOL_AT_FRONT_IF_NEEDED( m_vbSelected[p] );

		// SELECT_NONE rows get exported if they have focus when the user presses 
		// Start.
		int iChoice = GetChoiceInRowWithFocus( p );
		if( m_pHand->m_Def.m_selectType == SELECT_NONE && bFocus )
			m_vbSelected[p][iChoice] = true;
	}

	iChangeMask |= m_pHand->ExportOption( vpns, m_vbSelected );
	
	FOREACH_CONST( PlayerNumber, vpns, iter )
	{
		PlayerNumber p = *iter;
		bool bFocus = bRowHasFocus[p];

		int iChoice = GetChoiceInRowWithFocus( p );
		if( m_pHand->m_Def.m_selectType == SELECT_NONE && bFocus )
			m_vbSelected[p][iChoice] = false;
		
		INSERT_ONE_BOOL_AT_FRONT_IF_NEEDED( m_vbSelected[p] );
	}

	return iChangeMask;
}


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
