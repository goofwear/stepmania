#ifndef RAGE_SOUND_ALSA9
#define RAGE_SOUND_ALSA9

/* RageSound_ALSA9 fakes playing sounds having GetPosition()
 * return seconds since the constructor was called.
 *
 * Only tested in linux, but intended to work across the globe.
 * ( uses time_t, sleep() and nanosleep() from <time.h> )
 *
 * The timing probably isn't accurate, but at least it is fairly
 * steady.  Someone with more knowledge of RageSound, feel free
 * to play around with this (not that it's any sort of priority).
 */

#include "RageSound.h"
#include "RageThreads.h"
#include "RageSoundDriver.h"

#define ALSA_PCM_NEW_HW_PARAMS_API
#include <alsa/asoundlib.h>

/* needed because ALSA returns struct timeval info */
#include <sys/time.h>

class RageSound_ALSA9: public RageSoundDriver
{
public:
	struct sound {
		RageSound *snd;
		bool stopping;
                int flush_pos; /* state == STOPPING only */
        sound() { snd = NULL; stopping=false; }
	};

	/* List of currently playing sounds: */
	vector<sound *> sounds;

	snd_pcm_sframes_t total_frames;

	bool shutdown;
        int last_cursor_pos;

	snd_pcm_t *pcm;

	static int MixerThread_start(void *p);
	void MixerThread();
	RageThread MixingThread;

	int GetData();
	bool Recover( int r );

	/* virtuals: */
	void StartMixing(RageSound *snd);
	void StopMixing(RageSound *snd);
	int GetPosition(const RageSound *snd) const;
	float GetPlayLatency() const;

	void Update(float delta);

	RageSound_ALSA9();
	~RageSound_ALSA9();
};

#endif

/*
 * Copyright (c) 2003 by the person(s) listed below.  All rights reserved.
 *
 * Glenn Maynard
 * 
 * 2003-02   Modified to fake playing sound   Aaron VonderHaar
 * 
 */
