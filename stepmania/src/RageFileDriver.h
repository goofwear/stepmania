/*
 * RageFileDriver, RageFileObj: Low-level file access driver classes.
 */

#ifndef RAGE_FILE_DRIVER_H
#define RAGE_FILE_DRIVER_H

#include "RageFile.h"
#include "RageFileManager.h"

class RageFileObj;
class FilenameDB;
class RageFileDriver
{
public:
	RageFileDriver( FilenameDB *db ) { FDB = db; }
	virtual ~RageFileDriver();
	virtual RageBasicFile *Open( const CString &path, int mode, int &err ) = 0;
	virtual void GetDirListing( const CString &sPath, CStringArray &AddTo, bool bOnlyDirs, bool bReturnPathToo );
	virtual RageFileManager::FileType GetFileType( const CString &sPath );
	virtual int GetFileSizeInBytes( const CString &sFilePath );
	virtual int GetFileHash( const CString &sPath );
	virtual int GetPathValue( const CString &path );
	virtual bool Ready() { return true; } /* see RageFileManager::MountpointIsReady */
	virtual void FlushDirCache( const CString &sPath );
	virtual bool Remove( const CString &sPath ) { return false; }

	/* Possible error returns from Open, in addition to standard errno.h values: */
	enum { ERROR_WRITING_NOT_SUPPORTED = -1 };
protected:
	FilenameDB *FDB;
};

class RageFileObj: public RageBasicFile
{
public:
	RageFileObj();
	virtual ~RageFileObj() { }

	CString GetError() const { return m_sError; }
	void ClearError() { SetError(""); }
	
	bool AtEOF() const { return m_bEOF; }

	int Seek( int iOffset );
	int Seek( int offset, int whence );
	int Tell() const { return m_iFilePos; }

	int Read( void *pBuffer, size_t iBytes );
	int Read( CString &buffer, int bytes = -1 );
	int Read( void *buffer, size_t bytes, int nmemb );

	int Write( const void *pBuffer, size_t iBytes );
	int Write( const CString &sString ) { return Write( sString.data(), sString.size() ); }
	int Write( const void *buffer, size_t bytes, int nmemb );

	int Flush();

	int GetLine( CString &out );
	int PutLine( const CString &str );

	virtual int GetFileSize() const = 0;
	virtual CString GetDisplayPath() const { return ""; }
	virtual RageBasicFile *Copy() const { FAIL_M( "Copying unimplemented" ); }

protected:
	virtual int SeekInternal( int iOffset ) { FAIL_M( "Seeking unimplemented" ); }
	virtual int ReadInternal( void *pBuffer, size_t iBytes ) = 0;
	virtual int WriteInternal( const void *pBuffer, size_t iBytes ) = 0;
	virtual int FlushInternal() { return 0; }

	void SetError( const CString &sError ) { m_sError = sError; }
	CString m_sError;

private:
	int FillBuf();
	void ResetBuf();

	bool m_bEOF;
	int m_iFilePos;

	enum { BSIZE = 1024 };
	char m_Buffer[BSIZE];
	char *m_pBuf;
	int  m_iBufAvail;
};

/* This is used to register the driver, so RageFileManager can see it. */
struct FileDriverEntry
{
	FileDriverEntry( CString Type );
	virtual ~FileDriverEntry();
	virtual RageFileDriver *Create( CString Root ) const = 0;

	CString m_Type;
	const FileDriverEntry *m_Link;
};
RageFileDriver *MakeFileDriver( CString Type, CString Root );

#endif

/*
 * Copyright (c) 2003-2004 Glenn Maynard
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
