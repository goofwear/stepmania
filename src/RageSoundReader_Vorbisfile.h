#ifndef RAGE_SOUND_READER_VORBISFILE
#define RAGE_SOUND_READER_VORBISFILE

#include "RageSoundReader_FileReader.h"

typedef struct OggVorbis_File OggVorbis_File;

class RageSoundReader_Vorbisfile: public SoundReader_FileReader {
	OggVorbis_File *vf;
	char buffer[4096*4];
	unsigned avail;
	int SetPosition(int ms, bool accurate);
	CString filename;

public:
	OpenResult Open(CString filename);
	int GetLength() const;
	int GetLength_Fast() const;
	int SetPosition_Accurate(int ms)  { return SetPosition(ms, true); }
	int SetPosition_Fast(int ms) { return SetPosition(ms, false); }
	int Read(char *buf, unsigned len);
	int GetSampleRate() const;
	~RageSoundReader_Vorbisfile();
	SoundReader *Copy() const;
};

#endif
/*
 * Copyright (c) 2002-2003 by the person(s) listed below.  All rights reserved.
 *	Glenn Maynard
 */
