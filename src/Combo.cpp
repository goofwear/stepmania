#include "global.h"
#include "Combo.h"
#include "ThemeManager.h"
#include "StatsManager.h"
#include "GameState.h"
#include "song.h"
#include "Command.h"

Combo::Combo()
{
	this->AddChild( &m_sprComboLabel );
	this->AddChild( &m_sprMissesLabel );
	this->AddChild( &m_textNumber );
}

void Combo::Load( PlayerNumber pn )
{
	m_PlayerNumber = pn;

	LABEL_X.Load(m_sName,"LabelX");
	LABEL_Y.Load(m_sName,"LabelY");
	LABEL_HORIZ_ALIGN.Load(m_sName,"LabelHorizAlign");
	LABEL_VERT_ALIGN.Load(m_sName,"LabelVertAlign");
	NUMBER_X.Load(m_sName,"NumberX");
	NUMBER_Y.Load(m_sName,"NumberY");
	NUMBER_HORIZ_ALIGN.Load(m_sName,"NumberHorizAlign");
	NUMBER_VERT_ALIGN.Load(m_sName,"NumberVertAlign");
	SHOW_COMBO_AT.Load(m_sName,"ShowComboAt");
	NUMBER_MIN_ZOOM.Load(m_sName,"NumberMinZoom");
	NUMBER_MAX_ZOOM.Load(m_sName,"NumberMaxZoom");
	NUMBER_MAX_ZOOM_AT.Load(m_sName,"NumberMaxZoomAt");
	PULSE_ZOOM.Load(m_sName,"PulseZoom");
	C_TWEEN_SECONDS.Load(m_sName,"TweenSeconds");
	FULL_COMBO_GREATS_COMMAND.Load(m_sName,"FullComboGreatsCommand");
	FULL_COMBO_PERFECTS_COMMAND.Load(m_sName,"FullComboPerfectsCommand");
	FULL_COMBO_MARVELOUSES_COMMAND.Load(m_sName,"FullComboMarvelousesCommand");
	FULL_COMBO_BROKEN_COMMAND.Load(m_sName,"FullComboBrokenCommand");
	SHOW_MISS_COMBO.Load(m_sName,"ShowMissCombo");

	m_sprComboLabel.Load( THEME->GetPathG(m_sName,"label") );
	m_sprComboLabel.SetShadowLength( 4 );
	m_sprComboLabel.StopAnimating();
	m_sprComboLabel.SetXY( LABEL_X, LABEL_Y );
	m_sprComboLabel.SetHorizAlign( (Actor::HorizAlign)(int)LABEL_HORIZ_ALIGN );
	m_sprComboLabel.SetVertAlign( (Actor::VertAlign)(int)LABEL_VERT_ALIGN );
	m_sprComboLabel.SetHidden( true );

	m_sprMissesLabel.Load( THEME->GetPathG(m_sName,"misses") );
	m_sprMissesLabel.SetShadowLength( 4 );
	m_sprMissesLabel.StopAnimating();
	m_sprMissesLabel.SetXY( LABEL_X, LABEL_Y );
	m_sprMissesLabel.SetHorizAlign( (Actor::HorizAlign)(int)LABEL_HORIZ_ALIGN );
	m_sprMissesLabel.SetVertAlign( (Actor::VertAlign)(int)LABEL_VERT_ALIGN );
	m_sprMissesLabel.SetHidden( true );

	m_textNumber.LoadFromFont( THEME->GetPathF(m_sName,"numbers") );
	m_textNumber.SetShadowLength( 4 );
	m_textNumber.SetXY( NUMBER_X, NUMBER_Y );
	m_textNumber.SetHorizAlign( (Actor::HorizAlign)(int)NUMBER_HORIZ_ALIGN );
	m_textNumber.SetVertAlign( (Actor::VertAlign)(int)NUMBER_VERT_ALIGN );
	m_textNumber.SetHidden( true );
}

void Combo::SetCombo( int iCombo, int iMisses )
{
	bool bMisses = iMisses > 0;
	int iNum = bMisses ? iMisses : iCombo;

	if( (iNum < (int)SHOW_COMBO_AT)  || 
		(bMisses && !(bool)SHOW_MISS_COMBO) )
	{
		m_sprComboLabel.SetHidden( true );
		m_sprMissesLabel.SetHidden( true );
		m_textNumber.SetHidden( true );
		return;
	}

	m_sprComboLabel.SetHidden( bMisses );
	m_sprMissesLabel.SetHidden( !bMisses );
	m_textNumber.SetHidden( false );

	CString txt = ssprintf("%d", iNum);
	/* Don't do anything if it's not changing. */
	if(m_textNumber.GetText() == txt) return;

	m_textNumber.SetText( txt );
	float fNumberZoom = SCALE(iNum,0.f,(float)NUMBER_MAX_ZOOM_AT,(float)NUMBER_MIN_ZOOM,(float)NUMBER_MAX_ZOOM);
	CLAMP( fNumberZoom, (float)NUMBER_MIN_ZOOM, (float)NUMBER_MAX_ZOOM );
	m_textNumber.StopTweening();
	m_textNumber.SetZoom( fNumberZoom * (float)PULSE_ZOOM ); 
	m_textNumber.BeginTweening( C_TWEEN_SECONDS );
	m_textNumber.SetZoom( fNumberZoom );

	Sprite &sprLabel = bMisses ? m_sprMissesLabel : m_sprComboLabel;

	sprLabel.StopTweening();
	sprLabel.SetZoom( PULSE_ZOOM ); 
	sprLabel.BeginTweening( C_TWEEN_SECONDS );
	sprLabel.SetZoom( 1 );

	// don't show a colored combo until 1/4 of the way through the song
	bool bPastMidpoint = GAMESTATE->GetCourseSongIndex()>0 ||
		GAMESTATE->m_fMusicSeconds > GAMESTATE->m_pCurSong->m_fMusicLengthSeconds/4;

	if( bPastMidpoint )
	{
		if( STATSMAN->m_CurStageStats.m_player[m_PlayerNumber].FullComboOfScore(TNS_MARVELOUS) )
		{
			sprLabel.RunCommands( FULL_COMBO_MARVELOUSES_COMMAND );
			m_textNumber.RunCommands( FULL_COMBO_MARVELOUSES_COMMAND );
		}
		else if( bPastMidpoint && STATSMAN->m_CurStageStats.m_player[m_PlayerNumber].FullComboOfScore(TNS_PERFECT) )
		{
			sprLabel.RunCommands( FULL_COMBO_PERFECTS_COMMAND );
			m_textNumber.RunCommands( FULL_COMBO_PERFECTS_COMMAND );
		}
		else if( bPastMidpoint && STATSMAN->m_CurStageStats.m_player[m_PlayerNumber].FullComboOfScore(TNS_GREAT) )
		{
			sprLabel.RunCommands( FULL_COMBO_GREATS_COMMAND );
			m_textNumber.RunCommands( FULL_COMBO_GREATS_COMMAND );
		}
		else
		{
			sprLabel.RunCommands( FULL_COMBO_BROKEN_COMMAND );
			m_textNumber.RunCommands( FULL_COMBO_BROKEN_COMMAND );
		}
	}
	else
	{
		sprLabel.RunCommands( FULL_COMBO_BROKEN_COMMAND );
		m_textNumber.RunCommands( FULL_COMBO_BROKEN_COMMAND );
	}
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
