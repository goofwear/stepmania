/*
 * This is a filesystem wrapper driver.  To use it, mount it on top of another filesystem at a
 * different mountpoint.  For example, mount the local path "d:/" as a normal directory to
 * /cdrom-native:
 *
 * FILEMAN->Mount( "dir", "d:/", "/cdrom-native" );
 *
 * and then mount a timeout filesystem on top of that:
 *
 * FILEMAN->Mount( "timeout", "/cdrom-native", "/cdrom-timeout" );
 *
 * and do all access to the device at /cdrom-timeout.
 *
 * A common problem with accessing devices, such as CDROMs or flaky pen drives, is that they
 * can take a very long time to fail under error conditions.  A single request may have a
 * timeout of 500ms, but a single operation may do many requests.  The system will usually
 * retry operations, and we want to allow it to do so, but we don't want the high-level
 * operation to take too long as a whole.
 *
 * There's no portable way to abort a running filesystem operation.  In Unix, you may be
 * able to use SIGALRM, which should cause a running operation to fail with EINTR, but
 * I don't trust that, and it's not portable.
 *
 * This driver abstracts all access to another driver, and enforces high-level timeouts.
 * Operations are run in a separate thread.  If an operation takes too long to complete,
 * the main thread will see it fail.  The operation in the thread will actually continue;
 * it will time out for real eventually.  To prevent accumulation of failed threads, if
 * an operation times out, we'll refuse all further access until all operations have
 * finished and exited.  (Load a separate driver for each device, so if one device fails,
 * others continue to function.)
 * 
 * All operations must run in the thread, including retrieving directory lists, Open()
 * and deleting file objects.  Read/write operations are copied through an intermediate
 * buffer, so we don't clobber stuff if the operation times out, the call returns and the
 * operation then completes.
 *
 * Unmounting the filesystem will wait for all timed-out operations to complete.
 *
 * This class is not threadsafe; do not access files on this filesystem from multiple
 * threads simultaneously.
 */

#include "global.h"
#include "RageFileDriverTimeout.h"
#include "RageUtil.h"
#include "RageUtil_FileDB.h"
#include "RageLog.h"

enum ThreadRequest
{
	REQ_OPEN,
	REQ_CLOSE,
	REQ_GET_FILE_SIZE,
	REQ_READ,
	REQ_WRITE,
	REQ_FLUSH,
	REQ_COPY,
	REQ_POPULATE_FILE_SET,
	REQ_FLUSH_DIR_CACHE,

	REQ_SHUTDOWN,
	REQ_INVALID,
	NUM_REQUESTS,
};

/* This is the class that does most of the work. */
class ThreadedFileWorker
{
public:
	ThreadedFileWorker( CString sPath );
	~ThreadedFileWorker();

	void SetTimeout( float fSeconds );

	/* Threaded operations.  If a file operation times out, the caller loses all access
	 * to the file and should fail all future operations; this is because the thread
	 * is still trying to finish the operation.  The thread will clean up afterwards. */
	RageFileBasic *Open( const CString &sPath, int iMode, int &iErr );
	void Close( RageFileBasic *pFile );
	int GetFileSize( RageFileBasic *&pFile );
	int Read( RageFileBasic *&pFile, void *pBuf, int iSize, CString &sError );
	int Write( RageFileBasic *&pFile, const void *pBuf, int iSize, CString &sError );
	int Flush( RageFileBasic *&pFile, CString &sError );
	RageFileBasic *Copy( RageFileBasic *&pFile, CString &sError );

	bool FlushDirCache( const CString &sPath );
	bool PopulateFileSet( FileSet &fs, const CString &sPath );

private:
	static int StartWorkerMain( void *pThis ) { ((ThreadedFileWorker *) (pThis))->WorkerMain(); return 0; }
	void WorkerMain();

	/* Run the given request.  Return true if the operation completed, false on timeout.
	 * All other return values (including error returns from the request) are returned
	 * below. */
	bool DoRequest( ThreadRequest r );

	RageThread m_WorkerThread;
	RageEvent m_WorkerEvent;

	RageFileDriver *m_pChildDriver;

	RageTimer m_Timeout;

	/* All requests: */
	ThreadRequest m_Request;
	bool m_bRequestFinished;
	bool m_bTimedOut;

	/* List of files to delete: */
	vector<RageFileBasic *> m_apDeletedFiles;

	/* REQ_OPEN, REQ_POPULATE_FILE_SET, REQ_FLUSH_DIR_CACHE: */
	CString m_sRequestPath; /* in */

	/* REQ_OPEN, REQ_COPY: */
	RageFileBasic *m_pResultFile; /* out */

	/* REQ_POPULATE_FILE_SET: */
	FileSet m_ResultFileSet; /* out */

	/* REQ_OPEN: */
	int m_iRequestMode; /* in */

	/* REQ_CLOSE, REQ_GET_FILE_SIZE, REQ_COPY: */
	RageFileBasic *m_pRequestFile; /* in */

	/* REQ_OPEN, REQ_GET_FILE_SIZE, REQ_READ */
	int m_iResultRequest; /* out */

	/* REQ_READ, REQ_WRITE */
	int m_iRequestSize; /* in */
	CString m_sResultError; /* out */

	/* REQ_READ */
	char *m_pResultBuffer; /* out */

	/* REQ_WRITE */
	char *m_pRequestBuffer; /* in */
};

static vector<ThreadedFileWorker *> g_apWorkers;
static RageMutex g_apWorkersMutex("WorkersMutex");

/* Set the timeout length, and reset the timer. */
void RageFileDriverTimeout::SetTimeout( float fSeconds )
{
	g_apWorkersMutex.Lock();
	for( unsigned i = 0; i < g_apWorkers.size(); ++i )
		g_apWorkers[i]->SetTimeout( fSeconds );
	g_apWorkersMutex.Unlock();
}


ThreadedFileWorker::ThreadedFileWorker( CString sPath ):
	m_WorkerEvent( "\"" + sPath + "\" worker event" )
{
	/* Grab a reference to the child driver.  We'll operate on it directly. */
	m_pChildDriver = FILEMAN->GetFileDriver( sPath );
	if( m_pChildDriver == NULL )
		LOG->Warn( "ThreadedFileWorker: Mountpoint \"%s\" not found", sPath.c_str() );

	m_Timeout.SetZero();
	m_Request = REQ_INVALID;
	m_bTimedOut = false;
	m_pResultFile = NULL;
	m_pRequestFile = NULL;
	m_pResultBuffer = NULL;

	m_WorkerThread.SetName( "ThreadedFileWorker fileset worker \"" + sPath + "\"" );
	m_WorkerThread.Create( StartWorkerMain, this );

	g_apWorkersMutex.Lock();
	g_apWorkers.push_back( this );
	g_apWorkersMutex.Unlock();
}

ThreadedFileWorker::~ThreadedFileWorker()
{
	/* If we're timed out, wait. */
	m_WorkerEvent.Lock();
	if( m_bTimedOut )
	{
		LOG->Trace( "Waiting for timed-out fs \"%s\" to complete ..." );
		while( m_bTimedOut )
			m_WorkerEvent.Wait();
	}
	m_WorkerEvent.Unlock();

	/* Disable the timeout.  This will ensure that we really wait for the worker
	 * thread to shut down. */
	SetTimeout( -1 );

	/* Shut down. */
	DoRequest( REQ_SHUTDOWN );
	m_WorkerThread.Wait();

	if( m_pChildDriver != NULL )
		FILEMAN->ReleaseFileDriver( m_pChildDriver );

	/* Unregister ourself. */
	g_apWorkersMutex.Lock();
	for( unsigned i = 0; i < g_apWorkers.size(); ++i )
	{
		if( g_apWorkers[i] == this )
		{
			g_apWorkers.erase( g_apWorkers.begin()+i );
			break;
		}
	}
	g_apWorkersMutex.Unlock();
}

void ThreadedFileWorker::SetTimeout( float fSeconds )
{
	m_WorkerEvent.Lock();
	if( fSeconds == -1 )
		m_Timeout.SetZero();
	else
	{
		m_Timeout.Touch();
		m_Timeout += fSeconds;
	}
	m_WorkerEvent.Unlock();
}

bool ThreadedFileWorker::DoRequest( ThreadRequest r )
{
	ASSERT( !m_bTimedOut );
	ASSERT( m_Request == REQ_INVALID );

	/* Set the request, and wake up the worker thread. */
	m_WorkerEvent.Lock();
	m_Request = r;
	m_WorkerEvent.Broadcast();

	/* Wait for it to complete or time out. */
	while( !m_bRequestFinished )
	{
		bool bTimedOut = !m_WorkerEvent.Wait( &m_Timeout );
		if( bTimedOut )
			break;
	}

	/* Don't depend on bTimedOut to tell if we timed out; only look at m_bRequestFinished,
	 * to avoid a race condition where the event times out and the operation completes
	 * simultaneously. */
	bool bRequestFinished = m_bRequestFinished;
	if( !m_bRequestFinished )
	{
		/* Set m_bTimedOut true.  It'll be set to false when the operation actually
		 * completes. */
		m_bTimedOut = true;
	}
	else
	{
		/* The operation completed; make sure that the worker thread doesn't erase the file. */
		m_pRequestFile = NULL;
		m_bRequestFinished = false;
	}
	m_WorkerEvent.Unlock();

	return bRequestFinished;
}

void ThreadedFileWorker::WorkerMain()
{
	while(1)
	{
		m_WorkerEvent.Lock();
		while( m_Request == REQ_INVALID && m_apDeletedFiles.empty() )
			m_WorkerEvent.Wait();
		const ThreadRequest r = m_Request;
		m_Request = REQ_INVALID;

		/* We're going to clean up any deleted files.  Make a copy of the list; don't
		 * hold the lock while we delete. */
		vector<RageFileBasic *> apDeletedFiles = m_apDeletedFiles;
		m_apDeletedFiles.clear();

		m_WorkerEvent.Unlock();

		for( unsigned i = 0; i < apDeletedFiles.size(); ++i )
			delete apDeletedFiles[i];
		apDeletedFiles.clear();

		if( r == REQ_INVALID )
			continue;
		
		/* We have a request. */
		switch( r )
		{
		case REQ_OPEN:
			ASSERT( m_pResultFile == NULL );
			ASSERT( !m_sRequestPath.empty() );
			m_iResultRequest = 0;
			m_pResultFile = m_pChildDriver->Open( m_sRequestPath, m_iRequestMode, m_iResultRequest );
			break;

		case REQ_CLOSE:
			ASSERT( m_pRequestFile != NULL );
			m_apDeletedFiles.push_back( m_pRequestFile );
			break;

		case REQ_GET_FILE_SIZE:
			ASSERT( m_pRequestFile != NULL );
			m_iResultRequest = m_pRequestFile->GetFileSize();
			break;

		case REQ_READ:
			ASSERT( m_pRequestFile != NULL );
			ASSERT( m_pResultBuffer != NULL );
			m_iResultRequest = m_pRequestFile->Read( m_pResultBuffer, m_iRequestSize );
            m_sResultError = m_pRequestFile->GetError();
			break;

		case REQ_WRITE:
			ASSERT( m_pRequestFile != NULL );
			ASSERT( m_pRequestBuffer != NULL );
			m_iResultRequest = m_pRequestFile->Write( m_pRequestBuffer, m_iRequestSize );
            m_sResultError = m_pRequestFile->GetError();
			break;

		case REQ_FLUSH:
			ASSERT( m_pRequestFile != NULL );
			m_iResultRequest = m_pRequestFile->Flush();
            m_sResultError = m_pRequestFile->GetError();
			break;

		case REQ_COPY:
			ASSERT( m_pRequestFile != NULL );
			m_pResultFile = m_pRequestFile->Copy();
			break;

		case REQ_POPULATE_FILE_SET:
			ASSERT( !m_bRequestFinished );
			ASSERT( !m_sRequestPath.empty() );

			m_ResultFileSet = FileSet();
			m_pChildDriver->FDB->GetFileSetCopy( m_sRequestPath, m_ResultFileSet );
			break;

		case REQ_FLUSH_DIR_CACHE:
			m_pChildDriver->FlushDirCache( m_sRequestPath );
			break;

		case REQ_SHUTDOWN:
			break;

		default:
			FAIL_M( ssprintf("%i", r) );
		}

		m_WorkerEvent.Lock();

		if( m_bTimedOut )
		{
			/* The event timed out.  Clean up any residue from the last action. */
			if( m_pRequestFile != NULL )
			{
				m_apDeletedFiles.push_back( m_pResultFile );
				m_pResultFile = NULL;
				break;
			}

			if( m_pResultFile != NULL )
			{
				m_apDeletedFiles.push_back( m_pResultFile );
				m_pResultFile = NULL;
				break;
			}

			delete m_pRequestBuffer;
			m_pRequestBuffer = NULL;
			delete m_pResultBuffer;
			m_pResultBuffer = NULL;

			/* Clear the time-out flag, indicating that we can work again. */
			m_bTimedOut = false;
		}
		else
			m_bRequestFinished = true;

		/* We're finished.  Wake up the requester (if he's still around). */
		m_WorkerEvent.Broadcast();
		m_WorkerEvent.Unlock();

		if( r == REQ_SHUTDOWN )
			break;
	}
}

RageFileBasic *ThreadedFileWorker::Open( const CString &sPath, int iMode, int &iErr )
{
	if( m_pChildDriver == NULL )
	{
		iErr = ENODEV;
		return NULL;
	}

	/* If we're currently in a timed-out state, fail. */
	if( m_bTimedOut )
	{
		iErr = EFAULT; /* Win32 has no ETIMEDOUT */
		return NULL;
	}

	m_sRequestPath = sPath;
	m_iRequestMode = iMode;

	if( !DoRequest(REQ_OPEN) )
	{
		LOG->Trace( "Open(%s) timed out", sPath.c_str() );
		return false;
	}

	iErr = m_iResultRequest;
	RageFileBasic *pRet = m_pResultFile;
	m_pResultFile = NULL;

	return pRet;
}

void ThreadedFileWorker::Close( RageFileBasic *pFile )
{
	ASSERT( m_pChildDriver != NULL ); /* how did you get a file to begin with? */

	if( !m_bTimedOut )
	{
		/* If we're not in a timed-out state, try to wait for the deletion to complete
		 * before continuing. */
		m_pRequestFile = pFile;
		DoRequest( REQ_CLOSE );
	}
	else
	{
		/* Delete the file when the timeout completes. */
		m_WorkerEvent.Lock();
		m_apDeletedFiles.push_back( pFile );
		m_WorkerEvent.Broadcast();
		m_WorkerEvent.Unlock();
	}
}

int ThreadedFileWorker::GetFileSize( RageFileBasic *&pFile )
{
	ASSERT( m_pChildDriver != NULL ); /* how did you get a file to begin with? */
	
	/* If we're currently in a timed-out state, fail. */
	if( m_bTimedOut )
	{
		this->Close( pFile );
		pFile = NULL;
	}

	if( pFile == NULL )
		return -1;

	m_pRequestFile = pFile;

	if( !DoRequest(REQ_GET_FILE_SIZE) )
	{
		/* If we time out, we can no longer access pFile. */
		pFile = NULL;
		return -1;
	}

	return m_iResultRequest;
}

int ThreadedFileWorker::Read( RageFileBasic *&pFile, void *pBuf, int iSize, CString &sError )
{
	ASSERT( m_pChildDriver != NULL ); /* how did you get a file to begin with? */

	/* If we're currently in a timed-out state, fail. */
	if( m_bTimedOut )
	{
		this->Close( pFile );
		pFile = NULL;
	}

	if( pFile == NULL )
	{
		sError = "Operation timed out";
		return -1;
	}

	m_pRequestFile = pFile;
	m_iRequestSize = iSize;
	m_pResultBuffer = new char[iSize];

	if( !DoRequest(REQ_READ) )
	{
		/* If we time out, we can no longer access pFile. */
		sError = "Operation timed out";
		pFile = NULL;
		return -1;
	}

	int iGot = m_iResultRequest;
	memcpy( pBuf, m_pResultBuffer, iGot );
	delete [] m_pResultBuffer;

	return iGot;
}

int ThreadedFileWorker::Write( RageFileBasic *&pFile, const void *pBuf, int iSize, CString &sError )
{
	ASSERT( m_pChildDriver != NULL ); /* how did you get a file to begin with? */

	/* If we're currently in a timed-out state, fail. */
	if( m_bTimedOut )
	{
		this->Close( pFile );
		pFile = NULL;
	}

	if( pFile == NULL )
	{
		sError = "Operation timed out";
		return -1;
	}

	m_pRequestFile = pFile;
	m_iRequestSize = iSize;
	m_pRequestBuffer = new char[iSize];
	memcpy( m_pRequestBuffer, pBuf, iSize );

	if( !DoRequest(REQ_WRITE) )
	{
		/* If we time out, we can no longer access pFile. */
		sError = "Operation timed out";
		pFile = NULL;
		return -1;
	}

	int iGot = m_iResultRequest;
	delete [] m_pResultBuffer;

	return iGot;
}

int ThreadedFileWorker::Flush( RageFileBasic *&pFile, CString &sError )
{
	ASSERT( m_pChildDriver != NULL ); /* how did you get a file to begin with? */

	/* If we're currently in a timed-out state, fail. */
	if( m_bTimedOut )
	{
		this->Close( pFile );
		pFile = NULL;
	}

	if( pFile == NULL )
	{
		sError = "Operation timed out";
		return -1;
	}

	m_pRequestFile = pFile;

	if( !DoRequest(REQ_FLUSH) )
	{
		/* If we time out, we can no longer access pFile. */
		sError = "Operation timed out";
		pFile = NULL;
		return -1;
	}

	return m_iResultRequest;
}

RageFileBasic *ThreadedFileWorker::Copy( RageFileBasic *&pFile, CString &sError )
{
	ASSERT( m_pChildDriver != NULL ); /* how did you get a file to begin with? */

	/* If we're currently in a timed-out state, fail. */
	if( m_bTimedOut )
	{
		this->Close( pFile );
		pFile = NULL;
	}

	if( pFile == NULL )
	{
		sError = "Operation timed out";
		return NULL;
	}

	m_pRequestFile = pFile;
	if( !DoRequest(REQ_COPY) )
	{
		/* If we time out, we can no longer access pFile. */
		sError = "Operation timed out";
		pFile = NULL;
		return NULL;
	}

	RageFileBasic *pRet = m_pResultFile;
	m_pResultFile = NULL;

	return pRet;
}


bool ThreadedFileWorker::PopulateFileSet( FileSet &fs, const CString &sPath )
{
	if( m_pChildDriver == NULL )
		return false;

	/* If we're currently in a timed-out state, fail. */
	if( m_bTimedOut )
		return false;

	/* Fill in a different FileSet; copy the results in on success. */
	m_sRequestPath = sPath;
	m_bRequestFinished = false;

	/* Kick off the worker thread, and wait for it to finish. */
	if( !DoRequest(REQ_POPULATE_FILE_SET) )
	{
		LOG->Trace( "PopulateFileSet(%s) timed out", sPath.c_str() );
		return false;
	}

	fs = m_ResultFileSet;
	return true;
}

bool ThreadedFileWorker::FlushDirCache( const CString &sPath )
{
	if( m_pChildDriver == NULL )
		return false;

	/* If we're currently in a timed-out state, fail. */
	if( m_bTimedOut )
		return false;

	m_sRequestPath = sPath;

	/* Kick off the worker thread, and wait for it to finish. */
	if( !DoRequest(REQ_FLUSH_DIR_CACHE) )
	{
		LOG->Trace( "FlushDirCache(%s) timed out", sPath.c_str() );
		return false;
	}

	return true;
}


class RageFileObjTimeout: public RageFileObj
{
public:
	/* pFile will be freed by passing it to pWorker. */
	RageFileObjTimeout( ThreadedFileWorker *pWorker, RageFileBasic *pFile, int iSize )
	{
		m_pWorker = pWorker;
		m_pFile = pFile;
		m_iFileSize = iSize;
	}

	~RageFileObjTimeout()
	{
		if( m_pFile != NULL )
			m_pWorker->Close( m_pFile );
	}

	int GetFileSize() const
	{
		return m_iFileSize;
	}

	RageFileBasic *Copy() const
	{
		CString sError;
		RageFileBasic *pRet = m_pWorker->Copy( m_pFile, sError );

		if( m_pFile == NULL )
		{
//			SetError( "Operation timed out" );
			return NULL;
		}

		if( pRet == NULL )
		{
//			SetError( sError );
			return NULL;
		}

		return pRet;
	}

protected:
	int ReadInternal( void *pBuffer, size_t iBytes )
	{
		CString sError;
		int iRet = m_pWorker->Read( m_pFile, pBuffer, iBytes, sError );

		if( m_pFile == NULL )
		{
			SetError( "Operation timed out" );
			return -1;
		}

		if( iRet == -1 )
			SetError( sError );

		return iRet;
	}

	int WriteInternal( const void *pBuffer, size_t iBytes )
	{
		CString sError;
		int iRet = m_pWorker->Write( m_pFile, pBuffer, iBytes, sError );

		if( m_pFile == NULL )
		{
			SetError( "Operation timed out" );
			return -1;
		}

		if( iRet == -1 )
			SetError( sError );

		return iRet;
	}

	int FlushInternal()
	{
		CString sError;
		int iRet = m_pWorker->Flush( m_pFile, sError );

		if( m_pFile == NULL )
		{
			SetError( "Operation timed out" );
			return -1;
		}

		if( iRet == -1 )
			SetError( sError );

		return iRet;
	}

	/* Mutable because the const operation Copy() timing out results in the original
	 * object no longer being available. */
	mutable RageFileBasic *m_pFile;

	ThreadedFileWorker *m_pWorker;

	/* GetFileSize isn't allowed to fail, so cache the file size on load. */
	int m_iFileSize;
};

/* This FilenameDB runs PopulateFileSet in the worker thread. */
class TimedFilenameDB: public FilenameDB
{
public:
	TimedFilenameDB()
	{
		ExpireSeconds = -1;
		m_pWorker = NULL;
	}

	void SetWorker( ThreadedFileWorker *pWorker )
	{
		ASSERT( pWorker != NULL );
		m_pWorker = pWorker;
	}

	void PopulateFileSet( FileSet &fs, const CString &sPath )
	{
		ASSERT( m_pWorker != NULL );
		m_pWorker->PopulateFileSet( fs, sPath );
	}

private:
	ThreadedFileWorker *m_pWorker;
};

RageFileDriverTimeout::RageFileDriverTimeout( CString sPath ):
        RageFileDriver( new TimedFilenameDB() )
{
	m_pWorker = new ThreadedFileWorker( sPath );

	((TimedFilenameDB *) FDB)->SetWorker( m_pWorker );
}

RageFileBasic *RageFileDriverTimeout::Open( const CString &sPath, int iMode, int &iErr )
{
	RageFileBasic *pChildFile = m_pWorker->Open( sPath, iMode, iErr );
	if( pChildFile == NULL )
		return NULL;

	/* RageBasicFile::GetFileSize isn't allowed to fail, but we are; grab the file
	 * size now and store it. */
	int iSize = 0;
	if( iMode & RageFile::READ )
	{
		iSize = m_pWorker->GetFileSize( pChildFile );
		if( iSize == -1 )
		{
			/* When m_pWorker->GetFileSize fails, it takes ownership of pChildFile. */
			ASSERT( pChildFile == NULL );
			iErr = EFAULT;
			return NULL;
		}
	}

	return new RageFileObjTimeout( m_pWorker, pChildFile, iSize );
}

void RageFileDriverTimeout::FlushDirCache( const CString &sPath )
{
	m_pWorker->FlushDirCache( sPath );
}

RageFileDriverTimeout::~RageFileDriverTimeout()
{
	delete m_pWorker;
}

static struct FileDriverEntry_Timeout: public FileDriverEntry
{
        FileDriverEntry_Timeout(): FileDriverEntry( "TIMEOUT" ) { }
        RageFileDriver *Create( CString Root ) const { return new RageFileDriverTimeout( Root ); }
} const g_RegisterDriver;

/*
 * Copyright (c) 2005 Glenn Maynard
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
