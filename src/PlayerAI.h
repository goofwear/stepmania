#ifndef PlayerAI_H
#define PlayerAI_H
/*
-----------------------------------------------------------------------------
 Class: PlayerAI

 Desc: .

 Copyright (c) 2001-2002 by the person(s) listed below.  All rights reserved.
	Chris Danford
-----------------------------------------------------------------------------
*/

#include "GameConstantsAndTypes.h"

const int NUM_SKILL_LEVELS = 6;	// 0-5

class PlayerAI
{
public:

	static void InitFromDisk();
	static TapNoteScore GetTapNoteScore( PlayerNumber pn );

};

#endif
