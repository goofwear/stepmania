#include "../../global.h"
#include "DSoundHelpers.h"
#include "../../RageUtil.h"
#include "../../RageLog.h"

#if defined(_WINDOWS)
#include <mmsystem.h>
#endif
#define DIRECTSOUND_VERSION 0x0700
#include <dsound.h>

#include <math.h>

#pragma comment(lib, "dsound.lib")

BOOL CALLBACK DSound::EnumCallback( LPGUID lpGuid, LPCSTR lpcstrDescription, LPCSTR lpcstrModule, LPVOID lpContext )
{
	LOG->Info( "DirectSound Driver: %s (%s)", lpcstrDescription, lpcstrModule );
	if(lpGuid)
		LOG->Info( "    ID: {%8.8x-%4.4x-%4.4x-%6.6x}", lpGuid->Data1, lpGuid->Data2, lpGuid->Data3, lpGuid->Data4 );

	return TRUE;
}

void DSound::SetPrimaryBufferMode()
{
#ifndef _XBOX
	DSBUFFERDESC format;
	memset( &format, 0, sizeof(format) );
	format.dwSize = sizeof(format);
	format.dwFlags = DSBCAPS_PRIMARYBUFFER;
	format.dwBufferBytes = 0;
	format.lpwfxFormat = NULL;

	IDirectSoundBuffer *buf;
	HRESULT hr = this->GetDS()->CreateSoundBuffer(&format, &buf, NULL);
	/* hr */
	if( FAILED(hr) )
	{
		LOG->Warn(hr_ssprintf(hr, "Couldn't create primary buffer"));
		return;
	}

    WAVEFORMATEX waveformat;
    waveformat.cbSize = 0;
    waveformat.wFormatTag = WAVE_FORMAT_PCM;
	waveformat.wBitsPerSample = WORD(16);
	waveformat.nChannels = WORD(2);
	waveformat.nSamplesPerSec = DWORD(44100);
	waveformat.nBlockAlign = WORD(4);
	waveformat.nAvgBytesPerSec = 44100 * waveformat.nBlockAlign;

	// Set the primary buffer's format
    hr = IDirectSoundBuffer_SetFormat( buf, &waveformat );
	if( FAILED(hr) )
		LOG->Warn( hr_ssprintf(hr, "SetFormat on primary buffer") );

	/* MS docs:
	 *
	 * When there are no sounds playing, DirectSound stops the mixer engine and halts DMA 
	 * (direct memory access) activity. If your application has frequent short intervals of
	 * silence, the overhead of starting and stopping the mixer each time a sound is played
	 * may be worse than the DMA overhead if you kept the mixer active. Also, some sound
	 * hardware or drivers may produce unwanted audible artifacts from frequent starting and
	 * stopping of playback. If your application is playing audio almost continuously with only
	 * short breaks of silence, you can force the mixer engine to remain active by calling the
	 * IDirectSoundBuffer::Play method for the primary buffer. The mixer will continue to run
	 * silently.
	 *
	 * However, I just added the above code and I don't want to change more until it's tested.
	 */
//	buf->Play(0, 0, DSBPLAY_LOOPING);

	buf->Release();
#endif
}

DSound::DSound()
{
	HRESULT hr;

#ifndef _XBOX
    // Initialize COM
    if( FAILED( hr = CoInitialize( NULL ) ) )
		RageException::ThrowNonfatal(hr_ssprintf(hr, "CoInitialize"));
#endif

    // Create IDirectSound using the primary sound device
    if( FAILED( hr = DirectSoundCreate( NULL, &ds, NULL ) ) )
		RageException::ThrowNonfatal(hr_ssprintf(hr, "DirectSoundCreate"));

#ifndef _XBOX
	DirectSoundEnumerate(EnumCallback, 0);
	
	/* Try to set primary mixing privileges */
	hr = ds->SetCooperativeLevel( GetDesktopWindow(), DSSCL_PRIORITY );
#endif

	SetPrimaryBufferMode();
}

DSound::~DSound()
{
	ds->Release();
#ifndef _XBOX
    CoUninitialize();
#endif
}

bool DSound::IsEmulated() const
{
#ifndef _XBOX
	/* Don't bother wasting time trying to create buffers if we're
 	 * emulated.  This also gives us better diagnostic information. */
	DSCAPS Caps;
	Caps.dwSize = sizeof(Caps);
	HRESULT hr;
	if( FAILED(hr = ds->GetCaps(&Caps)) )
	{
		LOG->Warn( hr_ssprintf(hr, "ds->GetCaps failed") );
		/* This is strange, so let's be conservative. */
		return true;
	}

	return !!(Caps.dwFlags & DSCAPS_EMULDRIVER);
#else
	return false;
#endif
}

DSoundBuf::DSoundBuf( DSound &ds, DSoundBuf::hw hardware,
					  int channels_, int samplerate_, int samplebits_, int writeahead_ )
{
	channels = channels_;
	samplerate = samplerate_;
	samplebits = samplebits_;
	writeahead = writeahead_ * bytes_per_frame();
	volume = 0;
	buffer_locked = false;
	last_cursor_pos = write_cursor = buffer_bytes_filled = 0;
	LastPosition = 0;
	playing = false;

	/* The size of the actual DSound buffer.  This can be large; we generally
	 * won't fill it completely. */
	buffersize = 1024*64;
	buffersize = max( buffersize, writeahead );

	WAVEFORMATEX waveformat;
	memset( &waveformat, 0, sizeof(waveformat) );
	waveformat.cbSize = sizeof(waveformat);
	waveformat.wFormatTag = WAVE_FORMAT_PCM;

	bool NeedCtrlFrequency = false;
	if( samplerate == DYNAMIC_SAMPLERATE )
	{
		samplerate = 44100;
		NeedCtrlFrequency = true;
	}

	int bytes = samplebits/8;
	waveformat.wBitsPerSample = WORD(samplebits);
	waveformat.nChannels = WORD(channels);
	waveformat.nSamplesPerSec = DWORD(samplerate);
	waveformat.nBlockAlign = WORD(bytes*channels);
	waveformat.nAvgBytesPerSec = samplerate * bytes*channels;

	/* Try to create the secondary buffer */
	DSBUFFERDESC format;
	memset( &format, 0, sizeof(format) );
	format.dwSize = sizeof(format);
#ifdef _XBOX
	format.dwFlags = 0;
#else
	format.dwFlags = DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_GLOBALFOCUS | DSBCAPS_CTRLVOLUME;
#endif
	
#ifndef _XBOX
	/* Don't use DSBCAPS_STATIC.  It's meant for static buffers, and we
	 * only use streaming buffers. */
	if( hardware == HW_HARDWARE )
		format.dwFlags |= DSBCAPS_LOCHARDWARE;
	else
		format.dwFlags |= DSBCAPS_LOCSOFTWARE;
#endif

	if( NeedCtrlFrequency )
		format.dwFlags |= DSBCAPS_CTRLFREQUENCY;

	format.dwBufferBytes = buffersize;
#ifndef _XBOX
	format.dwReserved = 0;
#else
	DSMIXBINVOLUMEPAIR dsmbvp[8] =
	{
		{ DSMIXBIN_FRONT_LEFT,		DSBVOLUME_MAX}, // left channel
		{ DSMIXBIN_FRONT_RIGHT,		DSBVOLUME_MAX}, // right channel
		{ DSMIXBIN_FRONT_CENTER,	DSBVOLUME_MAX}, // left channel
		{ DSMIXBIN_FRONT_CENTER,	DSBVOLUME_MAX}, // right channel
		{ DSMIXBIN_BACK_LEFT,		DSBVOLUME_MAX}, // left channel
		{ DSMIXBIN_BACK_RIGHT,		DSBVOLUME_MAX}, // right channel
		{ DSMIXBIN_LOW_FREQUENCY,	DSBVOLUME_MAX}, // left channel
		{ DSMIXBIN_LOW_FREQUENCY,	DSBVOLUME_MAX}  // right channel
	};
	DSMIXBINS dsmb;
	dsmb.dwMixBinCount = 8;
	dsmb.lpMixBinVolumePairs = dsmbvp;

	format.lpMixBins			= &dsmb;
#endif

	format.lpwfxFormat = &waveformat;

	HRESULT hr = ds.GetDS()->CreateSoundBuffer( &format, &buf, NULL );
	if( FAILED(hr) )
		RageException::ThrowNonfatal( hr_ssprintf(hr, "CreateSoundBuffer failed") );

#ifndef _XBOX
	/* I'm not sure this should ever be needed, but ... */
	DSBCAPS bcaps;
	bcaps.dwSize=sizeof(bcaps);
	hr = buf->GetCaps(&bcaps);
	if( FAILED(hr) )
		RageException::Throw(hr_ssprintf(hr, "buf->GetCaps"));
	if( int(bcaps.dwBufferBytes) != buffersize )
	{
		LOG->Warn( "bcaps.dwBufferBytes (%i) != buffersize(%i); adjusting", bcaps.dwBufferBytes, buffersize );
		buffersize = bcaps.dwBufferBytes;
		writeahead = min( writeahead, buffersize );
	}
#endif
}

void DSoundBuf::SetSampleRate(int hz)
{
	samplerate = hz;
	HRESULT hr = buf->SetFrequency( hz );
	if( FAILED(hr) )
		RageException::Throw( hr_ssprintf(hr, "buf->SetFrequency(%i)", hz) );
}

void DSoundBuf::SetVolume(float vol)
{
	ASSERT(vol >= 0);
	ASSERT(vol <= 1);
	
	if( vol == 0 )
		vol = 0.001f;		// fix log10f(0) == -INF
	float vl2 = log10f(vol) / log10f(2); /* vol log 2 */

	/* Volume is a multiplier; SetVolume wants attenuation in thousands of a decibel. */
	const int new_volume = max( int(1000 * vl2), DSBVOLUME_MIN );

	if( volume == new_volume )
		return;
	volume = new_volume;

	buf->SetVolume( volume );
}

/* Determine if "pos" is between "start" and "end", for a circular buffer. */
static bool contained( int start, int end, int pos )
{
	if( end >= start ) /* start ... pos ... end */
		return start <= pos && pos <= end;
	else
		return pos >= start || pos <= end;
}

DSoundBuf::~DSoundBuf()
{
	buf->Release();
}

bool DSoundBuf::get_output_buf( char **buffer, unsigned *bufsiz, int chunksize )
{
	ASSERT(!buffer_locked);

	chunksize *= bytes_per_frame();

	DWORD cursorstart, cursorend;

	HRESULT result;

	/* It's easiest to think of the cursor as a block, starting and ending at
	 * the two values returned by GetCurrentPosition, that we can't write to. */
	result = buf->GetCurrentPosition( &cursorstart, &cursorend );
#ifndef _XBOX
	if ( result == DSERR_BUFFERLOST )
	{
		buf->Restore();
		result = buf->GetCurrentPosition( &cursorstart, &cursorend );
	}
	if ( result != DS_OK )
	{
		LOG->Warn( hr_ssprintf(result, "DirectSound::GetCurrentPosition failed") );
		return false;
	}
#endif

	/* Some cards (Creative AudioPCI) have a no-write area even when not playing.  I'm not
	 * sure what that means, but it breaks the assumption that we can fill the whole writeahead
	 * when prebuffering. */
	if( !playing )
		cursorend = cursorstart;

	/* Update buffer_bytes_filled. */
	{
		int first_byte_filled = write_cursor-buffer_bytes_filled;
		if( first_byte_filled < 0 )
			first_byte_filled += buffersize; /* unwrap */

		/* The number of bytes that have been played since the last time we got here: */
		int bytes_played = cursorstart - first_byte_filled;
		if( bytes_played < 0 )
			bytes_played += buffersize; /* unwrap */

		buffer_bytes_filled -= bytes_played;
		buffer_bytes_filled = max( 0, buffer_bytes_filled );
	}

	/* XXX We can figure out if we've underrun, and increase the write-ahead
	 * when it happens.  Two problems:
	 * 1. It's ugly to wait until we actually underrun. (We could store the
	 *    write-ahead, though.)
	 * 2. We don't want a random underrun (eg. virus scanner starts) to
	 *    permanently increase our write-ahead.  We want the smallest possible
	 *    that will give us reliable audio in normal conditions.  I'm not sure
	 *    of a robust way to do this.
	 *
	 * Also, writeahead should be a static (all buffers write ahead the same
	 * amount); writeahead in the ctor should be a hint only (initial value),
	 * and the sound driver should query a sound to get the current writeahead
	 * in GetLatencySeconds().
	 */

	{
		int first_byte_filled = write_cursor-buffer_bytes_filled;
		if( first_byte_filled < 0 )
			first_byte_filled += buffersize; /* unwrap */

		/* Data between the play cursor and the write cursor is committed to be
		 * played.  If we don't actually have data there, we've underrun. 
		 * (If buffer_bytes_filled == buffersize, then it's full, and can't be
		 * an underrun.) 
		 */

		/* We're already underrunning, which means the play cursor has passed valid
		 * data.  Let's move the cursor forward. */
		if( buffer_bytes_filled < buffersize &&
		   (!contained(first_byte_filled, write_cursor, cursorstart) ||
		    !contained(first_byte_filled, write_cursor, cursorend)) )
		{
			int missed_by = cursorend - write_cursor;
			wrap( missed_by, buffersize );
			LOG->Trace("underrun %p: %i..%i filled but cursor at %i..%i (missed it by %i) %i/%i",
				this, first_byte_filled, write_cursor, cursorstart, cursorend,
				missed_by, buffer_bytes_filled, buffersize);

			/* Pretend the space between the play and write cursor is filled
			 * with data, and continue filling from there. */
			int no_write_zone_size = cursorend - cursorstart;
			if( no_write_zone_size < 0 )
				no_write_zone_size += buffersize; /* unwrap */

			buffer_bytes_filled = no_write_zone_size;
			write_cursor = cursorend;
		}
	}


	/* If we already have enough bytes written ahead, stop. */
	if( buffer_bytes_filled > writeahead )
		return false;

	int num_bytes_empty = writeahead-buffer_bytes_filled;

	/* num_bytes_empty is the amount of free buffer space.  If it's
	 * too small, come back later. */
	if( num_bytes_empty < chunksize )
		return false;

	/* I don't want to deal with DSound's split-circular-buffer locking stuff, so clamp
	 * the writing space at the end of the physical buffer. */
	num_bytes_empty = min(num_bytes_empty, buffersize - write_cursor);

	/* Don't fill more than one chunk at a time.  This reduces the maximum
	 * amount of time until we give data; that way, if we're short on time,
	 * we'll give some data soon instead of lots of data later. */
	num_bytes_empty = min(num_bytes_empty, chunksize);

//	LOG->Trace("gave %i at %i (%i, %i) %i filled", num_bytes_empty, write_cursor, cursor, write, buffer_bytes_filled);

	/* Lock the audio buffer. */
#ifdef _XBOX
	result = buf->Lock( write_cursor, num_bytes_empty, (LPVOID *)buffer, (DWORD *) bufsiz, NULL, NULL, 0 );
#else
	DWORD junk;
	result = buf->Lock( write_cursor, num_bytes_empty, (LPVOID *)buffer, (DWORD *) bufsiz, NULL, &junk, 0 );
#endif

#ifndef _XBOX
	if ( result == DSERR_BUFFERLOST )
	{
		buf->Restore();
		result = buf->Lock( write_cursor, num_bytes_empty, (LPVOID *)buffer, (DWORD *) bufsiz, NULL, &junk, 0 );
	}
#endif
	if ( result != DS_OK )
	{
		LOG->Warn( hr_ssprintf(result, "Couldn't lock the DirectSound buffer.") );
		return false;
	}

	write_cursor += num_bytes_empty;
	if( write_cursor >= buffersize )
		write_cursor -= buffersize;

	buffer_bytes_filled += num_bytes_empty;

	/* Increment last_cursor_pos to point at where the data we're about to
	 * ask for will actually be played. */
	last_cursor_pos += num_bytes_empty / bytes_per_frame();
	buffer_locked = true;

	return true;
}

void DSoundBuf::release_output_buf( char *buffer, unsigned bufsiz )
{
	buf->Unlock( buffer, bufsiz, NULL, 0 );
	buffer_locked = false;
}

int64_t DSoundBuf::GetPosition() const
{
	DWORD cursor, junk;
	buf->GetCurrentPosition( &cursor, &junk );
	int cursor_frames = int(cursor) / bytes_per_frame();
	int write_cursor_frames = write_cursor  / bytes_per_frame();

	int frames_behind = write_cursor_frames - cursor_frames;
	/* frames_behind will be 0 if we're called before the buffer starts playing:
	 * both write_cursor_frames and cursor_frames will be 0. */
	if( frames_behind < 0 )
		frames_behind += buffersize_frames(); /* unwrap */

	int64_t ret = last_cursor_pos - frames_behind;

	/* Failsafe: never return a value smaller than we've already returned.
	 * This can happen once in a while in underrun conditions. */
	ret = max( LastPosition, ret );
	LastPosition = ret;

	return ret;
}

void DSoundBuf::Play()
{
	if( playing )
		return;
	buf->Play( 0, 0, DSBPLAY_LOOPING );
	playing = true;
}

void DSoundBuf::Stop()
{
	if( !playing )
		return;

	buf->Stop();
	buf->SetCurrentPosition(0);

	last_cursor_pos = write_cursor = buffer_bytes_filled = 0;
	LastPosition = 0;

	/* When stopped and rewound, the play and write cursors should both be 0. */
	DWORD play, write;
	buf->GetCurrentPosition( &play, &write );
	RAGE_ASSERT_M( play == 0 && write == 0, ssprintf("%i, %i", play, write) );

	playing = false;
}

/*
 * Copyright (c) 2002 by the person(s) listed below.  All rights reserved.
 *
 * Glenn Maynard
 */
