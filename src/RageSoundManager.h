#ifndef RAGE_SOUND_MANAGER_H
#define RAGE_SOUND_MANAGER_H

#include <set>
#include <map>
#include "SDL_utils.h"
#include "RageUtil_CircularBuffer.h"

class RageSound;
class RageSoundBase;
class RageSoundDriver;
struct RageSoundParams;

class RageSoundManager
{
	/* Set of sounds that we've taken over (and are responsible for deleting
	 * when they're finished playing): */
	set<RageSound *> owned_sounds;
	set<RageSound *> playing_sounds;

	/* A list of all sounds that currently exist. */
	set<RageSound *> all_sounds;
	
	RageSoundDriver *driver;

	/* Prefs: */
	float MixVolume;
	struct queued_pos_map_t
	{
		int ID, pos, got_frames;
		int64_t frameno;
	};

	CircBuf<queued_pos_map_t> pos_map_queue;

public:
	RageSoundManager(CString drivers);
	~RageSoundManager();

	float GetMixVolume() const { return MixVolume; }
	void SetPrefs(float MixVol);

	void Update(float delta);
	void StartMixing( RageSoundBase *snd );	/* used by RageSound */
	void StopMixing( RageSoundBase *snd );	/* used by RageSound */
	int64_t GetPosition( const RageSoundBase *snd ) const;	/* used by RageSound */
	int RegisterSound( RageSound *p );		/* used by RageSound */
	void UnregisterSound( RageSound *p );	/* used by RageSound */
	void RegisterPlayingSound( RageSound *p );	/* used by RageSound */
	void UnregisterPlayingSound( RageSound *p );	/* used by RageSound */
	void CommitPlayingPosition( int ID, int64_t frameno, int pos, int got_bytes );	/* used by drivers */
	float GetPlayLatency() const;
	int GetDriverSampleRate( int rate ) const;
	set<RageSound *> GetPlayingSounds() const;

	void PlayOnce( CString sPath );

	RageSound *PlaySound( RageSound &snd, const RageSoundParams *params );
	void StopPlayingAllCopiesOfSound(RageSound &snd);

	/* Stop all sounds that were started by this thread.  This should be called
	 * before exiting a thread. */
	void StopPlayingSoundsForThisThread();

	void GetCopies( RageSound &snd, vector<RageSound *> &snds, bool bLockSounds=false );

	static void AttenuateBuf( Sint16 *buf, int samples, float vol );

private:
	void FlushPosMapQueue();
	RageSound *GetSoundByID( int ID );
};

/* This inputs and outputs 16-bit 44khz stereo input. */
class SoundMixBuffer
{
	Sint32 *mixbuf;
	unsigned bufsize; /* actual allocated samples */
	unsigned used; /* used samples */
	int vol; /* vol * 256 */

public:
	void write( const Sint16 *buf, unsigned size, float volume = -1, int offset = 0 );
	void read(Sint16 *buf);
	void read( float *buf );
	unsigned size() const { return used; }
	void SetVolume(float f);

	SoundMixBuffer();
	~SoundMixBuffer();
};

extern RageSoundManager *SOUNDMAN;

#endif
