#ifndef RAGE_SOUND_DSOUND_SOFTWARE
#define RAGE_SOUND_DSOUND_SOFTWARE

#include "RageSoundDriver.h"
#include "DSoundHelpers.h"
#include "RageThreads.h"

struct IDirectSound;
struct IDirectSoundBuffer;

class RageSound_DSound_Software: public RageSoundDriver
{
	struct sound {
	    RageSound *snd;

		bool stopping;

		int flush_pos; /* state == STOPPING only */

	    sound() { snd = NULL; stopping=false; }
	};

	/* List of currently playing sounds: */
	vector<sound *> sounds;

	bool shutdown;

	DSound ds;
	DSoundBuf *str_ds;

	bool GetData();
	void Update(float delta);

	static int MixerThread_start(void *p);
	void MixerThread();
	RageThread MixingThread;

	/* virtuals: */
	void StartMixing(RageSound *snd);	/* used by RageSound */
	void StopMixing(RageSound *snd);		/* used by RageSound */
	int GetPosition(const RageSound *snd) const;
	float GetPlayLatency() const;
	int GetSampleRate( int rate ) const;

public:
	RageSound_DSound_Software();
	~RageSound_DSound_Software();
};

#endif

/*
 * Copyright (c) 2002 by the person(s) listed below.  All rights reserved.
 *
 * Glenn Maynard
 */
