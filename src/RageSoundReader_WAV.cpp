#include "global.h"
#include "RageSoundReader_WAV.h"
#include "RageLog.h"
#include "RageUtil.h"

#include <SDL_endian.h>

#define BAIL_IF_MACRO(c, e, r) if (c) { SetError(e); return r; }
#define RETURN_IF_MACRO(c, r) if (c) return r;

#define riffID 0x46464952  /* "RIFF", in ascii. */
#define waveID 0x45564157  /* "WAVE", in ascii. */
#define fmtID  0x20746D66  /* "fmt ", in ascii. */
#define dataID 0x61746164  /* "data", in ascii. */

enum
{
	FMT_NORMAL= 1,           /* Uncompressed waveform data.     */
	FMT_ADPCM = 2,           /* ADPCM compressed waveform data. */
	FMT_ITU_G711_ALAW = 6,   /* ITU G.711 A-law */
	FMT_ITU_G711_MULAW = 7,  /* ITU G.711 mu-law */
	FMT_IMA_ADPCM = 17,      /* IMA ADPCM */
	FMT_ITU_G723_ADPCM = 20, /* ITU G.723 ADPCM */
	FMT_GSM_610 = 49,        /* GSM 6.10 */
	FMT_ITU_G721_ADPCM = 64, /* ITU G.721 ADPCM */
	FMT_MPEG = 80,           /* MPEG */
	FMT_MPEG_L3 = 85         /* MPEG Layer 3 */
};

/* Call this to convert milliseconds to an actual byte position, based on audio data characteristics. */
Uint32 RageSoundReader_WAV::ConvertMsToBytePos(int BytesPerSample, int channels, Uint32 ms) const
{
    const float frames_per_ms = ((float) SampleRate) / 1000.0f;
    const Uint32 frame_offset = (Uint32) (frames_per_ms * float(ms) + 0.5f);
    const Uint32 frame_size = (Uint32) BytesPerSample * channels;
    return frame_offset * frame_size;
}

Uint32 RageSoundReader_WAV::ConvertBytePosToMs(int BytesPerSample, int channels, Uint32 pos) const
{
    const Uint32 frame_size = (Uint32) BytesPerSample * channels;
    const Uint32 frame_no = pos / frame_size;
    const float frames_per_ms = ((float) SampleRate) / 1000.0f;
    return (Uint32) ((frame_no / frames_per_ms) + 0.5f);
}

/* Better than SDL_ReadLE16, since you can detect i/o errors... */
bool RageSoundReader_WAV::read_le16( RageFile &f, Sint16 *si16 ) const
{
    const int ret = f.Read( si16, sizeof(Sint16) );
	if( ret != sizeof(Sint16) )
	{
		SetError( ret >= 0? "end of file": f.GetError().c_str() );
		return false;
	}
    *si16 = SDL_SwapLE16( *si16 );
    return true;
}

bool RageSoundReader_WAV::read_le16( RageFile &f, Uint16 *ui16 ) const
{
    const int ret = f.Read( ui16, sizeof(Uint16) );
	if( ret != sizeof(Uint16) )
	{
		SetError( ret >= 0? "end of file": f.GetError().c_str() );
		return false;
	}
    *ui16 = SDL_SwapLE16(*ui16);
    return true;
}


/* Better than SDL_ReadLE32, since you can detect i/o errors... */
bool RageSoundReader_WAV::read_le32( RageFile &f, Sint32 *si32 ) const
{
    const int ret = f.Read( si32, sizeof(Sint32) );
	if( ret != sizeof(Sint32) )
	{
		SetError( ret >= 0? "end of file": f.GetError().c_str() );
		return false;
	}
    *si32 = SDL_SwapLE32( *si32 );
    return true;
}

bool RageSoundReader_WAV::read_le32( RageFile &f, Uint32 *ui32 ) const
{
    const int ret = f.Read( ui32, sizeof(Uint32) );
	if( ret != sizeof(Uint32) )
	{
		SetError( ret >= 0? "end of file": f.GetError().c_str() );
		return false;
	}
    *ui32 = SDL_SwapLE32( *ui32 );
    return true;
}

bool RageSoundReader_WAV::read_uint8( RageFile &f, Uint8 *ui8 ) const
{
    const int ret = f.Read( ui8, sizeof(Uint8) );
	if( ret != sizeof(Uint8) )
	{
		SetError( ret >= 0? "end of file": f.GetError().c_str() );
		return false;
	}
    return true;
}

RageSoundReader_WAV::adpcm_t::adpcm_t()
{
	cbSize = 0;
	memset( blockheaders, 0, sizeof(blockheaders) );
	wSamplesPerBlock = 0;
	samples_left_in_block = 0;
	nibble_state = 0;
	nibble = 0;
}


bool RageSoundReader_WAV::read_fmt_chunk()
{
    RETURN_IF_MACRO(!read_le16(rw, &fmt.wFormatTag), false);
    RETURN_IF_MACRO(!read_le16(rw, &fmt.wChannels), false);
    RETURN_IF_MACRO(!read_le32(rw, &SampleRate), false);
    RETURN_IF_MACRO(!read_le32(rw, &fmt.dwAvgBytesPerSec), false);
    RETURN_IF_MACRO(!read_le16(rw, &fmt.wBlockAlign), false);
    RETURN_IF_MACRO(!read_le16(rw, &fmt.wBitsPerSample), false);

    if( fmt.wFormatTag == FMT_ADPCM )
    {
		RETURN_IF_MACRO(!read_le16(rw, &adpcm.cbSize), false);
		RETURN_IF_MACRO(!read_le16(rw, &adpcm.wSamplesPerBlock), false);
		Uint16 NumCoef;
		RETURN_IF_MACRO(!read_le16(rw, &NumCoef), false);

		for ( int i = 0; i < NumCoef; i++ )
		{
			Sint16 c1, c2;
			RETURN_IF_MACRO(!read_le16(rw, &c1), false);
			RETURN_IF_MACRO(!read_le16(rw, &c2), false);

			adpcm.Coef1.push_back( c1 );
			adpcm.Coef2.push_back( c2 );
		}
	}

    return true;
}


int RageSoundReader_WAV::read_sample_fmt_normal(char *buf, unsigned len)
{
    const int ret = this->rw.Read( buf, len );
	if( ret == -1 )
	{
		SetError( ret >= 0? "end of file": rw.GetError().c_str() );
		return -1;
	}

    return ret;
}


int RageSoundReader_WAV::seek_sample_fmt_normal( Uint32 ms )
{
    const int offset = ConvertMsToBytePos( BytesPerSample, Channels, ms);
    const int pos = (int) (this->fmt.data_starting_offset + offset);

	const int ret = this->rw.Seek( pos );
	BAIL_IF_MACRO( ret == -1, this->rw.GetError(), -1 );

	/* If we seek past end of ifle, leave the cursor there, so subsequent reads will return EOF. */
	if( pos >= this->rw.GetFileSize() )
		return 0;

    return ms;
}

int RageSoundReader_WAV::get_length_fmt_adpcm() const
{
	int offset = this->rw.GetFileSize() - fmt.data_starting_offset;

	/* pcm bytes per block */
	const int bpb = (adpcm.wSamplesPerBlock * fmt.adpcm_sample_frame_size);
    const int blockno = offset / fmt.wBlockAlign;
    const int byteno = blockno * bpb;

    /* Seek back to the beginning of the last frame and find out how long it really is. */
	this->rw.Seek( blockno * fmt.wBlockAlign + fmt.data_starting_offset );

	/* Don't mess up this->adpcm; we'll put the cursor back as if nothing happened. */
	adpcm_t tmp_adpcm(adpcm);
	if ( !read_adpcm_block_headers(tmp_adpcm) )
		return 0;

	return ConvertBytePosToMs( BytesPerSample, Channels, byteno) + 
		   ConvertBytePosToMs( BytesPerSample, Channels, tmp_adpcm.samples_left_in_block * fmt.adpcm_sample_frame_size);
}


int RageSoundReader_WAV::get_length_fmt_normal() const
{
	const int offset = this->rw.GetFileSize();
    return ConvertBytePosToMs( BytesPerSample, Channels, offset - this->fmt.data_starting_offset);
}

#define FIXED_POINT_COEF_BASE      256
#define FIXED_POINT_ADAPTION_BASE  256
#define SMALLEST_ADPCM_DELTA       16

bool RageSoundReader_WAV::read_adpcm_block_headers( adpcm_t &out ) const
{
    ADPCMBLOCKHEADER *headers = out.blockheaders;

    int i;
    for (i = 0; i < fmt.wChannels; i++)
        RETURN_IF_MACRO(!read_uint8(rw, &headers[i].bPredictor), false);

    for (i = 0; i < fmt.wChannels; i++)
        RETURN_IF_MACRO(!read_le16(rw, &headers[i].iDelta), false);

    for (i = 0; i < fmt.wChannels; i++)
        RETURN_IF_MACRO(!read_le16(rw, &headers[i].iSamp[0]), false);

    for (i = 0; i < fmt.wChannels; i++)
        RETURN_IF_MACRO(!read_le16(rw, &headers[i].iSamp[1]), false);

    out.samples_left_in_block = out.wSamplesPerBlock;
    out.nibble_state = 0;
    return true;
}


void RageSoundReader_WAV::do_adpcm_nibble(Uint8 nib, ADPCMBLOCKHEADER *header, Sint32 lPredSamp)
{
	static const Sint32 max_audioval = ((1<<(16-1))-1);
	static const Sint32 min_audioval = -(1<<(16-1));
	static const Sint32 AdaptionTable[] =
    {
		230, 230, 230, 230, 307, 409, 512, 614,
		768, 614, 512, 409, 307, 230, 230, 230
	};

    Sint32 lNewSamp = lPredSamp;

    if (nib & 0x08)
        lNewSamp += header->iDelta * (nib - 0x10);
	else
        lNewSamp += header->iDelta * nib;

	lNewSamp = clamp(lNewSamp, min_audioval, max_audioval);

    Sint32 delta = ((Sint32) header->iDelta * AdaptionTable[nib]) /
              FIXED_POINT_ADAPTION_BASE;

	delta = max( delta, SMALLEST_ADPCM_DELTA );

    header->iDelta = Sint16(delta);
	header->iSamp[1] = header->iSamp[0];
	header->iSamp[0] = Sint16(lNewSamp);
}


bool RageSoundReader_WAV::decode_adpcm_sample_frame()
{
	ADPCMBLOCKHEADER *headers = adpcm.blockheaders;

	Uint8 nib = adpcm.nibble;
	for (int i = 0; i < this->fmt.wChannels; i++)
	{
		const Sint16 iCoef1 = adpcm.Coef1[headers[i].bPredictor];
		const Sint16 iCoef2 = adpcm.Coef2[headers[i].bPredictor];
		const Sint32 lPredSamp = ((headers[i].iSamp[0] * iCoef1) +
			(headers[i].iSamp[1] * iCoef2)) / FIXED_POINT_COEF_BASE;

		if (adpcm.nibble_state == 0)
		{
			if( !read_uint8(this->rw, &nib) )
				return false;
			adpcm.nibble_state = 1;
			do_adpcm_nibble(nib >> 4, &headers[i], lPredSamp);
		}
		else
		{
			adpcm.nibble_state = 0;
			do_adpcm_nibble(nib & 0x0F, &headers[i], lPredSamp);
		}
	}

	adpcm.nibble = nib;
	return true;
}


void RageSoundReader_WAV::put_adpcm_sample_frame( Uint16 *buf, int frame )
{
    ADPCMBLOCKHEADER *headers = adpcm.blockheaders;
    for (int i = 0; i < fmt.wChannels; i++)
        *(buf++) = headers[i].iSamp[frame];
}


Uint32 RageSoundReader_WAV::read_sample_fmt_adpcm(char *buf, unsigned len)
{
	Uint32 bw = 0;

	while (bw < len)
	{
		/* Read a new block. */
		if( adpcm.samples_left_in_block == 0 )
			if (!read_adpcm_block_headers(adpcm))
				return bw;

		const bool first_sample_in_block = ( adpcm.samples_left_in_block == adpcm.wSamplesPerBlock );
		put_adpcm_sample_frame( (Uint16 *) (buf + bw), first_sample_in_block? 1:0 );
		adpcm.samples_left_in_block--;
		bw += this->fmt.adpcm_sample_frame_size;

		if( !first_sample_in_block && adpcm.samples_left_in_block )
		{
			if (!decode_adpcm_sample_frame())
			{
				adpcm.samples_left_in_block = 0;
				return bw;
			}
		}
    }

	return bw;
}



int RageSoundReader_WAV::seek_sample_fmt_adpcm( Uint32 ms )
{
	const int offset = ConvertMsToBytePos( BytesPerSample, Channels, ms );
	const int bpb = (adpcm.wSamplesPerBlock * this->fmt.adpcm_sample_frame_size);
	const int skipsize = (offset / bpb) * this->fmt.wBlockAlign;

	const int pos = skipsize + this->fmt.data_starting_offset;
	int rc = this->rw.Seek( pos );
	BAIL_IF_MACRO(rc == -1, this->rw.GetError(), -1);

	/* The offset we need is in this block, so we need to decode to there. */
	rc = offset % bpb;  /* bytes into this block we need to decode */
	adpcm.samples_left_in_block = 0;

	if( rc == 0 )
		return ms;

	if (!read_adpcm_block_headers(adpcm))
	{
		adpcm.samples_left_in_block = 0;
		return 0;
	}

	adpcm.samples_left_in_block--;
	rc -= this->fmt.adpcm_sample_frame_size;

	while (rc > 0)
	{
		adpcm.samples_left_in_block--;
		rc -= this->fmt.adpcm_sample_frame_size;

		if (!decode_adpcm_sample_frame())
		{
			adpcm.samples_left_in_block = 0;
			return 0;
		}
	}

	return ms;
}


/* Locate a chunk by ID. */
int RageSoundReader_WAV::find_chunk( Uint32 id, Sint32 &size )
{
	Uint32 pos = this->rw.Tell();
	while (1)
	{
		Uint32 id_ = 0;
		if( !read_le32(rw, &id_) )
			return false;
		if( !read_le32(rw, &size) )
			return false;

		if (id_ == id)
			return true;

		if(size < 0)
			return false;

		pos += (sizeof (Uint32) * 2) + size;
		int ret = this->rw.Seek( pos );
		if( ret == -1 )
		{
			SetError( this->rw.GetError() );
			return false;
		}
	}
}


SoundReader_FileReader::OpenResult RageSoundReader_WAV::WAV_open_internal()
{
	Uint32 magic1;
	if( !read_le32(rw, &magic1) || magic1 != riffID )
	{
		SetError( "WAV: Not a RIFF file." );
		return OPEN_NO_MATCH;
	}

	Uint32 ignore;
	read_le32(rw, &ignore); /* throw the length away; we get this info later. */

	Uint32 magic2;
	if( !read_le32( rw, &magic2 ) || magic2 != waveID )
	{
		SetError( "Not a WAVE file." );
		return OPEN_NO_MATCH;
	}

	Sint32 NextChunk;
    BAIL_IF_MACRO(!find_chunk(fmtID, NextChunk), "No format chunk.", OPEN_MATCH_BUT_FAIL);
	NextChunk += this->rw.Tell();
    BAIL_IF_MACRO(!read_fmt_chunk(), "Can't read format chunk.", OPEN_MATCH_BUT_FAIL);

	/* I think multi-channel WAVs are possible, but I've never even seen one. */
    Channels = (Uint8) fmt.wChannels;
	ASSERT( Channels <= 2 );

	if( fmt.wFormatTag != FMT_NORMAL &&
		fmt.wFormatTag != FMT_ADPCM )
	{
		CString format;
		switch( fmt.wFormatTag )
		{
		case FMT_ITU_G711_ALAW:  format = "ITU G.711 A-law"; break;
		case FMT_ITU_G711_MULAW: format = "ITU G.711 mu-law"; break;
		case FMT_IMA_ADPCM:      format = "IMA ADPCM"; break;
		case FMT_ITU_G723_ADPCM: format = "ITU G.723 ADPCM"; break;
		case FMT_GSM_610:        format = "GSM 6.10"; break;
		case FMT_ITU_G721_ADPCM: format = "ITU G.721 ADPCM"; break;
		case FMT_MPEG:           format = "MPEG"; break;
		case FMT_MPEG_L3:        format = "MPEG Layer 3"; break; // or "other"?
		default: format = ssprintf( "Unknown WAV format #%i", fmt.wFormatTag ); break;
		}

		SetError( ssprintf("%s not supported", format.c_str() ) );

		/* It might be MP3 data in a WAV.  (Why do people *do* that?)  It's possible
		 * that the MAD decoder will figure that out, so let's return OPEN_NO_MATCH
		 * and keep searching for a decoder. */
		if( fmt.wFormatTag == FMT_MPEG_L3 )
			return OPEN_NO_MATCH;

		return OPEN_MATCH_BUT_FAIL;
	}

    if ( fmt.wBitsPerSample == 4 && this->fmt.wFormatTag == FMT_ADPCM )
	{
		Conversion = CONV_NONE;
		BytesPerSample = 2;
	}
    else if (fmt.wBitsPerSample == 8)
	{
		Conversion = CONV_8BIT_TO_16BIT;
		BytesPerSample = 1;
	}
    else if (fmt.wBitsPerSample == 16)
	{
		Conversion = CONV_16LSB_TO_16SYS;
		BytesPerSample = 2;
	}
    else
    {
		SetError( ssprintf("Unsupported sample size %i", fmt.wBitsPerSample) );
		return OPEN_MATCH_BUT_FAIL;
    }

	if( Conversion == CONV_8BIT_TO_16BIT )
		Input_Buffer_Ratio *= 2;
	if( Channels == 1 )
		Input_Buffer_Ratio *= 2;

	this->rw.Seek( NextChunk );

	Sint32 DataSize;
    BAIL_IF_MACRO(!find_chunk(dataID, DataSize), "No data chunk.", OPEN_MATCH_BUT_FAIL);

    fmt.data_starting_offset = this->rw.Tell();
    fmt.adpcm_sample_frame_size = BytesPerSample * Channels;

    return OPEN_OK;
}


SoundReader_FileReader::OpenResult RageSoundReader_WAV::Open( CString filename_ )
{
	Close();
	Input_Buffer_Ratio = 1;
	filename = filename_;
	if( !this->rw.Open( filename ) )
	{
		SetError( ssprintf("Couldn't open file: %s", this->rw.GetError().c_str()) );
		return OPEN_NO_MATCH;
	}

    memset(&fmt, 0, sizeof(fmt));

    SoundReader_FileReader::OpenResult rc = WAV_open_internal();
    if ( rc != OPEN_OK )
		Close();

    return rc;
}


void RageSoundReader_WAV::Close()
{
	this->rw.Close();
}


int RageSoundReader_WAV::Read(char *buf, unsigned len)
{
	/* Input_Buffer_Ratio is always 2 or 4.  Make sure len is always a multiple of
	 * Input_Buffer_Ratio; handling extra bytes is a pain and useless. */
	ASSERT( (len % Input_Buffer_Ratio) == 0);

	int ActualLen = len / Input_Buffer_Ratio;
	int ret = 0;
	switch (this->fmt.wFormatTag)
	{
	case FMT_NORMAL:
		ret = read_sample_fmt_normal( buf, ActualLen );
		break;
	case FMT_ADPCM:
		ret = read_sample_fmt_adpcm( buf, ActualLen );
		break;
	default: ASSERT(0); break;
	}

	if( ret <= 0 )
		return ret;

	if( Conversion == CONV_16LSB_TO_16SYS )
	{
		/* Do this in place. */
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
		const int cnt = len / sizeof(Sint16);
		Sint16 *tbuf = (Sint16 *) buf;
		for( int i = 0; i < cnt; ++i )
			tbuf[i] = SDL_Swap16( tbuf[i] );
#endif
	}

	static Sint16 *tmpbuf = NULL;
	static unsigned tmpbufsize = 0;
	if( len > tmpbufsize )
	{
		tmpbufsize = len;
		delete [] tmpbuf;
		tmpbuf = new Sint16[len];
	}
	if( Conversion == CONV_8BIT_TO_16BIT )
	{
		for( int s = 0; s < ret; ++s )
			tmpbuf[s] = (Sint16(buf[s])-128) << 8;
		memcpy( buf, tmpbuf, ret * sizeof(Sint16) );
		ret *= 2; /* 8-bit to 16-bit */
	}

	if( Channels == 1 )
	{
		Sint16 *in = (Sint16*) buf;
		for( int s = 0; s < ret/2; ++s )
			tmpbuf[s*2] = tmpbuf[s*2+1] = in[s];
		memcpy( buf, tmpbuf, ret * sizeof(Sint16) );
		ret *= 2; /* 1 channel -> 2 channels */
	}

	return ret;
}


int RageSoundReader_WAV::SetPosition(int ms)
{
	switch (this->fmt.wFormatTag)
	{
	case FMT_NORMAL:
		return seek_sample_fmt_normal( ms );
	case FMT_ADPCM:
		return seek_sample_fmt_adpcm( ms );
	}
	ASSERT(0);
	return -1;
}

int RageSoundReader_WAV::GetLength() const
{
    const int origpos = this->rw.Tell();
	
	int ret = 0;
	switch (this->fmt.wFormatTag)
	{
	case FMT_NORMAL:
		ret = get_length_fmt_normal();
		break;
	case FMT_ADPCM:
		ret = get_length_fmt_adpcm();
		break;
	}

	int rc = this->rw.Seek( origpos );
    BAIL_IF_MACRO( rc == -1, this->rw.GetError(), -1 );

	return ret;
}

RageSoundReader_WAV::RageSoundReader_WAV()
{
}

SoundReader *RageSoundReader_WAV::Copy() const
{
	RageSoundReader_WAV *ret = new RageSoundReader_WAV;
	ret->Open( filename );
	return ret;
}

RageSoundReader_WAV::~RageSoundReader_WAV()
{
	Close();
}

/*
 * Copyright (C) 2001  Ryan C. Gordon (icculus@clutteredmind.org)
 * Copyright (C) 2003-2004 Glenn Maynard
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

