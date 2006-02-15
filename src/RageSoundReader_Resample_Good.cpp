/*
 * This implements audio resampling, using the method described at:
 *  http://www.dspguru.com/info/faqs/mrfaq.htm
 *
 * Each conversion ratio uses some memory, but the resulting table is
 * shared, so the memory overhead per stream is negligible.
 */
#include "global.h"
#include "RageSoundReader_Resample_Good.h"
#include "RageLog.h"
#include "RageUtil.h"
#include "RageMath.h"
#include "RageThreads.h"

#include <numeric>

/* Filter length.  This must be a power of 2. */
#define L 8

namespace
{
	float sincf( float f )
	{
		if( f == 0 )
			return 1;
		return sinf(f)/f;
	}

	/* Modified Bessel function I0.  From Abramowitz and Stegun "Handbook of Mathematical
	 * Functions", "Modified Bessel Functions I and K". */
	float BesselI0( float fX )
	{
		float fAbsX = fabsf( fX );
		if( fAbsX < 3.75f )
		{
			float y = fX / 3.75f;
			y *= y;
			float fRet = 1.0f+y*(+3.5156229f+y*(+3.0899424f+y*(+1.2067492f+y*(+0.2659732f+y*(+0.0360768f+y*+0.0045813f)))));
			return fRet;
		}
		else
		{
			float y = 3.75f/fAbsX;
			float fRet = (exp(fAbsX)/sqrt(fAbsX)) *
				  (+0.39894228f+y*(+0.01328592f+y*(+0.00225319f+y*(-0.00157565f+y*(0.00916281f+
				y*(-0.02057706f+y*(+0.02635537f+y*(-0.01647633f+y*+0.00392377f))))))));
			return fRet;
		}
	}

	/* 
	 * Kaiser window:
	 * 
	 * K(n) = I0( B*sqrt(1-(n/p)^2) )
	 *        -----------------------
	 *                 I0(B)
	 *
	 * where B is the beta parameter, p is len/2, and n is in [-len/2,+len/2].
	 */
	void ApplyKaiserWindow( float *pBuf, int iLen, float fBeta )
	{
		const float fDenom = BesselI0(fBeta);
		float p = (iLen-1)/2.0f;
		for( int n = 0; n < iLen; ++n )
		{
			float fN1 = fabsf((n-p)/p);
			float fNum = fBeta * sqrtf( max(1-fN1*fN1, 0) );
			fNum = BesselI0( fNum );
			float fVal = fNum/fDenom;
			pBuf[n] *= fVal;
		}
	}

	void MultiplyVector( float *pStart, float *pEnd, float f )
	{
		for( ; pStart != pEnd; ++pStart )
			*pStart *= f;
	}

	void GenerateSincLowPassFilter( float *pFIR, int iWinSize, float fCutoff )
	{
		float p = (iWinSize-1)/2.0f;
		for( int n = 0; n < iWinSize; ++n )
		{
			float fN1 = (n-p);
			float fVal = sincf(2*PI*fCutoff * fN1)*(2*fCutoff);
			// printf( "n %i, %f, %f -> %f\n", n, p, fN1, fVal );
			pFIR[n] = fVal;
		}
#if 0
		float *pFIRp = pFIR+iWinSize/2;
		for(int i=-iWinSize/2;i<=iWinSize/2;i++)
		{
			float ff = sinc(2*M_PI*fCutoff * (i + 0.0))*(2*fCutoff);

			printf( "%i: %f\n", i, ff );

			pFIRp[i]=ff;
		}
		for( int i=0; i < iWinSize; i++ )
			printf( "sinc: %i: %f\n", i, pFIR[i] );
#endif
	}

	void NormalizeVector( float *pBuf, int iSize )
	{
		float fTotal = accumulate( &pBuf[0], &pBuf[iSize], 0.0f );
		MultiplyVector( &pBuf[0], &pBuf[iSize], 1/fTotal );
	}

	int GCD( int i1, int i2 )
	{
		while(1)
		{
			unsigned iRem = i2 % i1;
			if( iRem == 0 )
				return i1;

			i2 = i1;
			i1 = iRem;
		}

		return i1;
	}
}

#if 0
void RunFIRFilter( float *pIn, float *pOut, int iInputValues, float *pFIR, int iWinSize )
{
	for( int i = 0; i < iInputValues; ++i )
	{
		float fSum = 0;
		const float *pInData = &pIn[i];
		for( int j = 0; j < iWinSize; ++j )
		{
			float in = pInData[j];
			fSum += in*pFIR[j];
			printf( "%i: in %f * %f, += %f\n", j, pInData[j], pFIR[j], in*pFIR[j] );
		}

		pOut[i] = fSum;
	}
}
#endif

template<typename T>
class AlignedBuffer
{
public:
	AlignedBuffer( int iSize )
	{
		m_iSize = iSize;
		m_pBuf = new T[m_iSize];
	}

	AlignedBuffer( const AlignedBuffer &cpy )
	{
		m_iSize = cpy.m_iSize;
		m_pBuf = new T[m_iSize];
		memcpy( m_pBuf, cpy.m_pBuf, sizeof(T)*m_iSize );
	}
	~AlignedBuffer()
	{
		delete [] m_pBuf;
	}
	operator T*() { return m_pBuf; }
	operator const T*() const { return m_pBuf; }

private:
	T& operator=( T &rhs );
	int m_iSize;
	T *m_pBuf;
};

struct PolyphaseFilter
{
	struct State
	{
		State( const PolyphaseFilter &Target ):
			m_fBuf( L * 2 )
		{
			m_iPolyIndex = Target.m_iUpFactor-1;
			m_iFilled = 0;
			m_iBufNext = 0;
		}

		int m_iPolyIndex;
		int m_iFilled;

		/* This buffer is duplicated.  If the circular buffer is size L, the actual buffer
		 * is size L*2, and data at buf[N] is also at buf[N+L].  That way, we can access
		 * up to buf[N*2-1] without having to wrap. */
		AlignedBuffer<float> m_fBuf;
		int m_iBufNext;
	};
	friend struct State;

	PolyphaseFilter( int iUpFactor ):
		m_pPolyphase( L*iUpFactor )
	{
		m_iUpFactor = iUpFactor;
	}

	void Generate( const float *pFIR );
	int RunPolyphaseFilter( State &State, const float *pIn, int iSamplesIn, int iDownFactor,
			float *pOut, int iSamplesOut ) const;
	int GetLatency() const { return L/2; }

	int NumInputsForOutputSamples( const State &State, int iOut, int iDownFactor ) const;

private:
	AlignedBuffer<float> m_pPolyphase;
	int m_iUpFactor;
};

/*
 * Convert an FIR filter to a polyphase filter.
 *
 * pFIR is the input FIR filter, which has iL*iUpFactor values.
 * iL is the number of real samples each output sample looks at.
 * iUpFactor is the actual upsampling factor; the amount of zero-stuffing between each real sample.
 * pOutput is the 2D output polyphase filter, with iL*iL values.
 *
 * With an upsampling factor (iUpFactor) of 3, and a sinc filter length of 12 (iL*iUpFactor),
 *
 * input     first output sample (before decimation)
 * sample          second output sample
 *                     third output sample
 * 
 * 0         0
 * 0         1     0
 * 1592      2     1   0
 * 0         3     2   1
 * 0         4     3   2
 * 1623      5     4   3
 * 0         6     5   4
 * 0         7     6   5
 * 1682      8     7   6
 * 0         9     8   7
 * 0         10    9   8
 * 1730      11    10  9
 * 0               11  10
 * 0                   11
 *
 * first row: 2, 5, 8, 11
 * second: 1, 4, 7, 10
 * third: 0, 3, 6, 9
 * Read a new sample after passing the last line.
 */
void PolyphaseFilter::Generate( const float *pFIR )
{
	float *pOutput=m_pPolyphase;
	int iInputSize = L*m_iUpFactor;

	for( int iRow = 0; iRow < m_iUpFactor; ++iRow )
	{
		int iInputOffset = (m_iUpFactor-iRow-1) % m_iUpFactor;
		for( int iCol = 0; iCol < L; ++iCol )
		{
			*pOutput = pFIR[iInputOffset];
			++pOutput;
			iInputOffset += m_iUpFactor;
			iInputOffset %= iInputSize;
		}
	}
}

/*
 * We only want one boundary check when running the filter; either on the
 * number of inputs used, or the number of outputs produced.  Otherwise, we'll
 * have to maintain two counters, and check two values per iteration.
 *
 * First, call NumInputsForOutputSamples(out), to find out how many inputs to supply to get
 * the desired number of outputs.  Then, pass the data, the input count
 * and the output count to RunPolyphaseFilter.
 *
 * - When downsampling, we use the number of inputs as the boundary.  For example,
 * if the ratio is 1:3 (downsample x3), and the user gives us 10 samples, then we
 * process until we've consumed all of the input.  (This will result in exactly
 * the number of samples the user asked for with NumInputsForOutputSamples.)
 *
 * - When upsampling, we use the number of outputs as the boundary.  For example,
 * if the ratio is 3:1 (upsample x3), and the user wants 8 samples to be output,
 * we'll have been given 3 samples as input.  Process until we've produced 8
 * samples.
 *
 * In both cases, we have overlap.  In the first, it's possible that we could
 * have consumed an additional input without producing an output.  In the second,
 * it's possible that we could have produced an additional output without
 * consuming an input.
 */
int PolyphaseFilter::RunPolyphaseFilter(
		State &State,
		const float *pIn, int iSamplesIn, int iDownFactor,
		float *pOut, int iSamplesOut ) const
{
	ASSERT( iSamplesIn >= 0 );

	float *pOutOrig = pOut;
	const float *pInEnd = pIn + iSamplesIn;
	const float *pOutEnd = pOut + iSamplesOut;
	
	int iFilled = State.m_iFilled;
	int iPolyIndex = State.m_iPolyIndex;
	while( pOut != pOutEnd )
	{
		if( iFilled < L )
		{
			if( pIn == pInEnd )
				break;

			State.m_fBuf[State.m_iBufNext] = *pIn;
			State.m_fBuf[State.m_iBufNext + L] = *pIn;
			++State.m_iBufNext;
			State.m_iBufNext &= L-1;

			++pIn;
			++iFilled;
			continue;
		}

		while( pOut != pOutEnd )
		{
			const float *pCurPoly = &m_pPolyphase[iPolyIndex*L];
			const float *pInData = &State.m_fBuf[State.m_iBufNext];

			float fTot = 0;
			for( int j = 0; j < L; ++j )
				fTot += pInData[j]*pCurPoly[j];
			*pOut = fTot;
			++pOut;

			iPolyIndex += iDownFactor;
			if( iPolyIndex >= m_iUpFactor )
				break;
		}
		iFilled -= iPolyIndex/m_iUpFactor;
		iPolyIndex %= m_iUpFactor;
	}

	State.m_iFilled = iFilled;
	State.m_iPolyIndex = iPolyIndex;

	return pOut - pOutOrig;
}

/*
 * Return the number of input samples needed to produce the given number of output
 * samples.  This is dependent on the number of bytes in the buffer and the current
 * position of the stream.
 */
int PolyphaseFilter::NumInputsForOutputSamples( const State &State, int iOut, int iDownFactor ) const
{
	int iIn = 0;
	int iFilled = State.m_iFilled;
	int iPolyIndex = State.m_iPolyIndex;

#if 0
	while( iOut > 0 )
	{
		if( iFilled < L )
		{
			int iToFill = L-iFilled;
			iIn += iToFill;
			iFilled += iToFill;
		}

		while( iFilled == L && iOut )
		{
			--iOut;
			iPolyIndex += iDownFactor;

			if( iPolyIndex >= m_iUpFactor )
				break;
		}
		iFilled -= iPolyIndex/m_iUpFactor;
		iPolyIndex %= m_iUpFactor;
	}
#endif

	if( iOut > 0 )
	{
		if( iFilled < L )
		{
			int iToFill = L-iFilled;
			iIn += iToFill;
		}

		// The -1 here is because we don't refill m_fBuf after writing the last output.
		iPolyIndex += iDownFactor*(iOut-1);
		iIn += iPolyIndex/m_iUpFactor;
	}

	return iIn;
}

/*
 * Interface to PolyphaseFilter, providing a simple resampling interface.  This handles
 * reuse of PolyphaseFilters.  This does not handle delay, flushing, or multiple channels.
 */
class RageSoundResampler_Polyphase
{
public:
	RageSoundResampler_Polyphase( int iSourceRate, int iDestRate )
	{
		int iUpFactor = iDestRate;
		m_iDownFactor = iSourceRate;

		{
			int iGCD = GCD( iUpFactor, m_iDownFactor );
			iUpFactor /= iGCD;
			m_iDownFactor /= iGCD;
		}

		float fCutoffFrequency;
		{
			/*
			 * If we're upsampling, we want the low-pass filter to cut off at the
			 * nyquist frequency of the original sample.
			 *
			 * If we're downsampling, we want the low-pass filter to cut off at the
			 * nyquist frequency of the new sample.
			 */
			fCutoffFrequency = 1.0f / (2*iUpFactor);
			fCutoffFrequency = min( fCutoffFrequency, 1.0f / (2*m_iDownFactor) );
			LOG->Trace( "cutoff frequency %f -> %f, %f", fCutoffFrequency, 1.0f / (2*iUpFactor), 1.0f / (2*m_iDownFactor) );
		}

		/* Cache filter data, and reuse it without copying.  All operations after creation
		 * are const, so this doesn't cause thread-safety problems. */
		typedef map<pair<int,float>, PolyphaseFilter *> FilterMap;
		static RageMutex PolyphaseFiltersLock("PolyphaseFiltersLock");
		static FilterMap g_mapPolyphaseFilters;
		
		PolyphaseFiltersLock.Lock();
		pair<int,float> params( make_pair(iUpFactor, fCutoffFrequency) );
		FilterMap::const_iterator it = g_mapPolyphaseFilters.find(params);
		if( it == g_mapPolyphaseFilters.end() )
		{
			int iWinSize = L*iUpFactor;
			float *pFIR = new float[iWinSize];
			GenerateSincLowPassFilter( pFIR, iWinSize, fCutoffFrequency );
			ApplyKaiserWindow( pFIR, iWinSize, 8 );
			NormalizeVector( pFIR, iWinSize );
			MultiplyVector( &pFIR[0], &pFIR[iWinSize], (float) iUpFactor );

			PolyphaseFilter *pPolyphase = new PolyphaseFilter( iUpFactor );
			pPolyphase->Generate( pFIR );
			delete [] pFIR;

			g_mapPolyphaseFilters[params] = pPolyphase;
			m_pPolyphase = pPolyphase;
		}
		else
		{
			/* We already have a filter for this upsampling factor and cutoff; use it. */
			m_pPolyphase = it->second;
		}
		PolyphaseFiltersLock.Unlock();

		m_pState = new PolyphaseFilter::State( *m_pPolyphase );
	}

	~RageSoundResampler_Polyphase()
	{
		delete m_pState;
	}

	int Run( const float *pIn, int iSamplesIn, float *pOut, int iSamplesOut ) const
	{
		return m_pPolyphase->RunPolyphaseFilter( *m_pState, pIn, iSamplesIn, m_iDownFactor, pOut, iSamplesOut );
	}

	void Reset()
	{
		delete m_pState;
		m_pState = new PolyphaseFilter::State( *m_pPolyphase );
	}

	int NumInputsForOutputSamples( int iOut ) const { return m_pPolyphase->NumInputsForOutputSamples(*m_pState, iOut, m_iDownFactor); }
	int GetLatency() const { return m_pPolyphase->GetLatency(); }

	RageSoundResampler_Polyphase( const RageSoundResampler_Polyphase &cpy )
	{
		m_pPolyphase = new PolyphaseFilter(*cpy.m_pPolyphase);
		m_pState = new PolyphaseFilter::State(*cpy.m_pState);
		m_iDownFactor = cpy.m_iDownFactor;
	}

private:
	const PolyphaseFilter *m_pPolyphase;
	PolyphaseFilter::State *m_pState;
	int m_iDownFactor;
};

RageSoundReader_Resample_Good::RageSoundReader_Resample_Good()
{
	m_pSource = NULL;
	m_iSampleRate = -1;
}

/* Call this if the input position is changed or reset. */
void RageSoundReader_Resample_Good::Reset()
{
	for( size_t iChannel = 0; iChannel < m_pSource->GetNumChannels(); ++iChannel )
		resamplers[iChannel]->Reset();
}


/* Call this if the sample factor changes. */
void RageSoundReader_Resample_Good::ReopenResampler()
{
	for( size_t iChannel = 0; iChannel < resamplers.size(); ++iChannel )
		delete resamplers[iChannel];
	resamplers.clear();
	for( size_t iChannel = 0; iChannel < m_pSource->GetNumChannels(); ++iChannel )
	{
		RageSoundResampler_Polyphase *p = new RageSoundResampler_Polyphase( m_pSource->GetSampleRate(), m_iSampleRate );
		resamplers.push_back( p );
	}
}

void RageSoundReader_Resample_Good::Open( SoundReader *pSource )
{
	ASSERT(pSource);
	m_pSource = pSource;
}


RageSoundReader_Resample_Good::~RageSoundReader_Resample_Good()
{
	for( size_t iChannel = 0; iChannel < resamplers.size(); ++iChannel )
		delete resamplers[iChannel];
	delete m_pSource;
}

void RageSoundReader_Resample_Good::SetSampleRate( int iHZ )
{
	m_iSampleRate = iHZ;
	ReopenResampler();
}

int RageSoundReader_Resample_Good::GetLength() const
{
	return m_pSource->GetLength();
}

int RageSoundReader_Resample_Good::GetLength_Fast() const
{
	return m_pSource->GetLength_Fast();
}

int RageSoundReader_Resample_Good::SetPosition_Accurate(int ms)
{
	Reset();
	return m_pSource->SetPosition_Accurate(ms);
}

int RageSoundReader_Resample_Good::SetPosition_Fast(int ms)
{
	Reset();
	return m_pSource->SetPosition_Fast(ms);
}

int RageSoundReader_Resample_Good::Read( char *bufp, unsigned len )
{
	int iChannels = resamplers.size();
	int iBytesPerFrame = sizeof(int16_t) * iChannels;

	int iFrames = len / iBytesPerFrame; /* bytes -> frames */
	int16_t *pBuf = (int16_t *) bufp;

	int iFramesRead = 0;

	{
		int iFramesNeeded = resamplers[0]->NumInputsForOutputSamples(iFrames);
		int iBytesNeeded = iFramesNeeded * sizeof(int16_t) * iChannels;
		int16_t *pTmpBuf = (int16_t *) alloca( iBytesNeeded );
		ASSERT( pTmpBuf );
		int iBytesIn = m_pSource->Read( (char *) pTmpBuf, iBytesNeeded );

		if( iBytesIn == -1 )
		{
			SetError( m_pSource->GetError() );
			return -1;
		}

		iBytesNeeded -= iBytesIn;
		iFramesNeeded -= iBytesIn / (sizeof(int16_t) * iChannels);

		const int iSamplesIn = iBytesIn / sizeof(int16_t);
		const int iFramesIn = iSamplesIn / iChannels;

		float *pFloatBuf = (float *) alloca( iSamplesIn * sizeof(float) );
		float *pFloatOut = (float *) alloca( iFrames * sizeof(float) );
		for( int iChannel = 0; iChannel < iChannels; ++iChannel )
		{
			{
				int16_t *pBufIn = pTmpBuf + iChannel;
				float *pBufOut = pFloatBuf;
				for( int i = 0; i < iSamplesIn; i += iChannels )
					*(pBufOut++) = (float) pBufIn[i];
			}

			int iGotFrames = resamplers[iChannel]->Run( pFloatBuf, iFramesIn, pFloatOut, iFrames );
			ASSERT( iGotFrames <= iFrames );

			int16_t *pBufOut = pBuf + iChannel;
			for( int i = 0; i < iGotFrames; ++i )
			{
				*pBufOut = int16_t(lrintf(clamp(pFloatOut[i], -32768, 32767)));
				pBufOut += iChannels;
			}
			if( iChannel == 0 )
				iFramesRead += iGotFrames;
		}
	}

	return iFramesRead * iBytesPerFrame;
}

SoundReader *RageSoundReader_Resample_Good::Copy() const
{
	SoundReader *pSource = m_pSource->Copy();
	RageSoundReader_Resample_Good *ret = new RageSoundReader_Resample_Good;

	for( size_t i = 0; i < resamplers.size(); ++i )
		ret->resamplers.push_back( new RageSoundResampler_Polyphase(*resamplers[i]) );
	ret->m_pSource = pSource;
	ret->m_iSampleRate = m_iSampleRate;

	return ret;
}

/*
 * (c) 2006 Glenn Maynard
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
