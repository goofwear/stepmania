/* This reader simply precaches all of the data from another reader. This
 * reduces CPU usage for sounds that are played several times at once. */

#include "global.h"
#include "RageSoundReader_Preload.h"
#include "RageUtil.h"

#define samplesize (sizeof(float) * m_iChannels) /* 16-bit */

/* If a sound is smaller than this, we'll load it entirely into memory. */
const unsigned max_prebuf_size = 1024*256;

bool RageSoundReader_Preload::PreloadSound( RageSoundReader *&pSound )
{
	RageSoundReader_Preload *pPreload = new RageSoundReader_Preload;
	if( !pPreload->Open(pSound) )
	{
		/* Preload failed.  It read some data, so we need to rewind the reader. */
		pSound->SetPosition( 0 );
		delete pPreload;
		return false;
	}

	pSound = pPreload;
	return true;
}

RageSoundReader_Preload::RageSoundReader_Preload():
	m_Buffer( new RString )
{
}

int RageSoundReader_Preload::GetTotalSamples() const
{
	return m_Buffer->size() / samplesize;
}

bool RageSoundReader_Preload::Open( RageSoundReader *pSource )
{
	ASSERT( pSource );
	m_iSampleRate = pSource->GetSampleRate();
	m_iChannels = pSource->GetNumChannels();
	m_fRate = pSource->GetStreamToSourceRatio();
	
	/* Check the length, and see if we think it'll fit in the buffer. */
	int iLen = pSource->GetLength_Fast();
	if( iLen != -1 )
	{
		float fSecs = iLen / 1000.f;

		unsigned iPCMSize = unsigned( fSecs * m_iSampleRate * samplesize ); /* seconds -> bytes */
		if( iPCMSize > max_prebuf_size )
			return false; /* Don't bother trying to preload it. */

		m_Buffer.Get()->reserve( iPCMSize );
	}

	while(1)
	{
		/* If the rate changes, we won't preload it. */
		if( pSource->GetStreamToSourceRatio() != m_fRate )
			return false; /* Don't bother trying to preload it. */

		float buffer[1024];
		int iCnt = pSource->Read( buffer, ARRAYSIZE(buffer) / m_iChannels );

		if( iCnt == END_OF_FILE )
			break;
		if( iCnt < 0 )
			return false;

		/* Add the buffer. */
		m_Buffer.Get()->append( (char *) buffer, (char *) (buffer+iCnt*m_iChannels) );

		if( m_Buffer.Get()->size() > max_prebuf_size )
			return false; /* too big */
	}

	m_iPosition = 0;
	delete pSource;
	return true;
}

int RageSoundReader_Preload::GetLength() const
{
	return int(float(GetTotalSamples()) * 1000.f / m_iSampleRate);
}

int RageSoundReader_Preload::GetLength_Fast() const
{
	return GetLength();
}

int RageSoundReader_Preload::SetPosition( int iFrame )
{
	m_iPosition = iFrame;
	m_iPosition = lrintf(m_iPosition / m_fRate);

	if( m_iPosition >= int(m_Buffer->size() / samplesize) )
	{
		m_iPosition = m_Buffer->size() / samplesize;
		return 0;
	}

	return 1;
}

int RageSoundReader_Preload::GetNextSourceFrame() const
{
	return lrintf(m_iPosition * m_fRate);
}

int RageSoundReader_Preload::Read( float *pBuffer, int iFrames )
{
	const int iSizeFrames = m_Buffer->size() / samplesize;
	const int iFramesAvail = iSizeFrames - m_iPosition;

	iFrames = min( iFrames, iFramesAvail );
	if( iFrames == 0 )
		return END_OF_FILE;
	memcpy( pBuffer, m_Buffer->data() + (m_iPosition * samplesize), iFrames * samplesize );
	m_iPosition += iFrames;
	
	return iFrames;
}

RageSoundReader_Preload *RageSoundReader_Preload::Copy() const
{
	return new RageSoundReader_Preload(*this);
}

int RageSoundReader_Preload::GetReferenceCount() const
{
	return m_Buffer.GetReferenceCount();
}

/*
 * Copyright (c) 2003 Glenn Maynard
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
