#ifndef RAGE_SOUND_READER_MP3_H
#define RAGE_SOUND_READER_MP3_H

#include "SDL_utils.h"
#include "RageSoundReader_FileReader.h"

struct madlib_t;

class RageSoundReader_MP3: public SoundReader_FileReader
{
public:
	int SampleRate;
    int Channels;

	CString filename;
    FILE *rw;
    madlib_t *mad;


	bool MADLIB_rewind();
	int SetPosition_toc( int ms, bool Xing );
	int SetPosition_hard( int ms );
	int SetPosition_estimate( int ms );

	int fill_buffer();
	int do_mad_frame_decode();
	int resync();
	void synth_output();
	int seek_stream_to_byte( int byte );
	int handle_first_frame();
	int GetLengthInternal( bool fast );

public:
	OpenResult Open(CString filename);
	void Close();
	int GetLength() const { return ((RageSoundReader_MP3*)this)->GetLengthInternal(false); }
	int GetLength_Fast() const { return ((RageSoundReader_MP3*)this)->GetLengthInternal(true); }
	int SetPosition_Accurate(int ms);
	int SetPosition_Fast(int ms);
	int Read(char *buf, unsigned len);
	int GetSampleRate() const { return SampleRate; }
	RageSoundReader_MP3();
	~RageSoundReader_MP3();
	RageSoundReader_MP3( const RageSoundReader_MP3 & ); /* not defined; don't use */
	SoundReader *Copy() const;
};

#endif
