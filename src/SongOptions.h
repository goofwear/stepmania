/* SongOptions - Options that apply to an entire song (not per-player). */

#ifndef SONG_OPTIONS_H
#define SONG_OPTIONS_H

class SongOptions
{
public:
	enum LifeType
	{
		LIFE_BAR=0,
		LIFE_BATTERY,
		LIFE_TIME,
		NUM_LIFE_TYPES
	};
	LifeType m_LifeType;
	enum DrainType
	{
		DRAIN_NORMAL,
		DRAIN_NO_RECOVER,
		DRAIN_SUDDEN_DEATH
	};
	DrainType m_DrainType;	// only used with LifeBar
	int m_iBatteryLives;
	enum FailType { 
		FAIL_IMMEDIATE=0,			// fail immediately when life touches 0
		FAIL_IMMEDIATE_CONTINUE,		// fail immediately when life touches 0, but allow playing the rest of the song
		FAIL_AT_END,				// fail if life is at 0 when the song ends
		FAIL_OFF };					// never fail
	FailType m_FailType;
	float m_fMusicRate,	m_SpeedfMusicRate;
	float m_fHaste, m_SpeedfHaste;
	bool m_bAssistClap;
	bool m_bAssistMetronome;
	enum AutosyncType { 
		AUTOSYNC_OFF,
		AUTOSYNC_SONG,
		AUTOSYNC_MACHINE,
		AUTOSYNC_TEMPO,
		NUM_AUTOSYNC_TYPES
	};
	AutosyncType m_AutosyncType;
	bool m_bSaveScore;
	enum SoundEffectType {
		SOUNDEFFECT_OFF,
		SOUNDEFFECT_SPEED,
		SOUNDEFFECT_PITCH,
		NUM_SOUNDEFFECT
	};
	SoundEffectType m_SoundEffectType;


	SongOptions() { Init(); };
	void Init();
	void Approach( const SongOptions& other, float fDeltaSeconds );
	void GetMods( vector<RString> &AddTo ) const;
	void GetLocalizedMods( vector<RString> &AddTo ) const;
	RString GetString() const;
	RString GetLocalizedString() const;
	void FromString( const RString &sOptions );

	bool operator==( const SongOptions &other ) const;
	bool operator!=( const SongOptions &other ) const { return !operator==(other); }
};

#endif

/*
 * (c) 2001-2004 Chris Danford, Glenn Maynard
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
