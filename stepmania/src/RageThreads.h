#ifndef RAGE_THREADS_H
#define RAGE_THREADS_H

struct SDL_Thread;

class RageThread
{
	SDL_Thread *thr;
	CString name;

public:
	RageThread();
	~RageThread();

	void SetName( const CString &n ) { name = n; }
	void Create( int (*fn)(void *), void *data );

	/* For crash handlers: kill or suspend all threads (except for
	 * the running one) immediately. */ 
	static void HaltAllThreads( bool Kill=false );

	/* If HaltAllThreads was called (with Kill==false), resume. */
	static void ResumeAllThreads();

	static unsigned int GetCurrentThreadID();

	static const char *GetCurThreadName();
	static const char *GetThreadNameByID( unsigned int iID );
	int Wait();
	bool IsCreated() const { return thr != NULL; }
};

namespace Checkpoints
{
	void LogCheckpoints( bool yes=true );
	void SetCheckpoint( const char *file, int line, const char *message );
	const char *GetLogs( const char *delim );
};

#define CHECKPOINT (Checkpoints::SetCheckpoint(__FILE__, __LINE__, NULL))
#define CHECKPOINT_M(m) (Checkpoints::SetCheckpoint(__FILE__, __LINE__, m))

/* Mutex class that follows the behavior of Windows mutexes: if the same
 * thread locks the same mutex twice, we just increase a refcount; a mutex
 * is considered unlocked when the refcount reaches zero.  This is more
 * convenient, though much slower on some archs.  (We don't have any tightly-
 * coupled threads, so that's OK.) */
struct RageMutexImpl;
class RageMutex
{
	friend struct RageMutexImpl;

	RageMutexImpl *mut;
	CString m_sName;

	int m_UniqueID;
	
	void MarkLockedMutex();

public:
	CString GetName() const { return m_sName; }
	void SetName( const CString &s ) { m_sName = s; }
	void Lock();
	void Unlock();
	bool IsLockedByThisThread() const;

	RageMutex( CString name );
	~RageMutex();
};

/* Lock a mutex on construction, unlock it on destruction.  Helps for functions
 * with more than one return path. */
class LockMutex
{
	RageMutex &mutex;

	const char *file;
	int line;
	float locked_at;
	bool locked;

public:
	LockMutex(RageMutex &mut, const char *file, int line);
	LockMutex(RageMutex &mut): mutex(mut), file(NULL), line(-1), locked_at(-1), locked(true) { mutex.Lock(); }
	~LockMutex();
	LockMutex(LockMutex &cpy): mutex(cpy.mutex), file(NULL), line(-1), locked_at(cpy.locked_at), locked(true) { mutex.Lock(); }

	/* Unlock the mutex (before this would normally go out of scope).  This can
	 * only be called once. */
	void Unlock();
};

/* Double-abstracting __LINE__ lets us append it to other text, to generate
 * locally unique variable names.  (Otherwise we get "LocalLock__LINE__".) I'm
 * not sure why this works, but it does, in both VC and GCC. */

#if 0
#ifdef DEBUG
/* Use the debug version, which logs if something holds a lock for a long time.
 * __FUNCTION__ is nonstandard, but both GCC and VC support it; VC doesn't
 * support the standard, __func__. */
#define LockMutL2(m, l) LockMutex LocalLock ## l (m, __FUNCTION__, __LINE__)
#else
#define LockMutL2(m, l) LockMutex LocalLock ## l (m)
#endif

#define LockMutL(m, l) LockMutL2(m, l)
#define LockMut(m) LockMutL(m, __LINE__)
#endif

/* Gar.  It works in VC7, but not VC6, so for now this can only be used once
 * per function.  If you need more than that, declare LockMutexes yourself. 
 * Also, VC6 doesn't support __FUNCTION__. */
#if _MSC_VER < 1300 /* VC6, not VC7 */
#define LockMut(m) LockMutex LocalLock(m, __FILE__, __LINE__)
#else
#define LockMut(m) LockMutex LocalLock(m, __FUNCTION__, __LINE__)
#endif

#endif
