#ifndef SCREENLOGO_H
#define SCREENLOGO_H
/*
-----------------------------------------------------------------------------
 Class: ScreenLogo

 Desc: 

 Copyright (c) 2001-2002 by the person(s) listed below.  All rights reserved.
	Chris Danford
-----------------------------------------------------------------------------
*/

#include "ScreenAttract.h"
#include "Sprite.h"


class ScreenLogo : public ScreenAttract
{
public:
	ScreenLogo( CString sName );

protected:
	Sprite				m_sprLogo;
};



#endif
