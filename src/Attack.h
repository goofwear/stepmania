#ifndef ATTACK_H
#define ATTACK_H

#include "GameConstantsAndTypes.h"
#include "PlayerNumber.h"
class Song;

struct Attack
{
	AttackLevel	level;
	float fStartSecond; // -1 = now
	float fSecsRemaining;
	CString sModifier;
	bool bOn; // for GAMESTATE
	bool bGlobal; // true for song-wide course mods

	void MakeBlank() { sModifier=""; fStartSecond = -1; bOn = false; bGlobal = false; }
	Attack() { MakeBlank(); }
	Attack(
		AttackLevel	level_,
		float fStartSecond_,
		float fSecsRemaining_,
		CString sModifier_,
		bool bOn_,
		bool bGlobal_ )
	{
		level = level_;
		fStartSecond = fStartSecond_;
		fSecsRemaining = fSecsRemaining_;
		sModifier = sModifier_;
		bOn = bOn_;
		bGlobal = bGlobal_;
	}

	void GetAttackBeats( const Song *song, PlayerNumber pn, float &fStartBeat, float &fEndBeat ) const;
	bool IsBlank() { return sModifier.empty(); }
	bool operator== ( const Attack &rhs ) const;
	bool ContainsTransformOrTurn() const;
};

struct AttackArray : public vector<Attack>
{
	bool ContainsTransformOrTurn() const;
};

#endif

/*
 * (c) 2003-2004 Chris Danford
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
