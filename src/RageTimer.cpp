#include "global.h"

#include "RageTimer.h"
#include "RageLog.h"

#include "SDL.h"
#include "SDL_timer.h"
 
/* We only actually get 1000 using SDL. */
#define TIMESTAMP_RESOLUTION 1000000

const RageZeroTimer_t RageZeroTimer;

float RageTimer::GetTimeSinceStart()
{
	return SDL_GetTicks() / 1000.0f;
}

void RageTimer::Touch()
{
	unsigned ms = SDL_GetTicks();
	this->m_secs = ms / 1000;
	ms %= 1000;

	unsigned mult = TIMESTAMP_RESOLUTION / 1000;
	this->m_us = ms*mult;
}

float RageTimer::Ago() const
{
	const RageTimer Now;

	/* If the system clock has moved backwards (for example, ntpd), don't return negative values. */
	return max( 0.0f, Now - *this );
}

float RageTimer::GetDeltaTime()
{
	const RageTimer Now;
	const float diff = Difference( Now, *this );
	*this = Now;

	/* If the system clock has moved backwards (for example, ntpd), don't return negative values. */
	return max( 0.0f, diff );
}

/* Get a timer representing half of the time ago as this one.  This is
 * useful for averaging time.  For example,
 * 
 * RageTimer tm;
 * ... do stuff ...
 * RageTimer AverageTime = tm.Half();
 * printf("Something happened between now and tm; the average time is %f.\n", tm.Ago());
 * tm.Touch();
 */
RageTimer RageTimer::Half() const
{
	const RageTimer now;
	const float ProbableDelay = -(now - *this) / 2;
	return *this + ProbableDelay;
}


RageTimer RageTimer::operator+(float tm) const
{
	return Sum(*this, tm);
}

float RageTimer::operator-(const RageTimer &rhs) const
{
	return Difference(*this, rhs);
}

RageTimer RageTimer::Sum(const RageTimer &lhs, float tm)
{
	/* tm == 5.25  -> secs =  5, us = 5.25  - ( 5) = .25
	 * tm == -1.25 -> secs = -2, us = -1.25 - (-2) = .75 */
	int seconds = (int) floorf(tm);
	int us = int( (tm - seconds) * TIMESTAMP_RESOLUTION );

	RageTimer ret;
	ret.m_secs = seconds + lhs.m_secs;
	ret.m_us = us + lhs.m_us;

	if( ret.m_us >= TIMESTAMP_RESOLUTION )
	{
		ret.m_us -= TIMESTAMP_RESOLUTION;
		++ret.m_secs;
	}

	return ret;
}

float RageTimer::Difference(const RageTimer &lhs, const RageTimer &rhs)
{
	int secs = lhs.m_secs - rhs.m_secs;
	int us = lhs.m_us - rhs.m_us;

	if( us < 0 )
	{
		us += TIMESTAMP_RESOLUTION;
		--secs;
	}

	return float(secs) + float(us) / TIMESTAMP_RESOLUTION;
}

/*
 * Copyright (c) 2001-2003 by the person(s) listed below.  All rights reserved.
 *	Chris Danford
 *	Glenn Maynard
 */
