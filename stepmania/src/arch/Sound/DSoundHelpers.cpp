#include "../../stdafx.h"
#include "DSoundHelpers.h"
#include "../../RageUtil.h"
#include "../../RageLog.h"

#define DIRECTSOUND_VERSION 0x0800
#include <mmsystem.h>
#include <dsound.h>

#include <math.h>

#pragma comment(lib, "dsound.lib")
#pragma comment(lib, "dxguid.lib")

DSound::DSound()
{
	HRESULT hr;

	if(FAILED(hr=DirectSoundCreate8(NULL, &ds8, NULL)))
		RageException::ThrowNonfatal(hr_ssprintf(hr, "DirectSoundCreate8"));

	/* Try to set primary mixing privileges */
	hr = ds8->SetCooperativeLevel(GetDesktopWindow(), DSSCL_PRIORITY);
}

DSound::~DSound()
{
	ds8->Release();
}

bool DSound::IsEmulated() const
{
	/* Don't bother wasting time trying to create buffers if we're
 	 * emulated.  This also gives us better diagnostic information. */
	DSCAPS Caps;
	Caps.dwSize = sizeof(Caps);
	HRESULT hr;
	if(FAILED(hr = ds8->GetCaps(&Caps)))
	{
		LOG->Warn(hr_ssprintf(hr, "ds8->GetCaps failed"));
		/* This is strange, so let's be conservative. */
		return true;
	}

	return !!(Caps.dwFlags & DSCAPS_EMULDRIVER);
}

DSoundBuf::DSoundBuf(DSound &ds, DSoundBuf::hw hardware,
			int channels_, int samplerate_, int samplebits_, int writeahead_)
{
	channels = channels_;
	samplerate = samplerate_;
	samplebits = samplebits_;
	writeahead = writeahead_;
	buffer_locked = false;
	last_cursor_pos = write_cursor = LastPosition = 0;

	/* The size of the actual DSound buffer.  This can be large; we generally
	 * won't fill it completely. */
	buffersize = 1024*64;

	WAVEFORMATEX waveformat;
	memset(&waveformat, 0, sizeof(waveformat));
	waveformat.cbSize = sizeof(waveformat);
	waveformat.wFormatTag = WAVE_FORMAT_PCM;

	int bytes = samplebits/8;
	waveformat.wBitsPerSample = WORD(samplebits);
	waveformat.nChannels = WORD(channels);
	waveformat.nSamplesPerSec = DWORD(samplerate);
	waveformat.nBlockAlign = WORD(bytes*channels);
	waveformat.nAvgBytesPerSec = samplerate * bytes*channels;

	/* Try to create the secondary buffer */
	DSBUFFERDESC format;
	memset(&format, 0, sizeof(format));
	format.dwSize = sizeof(format);
	format.dwFlags = DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_GLOBALFOCUS | DSBCAPS_CTRLVOLUME;
	/* Don't use DSBCAPS_STATIC.  It's meant for static buffers, and we
	 * only use streaming buffers. */
	if(hardware == HW_HARDWARE)
		format.dwFlags |= DSBCAPS_LOCHARDWARE;
	else
		format.dwFlags |= DSBCAPS_LOCSOFTWARE;

	format.dwBufferBytes = buffersize;
	format.dwReserved = 0;
	format.lpwfxFormat = &waveformat;

	IDirectSoundBuffer *sndbuf_buf;
	HRESULT hr = ds.GetDS8()->CreateSoundBuffer(&format, &sndbuf_buf, NULL);
	if (FAILED(hr))
		RageException::ThrowNonfatal(hr_ssprintf(hr, "CreateSoundBuffer failed"));

	sndbuf_buf->QueryInterface(IID_IDirectSoundBuffer8, (LPVOID*) &buf);
	ASSERT(buf);

	/* I'm not sure this should ever be needed, but ... */
	DSBCAPS bcaps;
	bcaps.dwSize=sizeof(bcaps);
	hr = buf->GetCaps(&bcaps);
	if(FAILED(hr))
		RageException::Throw(hr_ssprintf(hr, "buf->GetCaps"));
	if(int(bcaps.dwBufferBytes) != buffersize)
	{
		LOG->Warn("bcaps.dwBufferBytes (%i) != buffersize(%i); adjusting",
			bcaps.dwBufferBytes, buffersize);
		buffersize = bcaps.dwBufferBytes;
		writeahead = min(writeahead, buffersize);
	}
}

void DSoundBuf::SetVolume(float vol)
{
	ASSERT(vol >= 0);
	ASSERT(vol <= 1);

	float vl2 = log10f(vol) / log10f(2); /* vol log 2 */

	/* Volume is a multiplier; SetVolume wants attenuation in thousands of a
	 * decibel. */
	if(vol != 1)
		buf->SetVolume(max(int(1000 * vl2), DSBVOLUME_MIN));
}

DSoundBuf::~DSoundBuf()
{
	buf->Release();
}

bool DSoundBuf::get_output_buf(char **buffer, unsigned *bufsiz, int *play_pos, int chunksize)
{
	ASSERT(!buffer_locked);

	DWORD cursor, junk, write;

	HRESULT result;

	result = buf->GetCurrentPosition(&cursor, &write);
	if ( result == DSERR_BUFFERLOST ) {
		buf->Restore();
		result = buf->GetCurrentPosition(&cursor, &write);
	}
	if ( result != DS_OK ) {
		LOG->Warn(hr_ssprintf(result, "DirectSound::GetCurrentPosition failed"));
		return false;
	}

	int num_bytes_empty = cursor - write_cursor;
	if(num_bytes_empty <= 0) num_bytes_empty += buffersize; /* unwrap */

	/* XXX We can figure out if we've underrun, and increase the write-ahead
	 * when it happens.  Two problems:
	 * 1. It's ugly to wait until we actually underrun. (We could store the
	 *    write-ahead, though.)
	 * 2. We don't want a random underrun (eg. virus scanner starts) to
	 *    permanently increase our write-ahead.  We want the smallest possible
	 *    that will give us reliable audio in normal conditions.  I'm not sure
	 *    of a robust way to do this.  We could decrease the buffer size if
	 *    we seem to be consistently ahead, but that's getting a little messy ...
	 *
	 * Also, writeahead should be a static (all buffers write ahead the same
	 * amount); writeahead in the ctor should be a hint only (initial value),
	 * and the sound driver should query a sound to get the current writeahead
	 * in GetLatencySeconds().
	 */
#if 0
	{
	    /* Figure out the amount of space we're not supposed to write to: */
	    int unwritable = write-cursor;
	    if(unwritable < 0) unwritable += buffersize; /* unwrap */

	    if(writeahead < unwritable)
	    {
			writeahead = unwritable*2;
			LOG->Trace("boosted buffersize to %i", writeahead);
	    }

	    /* */
	    if(num_bytes_empty > buffersize - unwritable)
	    {
//			writeahead += 512;
//			LOG->Trace("underflow; bs now %i", writeahead);
	    }
	}
#endif

	int num_bytes_filled = buffersize - num_bytes_empty;
	if(num_bytes_filled > writeahead)
		return false;

	/* num_bytes_empty is now the actual amount of free buffer space.  If it's
	 * too small, come back later. */
	if(num_bytes_empty < chunksize)
		return false;

	/* I don't want to deal with DSound's split-circular-buffer locking stuff, so cap
	 * the writing space at the end of the physical buffer. */
	num_bytes_empty = min(num_bytes_empty, buffersize - write_cursor);

	/* Don't fill more than one chunk at a time.  This reduces the maximum
	 * amount of time until we give data; that way, if we're short on time,
	 * we'll give some data soon instead of lots of data later. */
	num_bytes_empty = min(num_bytes_empty, chunksize);

//	LOG->Trace("gave %i at %i (%i, %i) %i filled", num_bytes_empty, write_cursor, cursor, write, num_bytes_filled );

	/* Lock the audio buffer. */
	result = buf->Lock(write_cursor, num_bytes_empty, (LPVOID *)buffer, (DWORD *) bufsiz, NULL, &junk, 0);
	if ( result == DSERR_BUFFERLOST ) {
		buf->Restore();
		result = buf->Lock(write_cursor, num_bytes_empty, (LPVOID *)buffer, (DWORD *) bufsiz, NULL, &junk, 0);
	}
	if ( result != DS_OK ) {
		LOG->Warn(hr_ssprintf(result, "Couldn't lock the DirectSound buffer."));
		return false;
	}

	write_cursor += num_bytes_empty;
	if(write_cursor >= buffersize) write_cursor -= buffersize;

	*play_pos = last_cursor_pos;
	
	/* Increment last_cursor_pos to point at where the data we're about to
	 * ask for will actually be played. */
	last_cursor_pos += num_bytes_empty / samplesize();

	buffer_locked = true;

	return true;
}

void DSoundBuf::release_output_buf(char *buffer, unsigned bufsiz)
{
	buf->Unlock(buffer, bufsiz, NULL, 0);
	buffer_locked = false;
}

int DSoundBuf::GetPosition() const
{
	DWORD cursor, junk;
	buf->GetCurrentPosition(&cursor, &junk);
	int cursor_frames = int(cursor) / samplesize();
	int write_cursor_frames = write_cursor  / samplesize();

	int frames_behind = write_cursor_frames - cursor_frames;
	if(frames_behind <= 0)
		frames_behind += buffersize_frames(); /* unwrap */

	int ret = last_cursor_pos - frames_behind;

	/* Failsafe: never return a value smaller than we've already returned.
	 * This can happen once in a while in underrun conditions. */
	ret = max(LastPosition, ret);
	LastPosition = ret;

	return ret;
}

void DSoundBuf::Play()
{
	buf->Play(0, 0, DSBPLAY_LOOPING);
}

void DSoundBuf::Stop()
{
	buf->Stop();
	buf->SetCurrentPosition(0);
	last_cursor_pos = LastPosition = write_cursor = 0;
}


void DSoundBuf::Reset()
{
	/* Nothing is playing.  Reset the sample count; this is just to
     * prevent eventual overflow. */
	last_cursor_pos = LastPosition = 0;
}

/*
 * Copyright (c) 2002 by the person(s) listed below.  All rights reserved.
 *
 * Glenn Maynard
 */
