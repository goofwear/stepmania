#ifndef RAGE_SOUND_WAVEOUT
#define RAGE_SOUND_WAVEOUT

#include "RageSoundDriver.h"
#include "RageThreads.h"
#include "windows.h"
#include "Mmsystem.h"
#include "RageTimer.h"

class RageSound_WaveOut: public RageSoundDriver
{
	struct sound {
	    RageSound *snd;
		RageTimer start_time;

		bool stopping;

		int flush_pos; /* state == STOPPING only */

	    sound() { snd = NULL; stopping=false; }
	};

	/* List of currently playing sounds: */
	vector<sound *> sounds;

	HWAVEOUT wo;
	HANDLE sound_event;
	WAVEHDR buffers[8];
	bool shutdown;
	int last_cursor_pos;

	static int MixerThread_start(void *p);
	void MixerThread();
	RageThread MixingThread;

	bool GetData();
	void Update(float delta);

	/* virtuals: */
	void StartMixing(RageSound *snd);	/* used by RageSound */
	void StopMixing(RageSound *snd);		/* used by RageSound */
	int GetPosition(const RageSound *snd) const;
	float GetPlayLatency() const;

public:
	RageSound_WaveOut();
	~RageSound_WaveOut();
};

#endif

/*
 * Copyright (c) 2002 by the person(s) listed below.  All rights reserved.
 *
 * Glenn Maynard
 */
