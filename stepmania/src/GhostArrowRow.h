#ifndef GHOSTARROWROW_H
#define GHOSTARROWROW_H
/*
-----------------------------------------------------------------------------
 File: GhostArrowRow.h

 Desc: A row of GhostArrow Actors

 Copyright (c) 2001-2002 by the person(s) listed below.  All rights reserved.
	Chris Danford
-----------------------------------------------------------------------------
*/

#include "ActorFrame.h"
#include "GhostArrow.h"
#include "HoldGhostArrow.h"
#include "GameConstantsAndTypes.h"
#include "StyleDef.h"



class GhostArrowRow : public ActorFrame
{
public:
	GhostArrowRow();
	virtual void Update( float fDeltaTime );
	virtual void DrawPrimitives();
	virtual void CopyTweening( const GhostArrowRow &from );

	void Load( PlayerNumber pn, CString NoteSkin, float fYReverseOffset );
	
	void DidTapNote( int iCol, TapNoteScore score, bool bBright );
	void DidTapMine( int iCol, TapNoteScore score );
	void SetHoldIsActive( int iCol );
	
protected:
	int m_iNumCols;
	float m_fYReverseOffsetPixels;
	PlayerNumber m_PlayerNumber;

	GhostArrow		m_GhostDim[MAX_NOTE_TRACKS];
	GhostArrow		m_GhostBright[MAX_NOTE_TRACKS];
	GhostArrow		m_GhostMine[MAX_NOTE_TRACKS];
	HoldGhostArrow	m_HoldGhost[MAX_NOTE_TRACKS];
};


#endif
