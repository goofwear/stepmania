#include "global.h"
#include "RageSoundMixBuffer.h"
#include "RageUtil.h"

#if defined(MACOSX)
#include "archutils/Darwin/VectorHelper.h"
#ifdef USE_VEC
static bool g_bVector;
#endif
#endif

RageSoundMixBuffer::RageSoundMixBuffer()
{
	m_iBufSize = m_iBufUsed = 0;
	m_pMixbuf = NULL;
	m_iOffset = 0;
	SetVolume( 1.0f );
#ifdef USE_VEC
	g_bVector = Vector::CheckForVector();
#endif
}

RageSoundMixBuffer::~RageSoundMixBuffer()
{
	free( m_pMixbuf );
}

void RageSoundMixBuffer::SetVolume( float f )
{
	m_iVolumeFactor = int(256*f);
}

/* write() will start mixing iOffset samples into the buffer.  Be careful; this is
 * measured in samples, not frames, so if the data is stereo, multiply by two. */
void RageSoundMixBuffer::SetWriteOffset( int iOffset )
{
	m_iOffset = iOffset;
}

void RageSoundMixBuffer::Extend( unsigned iSamples )
{
	const unsigned realsize = iSamples+m_iOffset;
	if( m_iBufSize < realsize )
	{
		m_pMixbuf = (int32_t *) realloc( m_pMixbuf, sizeof(int32_t) * realsize );
		m_iBufSize = realsize;
	}

	if( m_iBufUsed < realsize )
	{
		memset( m_pMixbuf + m_iBufUsed, 0, (realsize - m_iBufUsed) * sizeof(int32_t) );
		m_iBufUsed = realsize;
	}
}

void RageSoundMixBuffer::write( const int16_t *pBuf, unsigned iSize, int iSourceStride, int iDestStride )
{
	if( iSize == 0 )
		return;

	/* iSize = 3, iDestStride = 2 uses 4 frames.  Don't allocate the stride of the
	 * last sample. */
	Extend( iSize * iDestStride - (iDestStride-1) );

	/* Scale volume and add. */
	int32_t *pDestBuf = m_pMixbuf+m_iOffset;
#ifdef USE_VEC
	if( g_bVector && iSourceStride == 1 && iDestStride == 1 )
	{
		Vector::FastSoundWrite( pDestBuf, pBuf, iSize, m_iVolumeFactor );
		return;
	}
#endif
	while( iSize )
	{
		*pDestBuf += *pBuf * m_iVolumeFactor;
		pBuf += iSourceStride;
		pDestBuf += iDestStride;
		--iSize;
	}
}

void RageSoundMixBuffer::read( int16_t *pBuf )
{
#ifdef USE_VEC
	if( g_bVector && (intptr_t(pBuf) & 0xF) == 0 )
	{
		Vector::FastSoundRead( pBuf, m_pMixbuf, m_iBufUsed );
		m_iBufUsed = 0;
		return;
	}
#endif
	for( unsigned iPos = 0; iPos < m_iBufUsed; ++iPos )
	{
		int32_t iOut = (m_pMixbuf[iPos]) / 256;
		pBuf[iPos] = (int16_t) clamp( iOut, -32768, 32767 );
	}
	m_iBufUsed = 0;
}

void RageSoundMixBuffer::read( float *pBuf )
{
#ifdef USE_VEC
	if( g_bVector && (intptr_t(pBuf) & 0xF) == 0 )
	{
		Vector::FastSoundRead( pBuf, m_pMixbuf, m_iBufUsed );
		m_iBufUsed = 0;
		return;
	}
#endif
	const int iMinimum = -32768 * 256;
	const int iMaximum = 32767 * 256;

	for( unsigned pos = 0; pos < m_iBufUsed; ++pos )
		pBuf[pos] = SCALE( (float)m_pMixbuf[pos], iMinimum, iMaximum, -1.0f, 1.0f );

	m_iBufUsed = 0;
}

/*
 * Copyright (c) 2002-2004 Glenn Maynard
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
