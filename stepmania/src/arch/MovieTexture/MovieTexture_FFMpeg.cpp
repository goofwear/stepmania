#include "global.h"
#include "MovieTexture_FFMpeg.h"

#include "RageDisplay.h"
#include "RageLog.h"
#include "RageUtil.h"
#include "RageFile.h"
#include "RageSurface.h"
#include "RageFileDriverReadAhead.h"

#include <cerrno>

namespace avcodec
{
#include <ffmpeg/avformat.h>
};

#if defined(_MSC_VER) && !defined(XBOX)
	#pragma comment(lib, "ffmpeg/lib/avcodec.lib")
	#pragma comment(lib, "ffmpeg/lib/avformat.lib")
#endif

#if defined(XBOX)
	/* NOTES: ffmpeg static libraries arent included in cvs - you have to build them yourself 
	 * or remove this file to produce xbox build
	 * 
	 * build ffmpeg with mingw32 ( howto http://arrozcru.no-ip.org/ffmpeg_wiki/tiki-index.php )
     * ./configure --enable-memalign-hack --enable-static --disable-mmx --target-os=mingw32 --arch=x86
     * you can use various switches to enable/disable codecs/muxers/etc. 
	 * 
	 * libgcc.a and libmingwex.a comes from mingw installation
	 * msys\mingw\lib\gcc\mingw32\3.4.5\libgcc.a */
	#pragma comment(lib, "ffmpeg/lib/libavcodec.a")
	#pragma comment(lib, "ffmpeg/lib/libavformat.a")
	#pragma comment(lib, "ffmpeg/lib/libavutil.a")
	#pragma comment(lib, "ffmpeg/lib/libgcc.a")
#endif

static struct AVPixelFormat_t
{
	int bpp;
	uint32_t masks[4];
	avcodec::PixelFormat pf;
	bool bHighColor;
	bool bByteSwapOnLittleEndian;
	MovieDecoderPixelFormatYCbCr YUV;
} AVPixelFormats[] = {
	{
		32,
		{ 0xFF000000,
		  0x00FF0000,
		  0x0000FF00,
		  0x000000FF },
		avcodec::PIX_FMT_YUYV422,
		false, /* N/A */
		true,
		PixelFormatYCbCr_YUYV422,
	},
	{ 
		32,
		{ 0x0000FF00,
		  0x00FF0000,
		  0xFF000000,
		  0x000000FF },
		avcodec::PIX_FMT_BGRA,
		true,
		true,
		PixelFormatYCbCr_Invalid,
	},
	{ 
		32,
		{ 0x00FF0000,
		  0x0000FF00,
		  0x000000FF,
		  0xFF000000 },
		avcodec::PIX_FMT_ARGB,
		true,
		true,
		PixelFormatYCbCr_Invalid,
	},
	/*
	{ 
		32,
		{ 0x000000FF,
		  0x0000FF00,
		  0x00FF0000,
		  0xFF000000 },
		avcodec::PIX_FMT_ABGR,
		true,
		true,
		PixelFormatYCbCr_Invalid,
	},
	{ 
		32,
		{ 0xFF000000,
		  0x00FF0000,
		  0x0000FF00,
		  0x000000FF },
		avcodec::PIX_FMT_RGBA,
		true,
		true,
		PixelFormatYCbCr_Invalid,
	}, */
	{ 
		24,
		{ 0xFF0000,
		  0x00FF00,
		  0x0000FF,
		  0x000000 },
		avcodec::PIX_FMT_RGB24,
		true,
		true,
		PixelFormatYCbCr_Invalid,
	},
	{ 
		24,
		{ 0x0000FF,
		  0x00FF00,
		  0xFF0000,
		  0x000000 },
		avcodec::PIX_FMT_BGR24,
		true,
		true,
		PixelFormatYCbCr_Invalid,
	},
	{
		16,
		{ 0x7C00,
		  0x03E0,
		  0x001F,
		  0x0000 },
		avcodec::PIX_FMT_RGB555,
		false,
		false,
		PixelFormatYCbCr_Invalid,
	},
	{ 0, { 0,0,0,0 }, avcodec::PIX_FMT_NB, true, false, PixelFormatYCbCr_Invalid }
};

static void FixLilEndian()
{
#if defined(ENDIAN_LITTLE)
	static bool Initialized = false;
	if( Initialized )
		return; 
	Initialized = true;

	for( int i = 0; i < AVPixelFormats[i].bpp; ++i )
	{
		AVPixelFormat_t &pf = AVPixelFormats[i];

		if( !pf.bByteSwapOnLittleEndian )
			continue;

		for( int mask = 0; mask < 4; ++mask)
		{
			int m = pf.masks[mask];
			switch( pf.bpp )
			{
				case 24: m = Swap24(m); break;
				case 32: m = Swap32(m); break;
				default: ASSERT(0);
			}
			pf.masks[mask] = m;
		}
	}
#endif
}

static int FindCompatibleAVFormat( bool bHighColor )
{
	for( int i = 0; AVPixelFormats[i].bpp; ++i )
	{
		AVPixelFormat_t &fmt = AVPixelFormats[i];
		if( fmt.YUV != PixelFormatYCbCr_Invalid )
		{
			EffectMode em = MovieTexture_Generic::GetEffectMode( fmt.YUV );
			if( !DISPLAY->IsEffectModeSupported(em) )
				continue;
		}
		else if( fmt.bHighColor != bHighColor )
		{
			continue;
		}

		PixelFormat pixfmt = DISPLAY->FindPixelFormat( fmt.bpp,
				fmt.masks[0],
				fmt.masks[1],
				fmt.masks[2],
				fmt.masks[3],
				true /* realtime */
				);

		if( pixfmt == PixelFormat_Invalid )
			continue;

		return i;
	}

	return -1;
}

class MovieTexture_FFMpeg: public MovieTexture_Generic
{
public:
	MovieTexture_FFMpeg( RageTextureID ID );

	static void RegisterProtocols();
	static RageSurface *AVCodecCreateCompatibleSurface( int iTextureWidth, int iTextureHeight, bool bPreferHighColor, int &iAVTexfmt, MovieDecoderPixelFormatYCbCr &fmtout );
};

RageSurface *RageMovieTextureDriver_FFMpeg::AVCodecCreateCompatibleSurface( int iTextureWidth, int iTextureHeight, bool bPreferHighColor, int &iAVTexfmt, MovieDecoderPixelFormatYCbCr &fmtout )
{
	FixLilEndian();

	int iAVTexfmtIndex = FindCompatibleAVFormat( bPreferHighColor );
	if( iAVTexfmtIndex == -1 )
		iAVTexfmtIndex = FindCompatibleAVFormat( !bPreferHighColor );

	if( iAVTexfmtIndex == -1 )
	{
		/* No dice.  Use the first avcodec format of the preferred bit depth,
		 * and let the display system convert. */
		for( iAVTexfmtIndex = 0; AVPixelFormats[iAVTexfmtIndex].bpp; ++iAVTexfmtIndex )
			if( AVPixelFormats[iAVTexfmtIndex].bHighColor == bPreferHighColor )
				break;
		ASSERT( AVPixelFormats[iAVTexfmtIndex].bpp );
	}
	
	const AVPixelFormat_t *pfd = &AVPixelFormats[iAVTexfmtIndex];
	iAVTexfmt = pfd->pf;
	fmtout = pfd->YUV;

	LOG->Trace( "Texture pixel format: %i %i (%ibpp, %08x %08x %08x %08x)", iAVTexfmt, fmtout,
		pfd->bpp, pfd->masks[0], pfd->masks[1], pfd->masks[2], pfd->masks[3] );

	if( pfd->YUV == PixelFormatYCbCr_YUYV422 )
		iTextureWidth /= 2;

	return CreateSurface( iTextureWidth, iTextureHeight, pfd->bpp,
		pfd->masks[0], pfd->masks[1], pfd->masks[2], pfd->masks[3] );
}

class MovieDecoder_FFMpeg: public MovieDecoder
{
public:
	MovieDecoder_FFMpeg();
	~MovieDecoder_FFMpeg();

	RString Open( RString sFile );
	void Close();
	void Rewind();

	void GetFrame( RageSurface *pOut );
	int DecodeFrame( float fTargetTime );

	int GetWidth() const { return m_pStream->codec->width; }
	int GetHeight() const { return m_pStream->codec->height; }

	RageSurface *CreateCompatibleSurface( int iTextureWidth, int iTextureHeight, bool bPreferHighColor, MovieDecoderPixelFormatYCbCr &fmtout );

	float GetTimestamp() const;
	float GetFrameDuration() const;

private:
	void Init();
	RString OpenCodec();
	int ReadPacket();
	int DecodePacket( float fTargetTime );

	avcodec::AVStream *m_pStream;
	avcodec::AVFrame m_Frame;
	avcodec::PixelFormat m_AVTexfmt; /* PixelFormat of output surface */

	float m_fPTS;
	avcodec::AVFormatContext *m_fctx;
	bool m_bGetNextTimestamp;
	float m_fTimestamp;
	float m_fTimestampOffset;
	float m_fLastFrameDelay;
	int m_iFrameNumber;
	bool m_bHadBframes;

	avcodec::AVPacket m_Packet;
	int m_iCurrentPacketOffset;

	/* 0 = no EOF
	 * 1 = EOF from ReadPacket
	 * 2 = EOF from ReadPacket and DecodePacket */
	int m_iEOF;
};

MovieDecoder_FFMpeg::MovieDecoder_FFMpeg()
{
	FixLilEndian();

	m_fctx = NULL;
	m_pStream = NULL;
	m_iCurrentPacketOffset = -1;

	/* Until we play the whole movie once without hitting a B-frame, assume
	 * they exist. */
	m_bHadBframes = true;

	Init();
}

MovieDecoder_FFMpeg::~MovieDecoder_FFMpeg()
{
	if( m_iCurrentPacketOffset != -1 )
	{
		avcodec::av_free_packet( &m_Packet );
		m_iCurrentPacketOffset = -1;
	}
}

void MovieDecoder_FFMpeg::Init()
{
	m_iEOF = 0;
	m_bGetNextTimestamp = true;
	m_fTimestamp = 0;
	m_fLastFrameDelay = 0;
	m_fPTS = -1;
	m_iFrameNumber = -1; /* decode one frame and you're on the 0th */
	m_fTimestampOffset = 0;

	if( m_iCurrentPacketOffset != -1 )
	{
		avcodec::av_free_packet( &m_Packet );
		m_iCurrentPacketOffset = -1;
	}
}

/* Read until we get a frame, EOF or error.  Return -1 on error, 0 on EOF, 1 if we have a frame. */
int MovieDecoder_FFMpeg::DecodeFrame( float fTargetTime )
{
	while( 1 )
	{
		int ret = DecodePacket( fTargetTime );

		if( ret == 1 )
			return 1;
		if( ret == -1 )
			return -1;
		if( ret == 0 && m_iEOF > 0 )
			return 0; /* eof */

		ASSERT( ret == 0 );
		ret = ReadPacket();
		if( ret < 0 )
			return ret; /* error */
	}
}

float MovieDecoder_FFMpeg::GetTimestamp() const
{
	return m_fTimestamp - m_fTimestampOffset;
}

float MovieDecoder_FFMpeg::GetFrameDuration() const
{
	return m_fLastFrameDelay;
}


/* Read a packet.  Return -1 on error, 0 on EOF, 1 on OK. */
int MovieDecoder_FFMpeg::ReadPacket()
{
	if( m_iEOF > 0 )
		return 0;

	while( 1 )
	{
		CHECKPOINT;
		if( m_iCurrentPacketOffset != -1 )
		{
			m_iCurrentPacketOffset = -1;
			avcodec::av_free_packet( &m_Packet );
		}

		int ret = avcodec::av_read_frame( m_fctx, &m_Packet );
		/* XXX: why is avformat returning AVERROR_NOMEM on EOF? */
		if( ret < 0 )
		{
			/* EOF. */
			m_iEOF = 1;
			m_Packet.size = 0;
			
			return 0;
		}

		if( m_Packet.stream_index == m_pStream->index )
		{
			m_iCurrentPacketOffset = 0;
			return 1;
		}

		/* It's not for the video stream; ignore it. */
		avcodec::av_free_packet( &m_Packet );
	}
}

/* Decode data from the current packet.  Return -1 on error, 0 if the packet is finished,
 * and 1 if we have a frame (we may have more data in the packet). */
int MovieDecoder_FFMpeg::DecodePacket( float fTargetTime )
{
	if( m_iEOF == 0 && m_iCurrentPacketOffset == -1 )
		return 0; /* no packet */

	while( m_iEOF == 1 || (m_iEOF == 0 && m_iCurrentPacketOffset < m_Packet.size) )
	{
		if( m_bGetNextTimestamp )
		{
			if (m_Packet.dts != int64_t(AV_NOPTS_VALUE))
			{
				m_fPTS = float( m_Packet.dts * av_q2d(m_pStream->time_base) );

				/* dts is the timestamp of the first frame in this packet. Only use it once;
				 * if we get more than one frame from the same packet (eg. f;lushing the last
				 * frame), extrapolate. */
				m_Packet.dts = int64_t(AV_NOPTS_VALUE);
			}
			else
				m_fPTS = -1;
			m_bGetNextTimestamp = false;
		}

		/* If we have no data on the first frame, just return EOF; passing an empty packet
		 * to avcodec_decode_video in this case is crashing it.  However, passing an empty
		 * packet is normal with B-frames, to flush.  This may be unnecessary in newer
		 * versions of avcodec, but I'm waiting until a new stable release to upgrade. */
		if( m_Packet.size == 0 && m_iFrameNumber == -1 )
			return 0; /* eof */

		bool bSkipThisFrame = 
			fTargetTime != -1 &&
			GetTimestamp() + GetFrameDuration() <= fTargetTime &&
			(m_pStream->codec->frame_number % 2) == 0;

		int iGotFrame;
		CHECKPOINT;
		/* Hack: we need to send size = 0 to flush frames at the end, but we have
		 * to give it a buffer to read from since it tries to read anyway. */
		static uint8_t dummy[FF_INPUT_BUFFER_PADDING_SIZE] = { 0 };
		int len = avcodec::avcodec_decode_video(
				m_pStream->codec, 
				&m_Frame, &iGotFrame,
				m_Packet.size? m_Packet.data:dummy, m_Packet.size );
		CHECKPOINT;

		if( len < 0 )
		{
			LOG->Warn("avcodec_decode_video: %i", len);
			return -1; // XXX
		}

		m_iCurrentPacketOffset += len;

		if( !iGotFrame )
		{
			if( m_iEOF == 1 )
				m_iEOF = 2;
			continue;
		}

		m_bGetNextTimestamp = true;

		if( m_fPTS != -1 )
		{
			m_fTimestamp = m_fPTS;
		}
		else
		{
			/* If the timestamp is zero, this frame is to be played at the
			 * time of the last frame plus the length of the last frame. */
			m_fTimestamp += m_fLastFrameDelay;
		}

		/* Length of this frame: */
		m_fLastFrameDelay = (float)m_pStream->codec->time_base.num / m_pStream->codec->time_base.den;
		m_fLastFrameDelay += m_Frame.repeat_pict * (m_fLastFrameDelay * 0.5f);

		++m_iFrameNumber;

		if( m_Frame.pict_type == FF_B_TYPE )
			m_bHadBframes = true;

		if( m_iFrameNumber == 0 )
		{
			/* Some videos start with a timestamp other than 0.  I think this is used
			 * when audio starts before the video.  We don't want to honor that, since
			 * the DShow renderer doesn't and we don't want to break sync compatibility. */
			const float expect = 0;
			const float actual = m_fTimestamp;
			if( actual - expect > 0 )
			{
				LOG->Trace("Expect %f, got %f -> %f", expect, actual, actual - expect );
				m_fTimestampOffset = actual - expect;
			}
		}

		if( bSkipThisFrame )
			continue;

		return 1;
	}

	return 0; /* packet done */
}

void MovieDecoder_FFMpeg::GetFrame( RageSurface *pSurface )
{
	avcodec::AVPicture pict;
	pict.data[0] = (unsigned char *) pSurface->pixels;
	pict.linesize[0] = pSurface->pitch;

	avcodec::img_convert( &pict, m_AVTexfmt,
			(avcodec::AVPicture *) &m_Frame, m_pStream->codec->pix_fmt, 
			m_pStream->codec->width, m_pStream->codec->height );
}

static avcodec::AVStream *FindVideoStream( avcodec::AVFormatContext *m_fctx )
{
	ASSERT_M( m_fctx->nb_streams <= MAX_STREAMS, ssprintf( "m_fctx->nb_streams = %d", m_fctx->nb_streams) );
	for( unsigned stream = 0; stream < m_fctx->nb_streams; ++stream )
	{
		avcodec::AVStream *enc = m_fctx->streams[stream];
		if( enc->codec->codec_type == avcodec::CODEC_TYPE_VIDEO )
			return enc;
	}
	return NULL;
}


static RString averr_ssprintf( int err, const char *fmt, ... )
{
	ASSERT( err < 0 );

	va_list     va;
	va_start(va, fmt);
	RString s = vssprintf( fmt, va );
	va_end(va); 

	RString Error;
	switch( err )
	{
	case AVERROR_IO:		Error = "I/O error"; break;
	case AVERROR_NUMEXPECTED:	Error = "number syntax expected in filename"; break;
	case AVERROR_INVALIDDATA:	Error = "invalid data found"; break;
	case AVERROR_NOMEM:		Error = "not enough memory"; break;
	case AVERROR_NOFMT:		Error = "unknown format"; break;
	default: Error = ssprintf( "unknown error %i", err ); break;
	}

	return s + " (" + Error + ")";
}

int URLRageFile_open( avcodec::URLContext *h, const char *filename, int flags )
{
	if( strncmp( filename, "rage://", 7 ) )
	{
		LOG->Warn("URLRageFile_open: Unexpected path \"%s\"", filename );
	    return -EIO;
	}
	filename += 7;

	int mode = 0;
	switch( flags )
	{
	case URL_RDONLY: mode = RageFile::READ; break;
	case URL_WRONLY: mode = RageFile::WRITE | RageFile::STREAMED; break;
	case URL_RDWR: FAIL_M( "O_RDWR unsupported" );
	}

	RageFileBasic *pFile = new RageFile;

	{
		RageFile *f = new RageFile;
		if( !f->Open(filename, mode) )
		{
			LOG->Trace("Error opening \"%s\": %s", filename, f->GetError().c_str() );
			delete f;
		    return -EIO;
		}
		pFile = f;
	}

	/* If possible, wrap this file in the read-ahead filter to avoid skips when we rewind. */
	if( RageFileDriverReadAhead::FileSupported(pFile) )
	{
		RageFileDriverReadAhead *pBufferedFile = new RageFileDriverReadAhead( pFile, 1024*128 );
		pBufferedFile->DeleteFileWhenFinished();
		pFile = pBufferedFile;
	}

	h->is_streamed = false;
	h->priv_data = pFile;
	return 0;
}

int URLRageFile_read( avcodec::URLContext *h, unsigned char *buf, int size )
{
	RageFileBasic *f = (RageFileBasic *) h->priv_data;
	return f->Read( buf, size );
}

int URLRageFile_write( avcodec::URLContext *h, unsigned char *buf, int size )
{
	RageFileBasic *f = (RageFileBasic *) h->priv_data;
	return f->Write( buf, size );
}

avcodec::offset_t URLRageFile_seek( avcodec::URLContext *h, avcodec::offset_t pos, int whence )
{
	RageFileBasic *f = (RageFileBasic *) h->priv_data;
	if( whence == AVSEEK_SIZE )
		return f->GetFileSize();

	if( whence != SEEK_SET && whence != SEEK_CUR && whence != SEEK_END )
		return -1;

	return f->Seek( (int) pos, whence );
}

int URLRageFile_close( avcodec::URLContext *h )
{
	RageFileBasic *f = (RageFileBasic *) h->priv_data;
	delete f;
	return 0;
}

static avcodec::URLProtocol RageProtocol =
{
	"rage",
	URLRageFile_open,
	URLRageFile_read,
	URLRageFile_write,
	URLRageFile_seek,
	URLRageFile_close,
	NULL
};

void MovieTexture_FFMpeg::RegisterProtocols()
{
	static bool Done = false;
	if( Done )
		return;
	Done = true;

	avcodec::av_register_all();
	avcodec::register_protocol( &RageProtocol );
}

RString MovieDecoder_FFMpeg::Open( RString sFile )
{
	MovieTexture_FFMpeg::RegisterProtocols();

	int ret = avcodec::av_open_input_file( &m_fctx, "rage://" + sFile, NULL, 0, NULL );
	if( ret < 0 )
		return RString( averr_ssprintf(ret, "AVCodec: Couldn't open \"%s\"", sFile.c_str()) );

	ret = avcodec::av_find_stream_info( m_fctx );
	if( ret < 0 )
		return RString( averr_ssprintf(ret, "AVCodec (%s): Couldn't find codec parameters", sFile.c_str()) );

	avcodec::AVStream *pStream = FindVideoStream( m_fctx );
	if( pStream == NULL )
		return "Couldn't find any video streams";
	m_pStream = pStream;

	if( m_pStream->codec->codec_id == avcodec::CODEC_ID_NONE )
		return ssprintf( "Unsupported codec %08x", m_pStream->codec->codec_tag );

	RString sError = OpenCodec();
	if( !sError.empty() )
		return ssprintf( "AVCodec (%s): %s", sFile.c_str(), sError.c_str() );

	LOG->Trace( "Bitrate: %i", m_pStream->codec->bit_rate );
	LOG->Trace( "Codec pixel format: %s", avcodec::avcodec_get_pix_fmt_name(m_pStream->codec->pix_fmt) );

	return RString();
}

RString MovieDecoder_FFMpeg::OpenCodec()
{
	Init();

	ASSERT( m_pStream );
	if( m_pStream->codec->codec )
		avcodec::avcodec_close( m_pStream->codec );

	avcodec::AVCodec *pCodec = avcodec::avcodec_find_decoder( m_pStream->codec->codec_id );
	if( pCodec == NULL )
		return ssprintf( "Couldn't find decoder %i", m_pStream->codec->codec_id );

	LOG->Trace("Opening codec %s", pCodec->name );

	if( !m_bHadBframes )
	{
		LOG->Trace("Setting CODEC_FLAG_LOW_DELAY" );
		m_pStream->codec->flags |= CODEC_FLAG_LOW_DELAY;
	}

	int ret = avcodec::avcodec_open( m_pStream->codec, pCodec );
	if( ret < 0 )
		return RString( averr_ssprintf(ret, "Couldn't open codec \"%s\"", pCodec->name) );
	ASSERT( m_pStream->codec->codec );

	/* This is set to true when we find a B-frame, to use on the next loop. */
	m_bHadBframes = false;

	return RString();
}

void MovieDecoder_FFMpeg::Close()
{
	if( m_pStream && m_pStream->codec->codec )
	{
		avcodec::avcodec_close( m_pStream->codec );
		m_pStream = NULL;
	}

	if( m_fctx )
	{
		avcodec::av_close_input_file( m_fctx );
		m_fctx = NULL;
	}

	Init();
}

void MovieDecoder_FFMpeg::Rewind()
{
	avcodec::av_seek_frame( m_fctx, -1, 0, 0 );
	OpenCodec();
}

RageSurface *MovieDecoder_FFMpeg::CreateCompatibleSurface( int iTextureWidth, int iTextureHeight, bool bPreferHighColor, MovieDecoderPixelFormatYCbCr &fmtout )
{
	return RageMovieTextureDriver_FFMpeg::AVCodecCreateCompatibleSurface( iTextureWidth, iTextureHeight, bPreferHighColor, *ConvertValue<int>(&m_AVTexfmt), fmtout );
}

MovieTexture_FFMpeg::MovieTexture_FFMpeg( RageTextureID ID ):
	MovieTexture_Generic( ID, new MovieDecoder_FFMpeg )
{
}

RageMovieTexture *RageMovieTextureDriver_FFMpeg::Create( RageTextureID ID, RString &sError )
{
	MovieTexture_FFMpeg *pRet = new MovieTexture_FFMpeg( ID );
	sError = pRet->Init();
	if( !sError.empty() )
		SAFE_DELETE( pRet );
	return pRet;
}

REGISTER_MOVIE_TEXTURE_CLASS( FFMpeg );

/*
 * (c) 2003-2005 Glenn Maynard
 * All rights reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, and/or sell copies of the Software, and to permit persons to
 * whom the Software is furnished to do so, provided that the above
 * copyright notice(s) and this permission notice appear in all copies of
 * the Software and that both the above copyright notice(s) and this
 * permission notice appear in supporting documentation.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF
 * THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS
 * INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
