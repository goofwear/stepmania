#ifndef RAGE_SOUND_WAVEOUT
#define RAGE_SOUND_WAVEOUT

#include "RageSoundDriver.h"
#include "RageThreads.h"
#include "RageTimer.h"

class RageSound_OSS: public RageSoundDriver
{
	int fd;

	struct sound {
	    RageSound *snd;
		RageTimer start_time;

		bool stopping;

		int flush_pos; /* state == STOPPING only */

	    sound() { snd = NULL; stopping=false; }
	};

	/* List of currently playing sounds: */
	vector<sound *> sounds;

	bool shutdown;
	int last_cursor_pos;
	int samplerate;

	static int MixerThread_start(void *p);
	void MixerThread();
	RageThread MixingThread;

public:
	bool GetData();
	void Update(float delta);
	int GetSampleRate( int rate ) const { return samplerate; }

	/* virtuals: */
	void StartMixing(RageSound *snd);	/* used by RageSound */
	void StopMixing(RageSound *snd);		/* used by RageSound */
	int GetPosition(const RageSound *snd) const;
	float GetPlayLatency() const;

	RageSound_OSS();
	~RageSound_OSS();
};

#endif

/*
 * Copyright (c) 2002 by the person(s) listed below.  All rights reserved.
 *
 * Glenn Maynard
 */
