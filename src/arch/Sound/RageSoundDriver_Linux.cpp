/* Rage Sound Driver for Linux using ALSA
 * (C) 2003 Tim Hentenaar
 */

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#define ALSA_PCM_NEW_HW_PARAMS_API
#include <alsa/asound.h>

#include "RageSoundDriver_Linux.h"
#include "RageTimer.h"
#include "RageLog.h"
#include "RageSound.h"
#include "RageUtil.h"
#include "SDL.h"
#include "SDL_Thread.h"

/* Get the chunk size */
#define CHUNKSIZE(x,y) y = x / 8;

/* Buffer size in frames */
#define BUFSIZE_F(x,y) y = x / 2;

/* Minimum Buffer Size (bytes) */
#define BUFSIZE(x) if (x < 1024*16) x = 1024*16;

void RageSound_Linux::OpenAudio() {
	/* Open Device [default hardware sound device] (non-blocking) */
	snd_pcm_open(&playback_handle, "hw", SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK);
	
	/* Set up the device parameter struct */
	snd_pcm_hw_params_t *hw_params;
	snd_pcm_hw_params_malloc(&hw_params);
	snd_pcm_hw_params_any(&hw_params);

	/* Non-Interleaved Access (since we hold the samples in a buffer) */
	snd_pcm_hw_params_set_access(playback_handle,hw_params,SND_PCM_ACCESS_RW_NONINTERLEAVED);
	
	/* Set the audio format */
	
	/* 16-bit signed CPU-Endian */
	snd_pcm_hw_params_set_format(playback_handle,hw_params,SND_PCM_FORMAT_S16);    
	
	/* 44100 Hz Rate (or closest supported match) */
	snd_pcm_hw_params_set_rate_near(playback_handle,hw_params,44100,0);
	
	/* 2 channels (stereo) */
	snd_pcm_hw_params_set_channels(playback_handle,hw_params,2);

	/* Set Sample Size (channels * 2) */
	snd_pcm_hw_params_set_period_size_near(playback_handle,hw_params,4,NULL);

	/* Get hardware Buffer Size and allocate buffer 
	 * It should be as big as the hardware buffer or 8K whichever 
	 * is larger 
	 */
	snd_pcm_hw_params_get_buffer_size(params, &buffer_size);
	sbuffer = (Sint16 *)malloc(BUFSIZE(buffer_size)); /* snd_pcm_sframes_t = long */

	/* Apply the settings to the device */
	snd_pcm_hw_params(playback_handle,hw_params);

	/* free hw_params struct */
	snd_pcm_hw_params_free(hw_params);
}

void RageSound_Linux::CloseAudio() {
	/* close the PCM */
	snd_pcm_close(playback_handle);

	if (sbuffer) free(sbuffer); /* free sound buffer */
	sbuffer = NULL;
}

void RageSound_Linux::RecoverState(long state) {
	/* Revcover from suspended/overflowed state 
	 * by re-preparing the device for use
	*/

	int err = 0;
	
	switch((int)state) {
		case -EPIPE: /* buffer under-run */
			if (snd_pcm_prepare(playback_handle) < 0) 
				LOG->Trace("RageSound_Linux::RecoverState(): Unable to recover from sound buffer underrun!");
		break;
		case -ESTRPIPE: /* suspend */
			if (err = snd_pcm_resume(playback_handle) == -EAGAIN) sleep(1);
			err = snd_pcm_resume(playback_handle);
			if (err < 0) RageException::Throw(CString("RageSound_Linux::RecoverState(): Unable To recover from suspend!"));
		break;
	};
	
	snd_pcm_prepare(playback_handle);
}

void RageSound_Linux::StartMixing(RageSound *snd) {
	sound *s = new sound;
	s->snd = snd;

	SDL_LockAudio();
	sounds.push_back(s);
	SDL_UnlockAudio();
}

void RageSound_Linux::StopMixing(RageSound *snd) {
	LockMutex L(SOUNDMAN->lock);

	/* Find the sound. */
	for(int i = 0; i < sounds.size(); ++i) if(sounds[i]->snd == snd) break;
	
	if(i == sounds.size())	{
		LOG->Trace("RageSound_Linux::StopMixing(): not stopping a sound because it's not playing");
		return;
	}

	delete sounds[i];
	sounds.erase(sounds.begin()+i, sounds.begin()+i+1);
}


bool RageSound_Linux::GetData() {

	LockMutex L(SOUNDMAN->lock);
	
	Sint16 *buf = NULL;

	/* Find a free chunk */

	for (int b = 0; b<8; b++) {
	  if (chunks[b] != NULL) break;
	  if (b == 8) return false;
	
	  int bufsiz = 0;
	  BUFSIZE_F(buffer_size,bufsiz);
	  bufsiz *= 16;
	  if (!buf) buf = (Sint16 *)malloc(bufsiz);
	
	  int cs = 0;
	  CHUNKSIZE(buffer_size,cs);

	  memset(buf,0,sizeof(buf));
	  memset(chunks[b],0,sizeof(chunks[b]));

	  SoundMixBuffer mix;
	
	  for (int i = 0; i<sounds.size(); i++) {
		if (sounds[i]->stopping) continue;
		/* Call the Callback */
		unsigned int got = sounds[i]->snd->GetPCM((char *)buf,cs,last_pos);

		mix.write(buf,got/2);
		
		if (got < chunksize) {
			/* sound is finishing */
			sounds[i].stopping = true;
			sounds[i].flush_pos = last_pos + (got / 16);
		}

	  }

	  mix.read(chunks[b]); /* Get the sound */

	  snd_pcm_prepare(playback_handle); /* Prepare for write */
	
	  /* Non-Interleaved Write */
	  long ret = snd_pcm_writen(playback_handle,&chunks[b],sizeof(chunks[b]));   
	  RecoverState(ret); /* Check for an error and recover */
	  last_pos += cs;
	}
	
	if (buf) free(buf);
	return true;
}

void RageSound_Linux::Update(float delta) {
	LockMutex L(SOUNDMAN->lock);

	/* SoundStopped might erase sounds out from under us, so make a copy
	 * of the sound list. */
	vector<sound *> snds = sounds;
	for(unsigned i = 0; i < snds.size(); ++i)
	{
		if(!sounds[i]->stopping) continue;

		if(GetPosition(snds[i]->snd) < sounds[i]->flush_pos) continue; /* stopping but still flushing */

		/* This sound is done. */
		snds[i]->snd->StopPlaying();
	}

}

int RageSound_Linux::GetPosition(const RageSound *snd) const {
	LockMutex L(SOUNDMAN->lock);

	/*  Get a timestamp from the stream's status */
	int err;
	snd_pcm_status_t *status;
	snd_timestamp_t *x;

        snd_pcm_status_alloca(&status);
        if ((err = snd_pcm_status(handle, status)) < 0) {
		RageException::Throw(CString(ssprintf("RageSound_Linux::GetPosition(): Stream status error: %s\n", snd_strerror(err))));
		return 0;
        }
        snd_pcm_status_get_trigger_tstamp(status, x);	

	/* Convert it to samples 
	 * To be exact, we should get the rate from the 
	 * hardware, but this should do. (The rate will
	 * be 44100 Hz anyway :P)
	 */
	
	return (x->tv_usec / 1000) * 44100;
}	

float RageSound_Linux::GetPlayLatency() const { 
	int bufsize_frames;
	BUFSIZE_F(bufsize_frames,buffer_size);
	return (float)((1.0f / 44100) * ( bufsize_frames - (bufsize_frames / 8))); 
}

RageSound_Linux::RageSound_Linux() {
	last_pos = 0;
	MixerThread = SDL_CreateThread(MixerThread_start,this);
	sound_event = CreateEvent(NULL, false, true, NULL);
}
	
RageSound_Linux::~RageSound_Linux() {
	LOG->Trace("RageSoundDriver_Linux::~RageSoundDriver_Linux(): Shutting down mixer thread ...");
	SDL_WaitThread(MixerThreadPtr, NULL);
	LOG->Trace("Mixer thread shut down.");
	CloseHandle(sound_event);
	CloseAudio();
}

	
	
	
	
	

