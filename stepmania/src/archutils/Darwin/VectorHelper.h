#ifndef VECTOR_HELPER_H
#define VECTOR_HELPER_H

#if ( defined(__VEC__) || (defined(__SSE__) && defined(__SSE2__)) ) && defined(__GNUC__)
namespace Vector
{
	bool CheckForVector();

	// Dest and src need only natural alignment for their types.
	void FastSoundWrite( int32_t *dest, const int16_t *src, unsigned size, short volume );
	
	/* For both FastSoundRead(), dest and src need to be 16 byte aligned. The audio HAL
	 * will provide a 16-byte aligned buffer and RageSoundMixBuffer's buffer is also
	 * 16-byte aligned since it lives on the heap. */
	void FastSoundRead( int16_t *dest, const int32_t *src, unsigned size );
	void FastSoundRead( float *dest, const int32_t *src, unsigned size );
}
#define USE_VEC
#endif

#endif

/*
 * (c) 2006 Steve Checkoway
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
