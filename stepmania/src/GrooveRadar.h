#ifndef GROOVERADAR_H
#define GROOVERADAR_H
/*
-----------------------------------------------------------------------------
 Class: GrooveRadar

 Desc: The song's GrooveRadar displayed in SelectSong.

 Copyright (c) 2001-2002 by the person(s) listed below.  All rights reserved.
	Chris Danford
-----------------------------------------------------------------------------
*/

#include "ActorFrame.h"
#include "Sprite.h"
#include "PlayerNumber.h"
#include "GameConstantsAndTypes.h"
class Steps;


class GrooveRadar : public ActorFrame
{
public:
	GrooveRadar();

	void SetEmpty( PlayerNumber pn )
	{
		SetFromSteps( pn, NULL );
	}

	void SetFromSteps( PlayerNumber pn, Steps* pSteps )	// NULL means no Song
	{
		m_GrooveRadarValueMap.SetFromSteps( pn, pSteps );
	}

	void TweenOnScreen();
	void TweenOffScreen();

protected:

	// the value map must be a separate Actor so we can tween it separately from the labels
	class GrooveRadarValueMap : public ActorFrame
	{
	public:
		GrooveRadarValueMap();

		virtual void Update( float fDeltaTime );
		virtual void DrawPrimitives();

		void SetFromSteps( PlayerNumber pn, Steps* pSteps );	// NULL means no Song

		void TweenOnScreen();
		void TweenOffScreen();

		bool m_bValuesVisible[NUM_PLAYERS];
		float m_PercentTowardNew[NUM_PLAYERS];
		float m_fValuesNew[NUM_PLAYERS][NUM_RADAR_CATEGORIES];
		float m_fValuesOld[NUM_PLAYERS][NUM_RADAR_CATEGORIES];

		Sprite m_sprRadarBase;
	};

	GrooveRadarValueMap m_GrooveRadarValueMap;
	Sprite m_sprRadarLabels[NUM_RADAR_CATEGORIES];
};

#endif
