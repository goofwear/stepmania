#ifndef RAGE_SOUND_MANAGER_H
#define RAGE_SOUND_MANAGER_H

#include <set>
#include <map>
#include "SDL_utils.h"
#include "RageThreads.h"

class RageSound;
class RageSoundBase;
class RageSoundDriver;

class RageSoundManager
{
	/* Set of sounds that we've taken over (and are responsible for deleting
	 * when they're finished playing): */
	set<RageSoundBase *> owned_sounds;

	/* Set of sounds that are finished and should be deleted. */
	set<RageSoundBase *> sounds_to_delete;

	RageSoundDriver *driver;

	/* Prefs: */
	float MixVolume;

public:
	RageMutex lock;

	RageSoundManager(CString drivers);
	~RageSoundManager();

	float GetMixVolume() const { return MixVolume; }
	void SetPrefs(float MixVol);

	void Update(float delta);
	void StartMixing( RageSoundBase *snd );	/* used by RageSound */
	void StopMixing( RageSoundBase *snd );	/* used by RageSound */
	int64_t GetPosition( const RageSoundBase *snd ) const;	/* used by RageSound */
	float GetPlayLatency() const;
	int GetDriverSampleRate( int rate ) const;
	const set<RageSound *> &GetPlayingSounds() const { return playing_sounds; }

	void PlayOnce( CString sPath );

	RageSound *PlaySound(RageSound &snd);
	void StopPlayingSound(RageSound &snd);

	/* A list of all sounds that currently exist.  RageSound adds and removes
	 * itself to this. */
	set<RageSound *> all_sounds;
	
	/* RageSound adds and removes itself to this. */
	set<RageSound *> playing_sounds;
	void GetCopies(RageSound &snd, vector<RageSound *> &snds);

	static void AttenuateBuf( Sint16 *buf, int samples, float vol );
};

/* This inputs and outputs 16-bit 44khz stereo input. */
class SoundMixBuffer
{
	Sint32 *mixbuf;
	unsigned bufsize; /* actual allocated samples */
	unsigned used; /* used samples */
	int vol; /* vol * 256 */

public:
	void write(const Sint16 *buf, unsigned size);
	void read(Sint16 *buf);
	void read( float *buf );
	unsigned size() const { return used; }
	void SetVolume(float f);

	SoundMixBuffer();
	~SoundMixBuffer();
};

extern RageSoundManager *SOUNDMAN;

#endif
