/*
-----------------------------------------------------------------------------
 File: TransitionFadeWipe.cpp

 Desc: Fades out or in.

 Copyright (c) 2001 Chris Danford.  All rights reserved.
-----------------------------------------------------------------------------
*/

#ifndef _TransitionFadeWipe_H_
#define _TransitionFadeWipe_H_


#include "Transition.h"
#include "RageScreen.h"
#include "RageSound.h"


class TransitionFadeWipe : public Transition
{
public:
	TransitionFadeWipe();
	~TransitionFadeWipe();

	void Draw();
};




#endif