#include "global.h"

/*
 * Known problems:
 * I hear a click in one speaker at the beginning of some MP3s.  This is probably
 * something wrong with my SDL_sound MAD wrapper ...
 */

#ifdef DEBUG
#pragma comment(lib, "SDL_sound-1.0.0/lib/sdl_sound_static_d.lib")
#else
#pragma comment(lib, "SDL_sound-1.0.0/lib/sdl_sound_static.lib")
#endif

#pragma comment(lib, "SDL_sound-1.0.0/lib/ogg_static.lib")
#pragma comment(lib, "SDL_sound-1.0.0/lib/vorbis_static.lib")
#pragma comment(lib, "SDL_sound-1.0.0/lib/vorbisfile_static.lib")

#include "RageSoundReader_SDL_Sound.h"

const int channels = 2;
const int samplesize = 2 * channels; /* 16-bit */
const int samplerate = 44100;

/* The amount of data to read from SDL_sound at once. */
const int read_block_size = 1024;

bool SoundReader_SDL_Sound::Open(CString filename_)
{
	filename=filename_;
	static bool initialized = false;
	if(!initialized)
	{
		if(!Sound_Init())
			RageException::Throw( "RageSoundManager::RageSoundManager: error initializing sound loader: %s", Sound_GetError());
		initialized = true;
	}

	inbuf = NULL;
	avail = 0;

	Sound_AudioInfo sound_desired;
	sound_desired.channels = channels;
	sound_desired.format = AUDIO_S16SYS;
	sound_desired.rate = 0; // samplerate;

    Sample = Sound_NewSampleFromFile(filename.GetString(),
                    &sound_desired, read_block_size);

	resamp.reset();
	resamp.SetInputChannels(2);
	resamp.SetOutputChannels(2);
	resamp.SetInputSampleRate(Sample->actual.rate);
	resamp.SetOutputSampleRate(samplerate);
	if(Sample)
		return true;
	
	SetError(Sound_GetError());
	return false;
}

int SoundReader_SDL_Sound::GetLength() const
{
	int len = Sound_Length(Sample);
	if(len == -1 && Sample->flags & SOUND_SAMPLEFLAG_EAGAIN) {
		/* This indicates the length check will take a little while; call
		 * it again to confirm. */
		len = Sound_Length(Sample);
	}

	if(len < 0)
	{
		SetError(Sound_GetError());
		return -1;
	}

	return len;
}

int SoundReader_SDL_Sound::GetLength_Fast() const
{
	int ret = Sound_Length(Sample);

	Sound_Rewind(Sample);
	return ret;
}

int SoundReader_SDL_Sound::SetPosition(int ms, bool accurate)
{
	resamp.reset();
	if(ms == 0)
	{
		Sound_Rewind(Sample);
		return 0;
	}

	int ret;
	if(accurate)
		ret = Sound_AccurateSeek(Sample, ms);
	else
		ret = Sound_FastSeek(Sample, ms);

	if(Sample->flags & SOUND_SAMPLEFLAG_ERROR)
	{
		SetError(Sound_GetError());
		return -1;
	}

	/* If the seek function returned false but didn't set error, it's a
	 * recoverable error.  We probably seeked past end of file. */
	if(!ret)
	{
		/* Past EOF.  Rewind. */
		Sound_Rewind(Sample);
		return 0;
	}


	return ms;
}

int SoundReader_SDL_Sound::Read(char *buf, unsigned len)
{
	int bytes_read = 0;
	while(len)
	{
		int size = resamp.read(buf, len);

		if(size == -1)
			break;

		buf += size;
		len -= size;
		bytes_read += size;

		if(size == 0)
		{
			/* Don't read at all if we've already hit EOF (it'll whine). */
			if(Sample->flags & SOUND_SAMPLEFLAG_EOF)
				return bytes_read; /* EOF */

			int cnt = Sound_Decode(Sample);

			if(Sample->flags & SOUND_SAMPLEFLAG_EOF)
			{
				resamp.eof();
				continue;
			}

			if(Sample->flags & SOUND_SAMPLEFLAG_ERROR)
			{
				SetError(Sound_GetError());
				return -1;
			}

			resamp.write(Sample->buffer, cnt);
		}
	}

	return bytes_read;
}

SoundReader_SDL_Sound::~SoundReader_SDL_Sound()
{
	Sound_FreeSample(Sample);
}

SoundReader *SoundReader_SDL_Sound::Copy() const
{
	SoundReader_SDL_Sound *ret = new SoundReader_SDL_Sound;
	ret->Open(filename);
	return ret;
}

/*
-----------------------------------------------------------------------------
 Copyright (c) 2002-2003 by the person(s) listed below.  All rights reserved.
	Glenn Maynard
-----------------------------------------------------------------------------
*/
