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

	void GetAttackBeats( const Song *song, PlayerNumber pn, float &fStartBeat, float &fEndBeat ) const;
	bool IsBlank() { return sModifier.empty(); }
	void MakeBlank() { sModifier=""; }
	Attack() { fStartSecond = -1; }
};
typedef vector<Attack> AttackArray;

#endif
