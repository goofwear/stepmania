/* for dladdr: */
#define __USE_GNU
#include "global.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/wait.h>

#include "StepMania.h" /* for g_argv */
#include "RageUtil.h"
#include "CrashHandler.h"
#include "CrashHandlerInternal.h"
#include "RageLog.h" /* for RageLog::GetAdditionalLog, etc. only */
#include "ProductInfo.h"

#if defined(DARWIN)
#include "archutils/Darwin/Crash.h"
#endif

#if defined(HAVE_VERSION_INFO)
extern const unsigned version_num;
#endif

struct BacktraceNames
{
    CString Symbol, File;
    int Address;
    int Offset;
    void FromAddr( void *p );
    void FromString( CString str );
    void Demangle();
    CString Format() const;
};

#if defined(HAVE_LIBIBERTY)
#include "libiberty.h"

/* This is in libiberty. Where is it declared? */
extern "C" {
    char *cplus_demangle (const char *mangled, int options);
}

void BacktraceNames::Demangle()
{
    char *f = cplus_demangle(Symbol, 0);
    if(!f)
        return;
    Symbol = f;
    free(f);
}
#else
void BacktraceNames::Demangle() { }
#endif


CString BacktraceNames::Format() const
{
    CString ShortenedPath = File;
    if( ShortenedPath != "" )
    {
        /* We have some sort of symbol name, so abbreviate or elide the module name. */
        if( ShortenedPath == g_argv[0] )
            ShortenedPath = "";
        else
        {
            unsigned slash = ShortenedPath.rfind('/');
            if( slash != ShortenedPath.npos )
                ShortenedPath = ShortenedPath.substr(slash+1);
            ShortenedPath = CString("(") + ShortenedPath + ")";
        }
    }

    CString ret = ssprintf( "%08x: ", Address );
    if( Symbol != "" )
        ret += Symbol + " ";
    ret += ShortenedPath;

    return ret;
}


#if defined(BACKTRACE_LOOKUP_METHOD_DLADDR)
/* This version simply asks libdl, which is more robust. */
#include <dlfcn.h>
void BacktraceNames::FromAddr( void *p )
{
    Dl_info di;
    if( !dladdr(p, &di) )
        return;

    Symbol = di.dli_sname;
    File = di.dli_fname;
    Address = (int) p;
    Offset = (char*)(p)-(char*)di.dli_saddr;
}

#elif defined(BACKTRACE_LOOKUP_METHOD_BACKTRACE_SYMBOLS)
/* This version parses backtrace_symbols(), an doesn't need libdl. */
#include <execinfo.h>
void BacktraceNames::FromAddr( void *p )
{
    char **foo = backtrace_symbols(&p, 1);
    if( foo == NULL )
        return;
    FromString( foo[0] );
    Address = (int) p;
    free(foo);
}

/* "path(mangled name+offset) [address]" */
void BacktraceNames::FromString( CString s )
{
    /* Hacky parser.  I don't want to use regexes in the crash handler. */
    CString MangledAndOffset, sAddress;
    unsigned pos = 0;
    while( pos < s.size() && s[pos] != '(' && s[pos] != '[' )
        File += s[pos++];
    TrimRight( File );
    TrimLeft( File );

    if( pos < s.size() && s[pos] == '(' )
    {
        pos++;
        while( pos < s.size() && s[pos] != ')' )
            MangledAndOffset += s[pos++];
    }

    if( MangledAndOffset != "" )
    {
        unsigned plus = MangledAndOffset.rfind('+');

        if(plus == MangledAndOffset.npos)
        {
            Symbol = MangledAndOffset;
            Offset = 0;
        }
        else
        {
            Symbol = MangledAndOffset.substr(0, plus);
            CString str = MangledAndOffset.substr(plus);
            if( sscanf(str, "%i", &Offset) != 1 )
                Offset=0;
        }
    }
}
#elif defined(BACKTRACE_LOOKUP_METHOD_ATOS)
#include <cxxabi.h>

void BacktraceNames::FromAddr( void *p )
{
    int fds[2];
    int out = fileno(stdout);
    pid_t pid;
    pid_t ppid = getppid(); /* Do this before fork()ing! */
    
    Offset = 0;
    Address = long(p);

    if (pipe(fds) != 0)
    {
        fprintf(stderr, "FromAddr pipe() failed: %s\n", strerror(errno));
        return;
    }

    pid = fork();
    if (pid == -1)
    {
        fprintf(stderr, "FromAddr fork() failed: %s\n", strerror(errno));
        return;
    }

    if (pid == 0)
    {
        close(fds[0]);
        for (int fd = 3; fd < 1024; ++fd)
            if (fd != fds[1])
                close(fd);
        dup2(fds[1], out);
        close(fds[1]);

        char *addy;
        asprintf(&addy, "0x%x", long(p));
        char *p;
        asprintf(&p, "%d", ppid);

        execl("/usr/bin/atos", "/usr/bin/atos", "-p", p, addy, NULL);
        
        fprintf(stderr, "execl(atos) failed: %s\n", strerror(errno));
        free(addy);
        free(p);
        close(out);
        _exit(1);
    }
    
    close(fds[1]);
    char f[1024];
    bzero(f, 1024);
    int len = read(fds[0], f, 1024);

    Symbol = "";
    File = "";

    if (len == -1)
    {
        fprintf(stderr, "FromAddr read() failed: %s\n", strerror(errno));
        return;
    }
    CStringArray mangledAndFile;

    split(f, " ", mangledAndFile, true);
    if (mangledAndFile.size() == 0)
        return;
    Symbol = mangledAndFile[0];
    /* eg
     * -[NSApplication run]
     * +[SomeClass initialize]
     */
    if (Symbol[0] == '-' || Symbol[0] == '+')
    {
        Symbol = mangledAndFile[0] + " " + mangledAndFile[1];
        /* eg
         * (crt.c:300)
         * (AppKit)
         */
        if (mangledAndFile.size() == 3)
        {
            File = mangledAndFile[2];
            unsigned pos = File.find('(');
            unsigned start = (pos == File.npos ? 0 : pos+1);
            pos = File.rfind(')') - 1;
            File = File.substr(start, pos);
        }
        return;
    }
    /* eg
     * __start   -> _start
     * _SDL_main -> SDL_main
     */
    if (Symbol[0] == '_')
        Symbol = Symbol.substr(1);
    
    /* demangle the name using __cxa_demangle() if needed */
    if (Symbol.substr(0, 2) == "_Z")
    {
        const char *name;
        int status = 0;
        name = abi::__cxa_demangle(Symbol, 0, 0, &status);
        if (name)
            Symbol = name;
        else
        {
            switch (status)
            {
                case 0:
                    fprintf(stderr, "WTF? It returned success...\n");
                    break;
                case -1:
                    fprintf(stderr, "Memory allocation failure.\n");
                    break;
                case -2:
                    fprintf(stderr, "Invalid mangled name: %s.\n", Symbol.c_str());
                    break;
                case -3:
                    fprintf(stderr, "Invalid arguments.\n");
                    break;
                default:
                    fprintf(stderr, "No idea what happened here.");
            }
        }
    }
    /* eg, the full line:
     * __Z1Ci (in a.out) (asmtest.cc:33)
     * _main (in a.out) (asmtest.cc:52)
     */
    if (mangledAndFile.size() > 3)
    {
        File = mangledAndFile[3];
        unsigned pos = File.find('(');
        unsigned start = (pos == File.npos ? 0 : pos+1);
        pos = File.rfind(')') - 1;
        File = File.substr(start, pos);
    }
    /* eg, the full line:
     * _main (SDLMain.m:308)
     * __Z8GameLoopv (crt.c:300)
     */
    else if (mangledAndFile.size() == 3)
        File = mangledAndFile[2].substr(0, mangledAndFile[2].rfind(')'));
}
#else
#warning Undefined BACKTRACE_LOOKUP_METHOD_*
void BacktraceNames::FromAddr( void *p )
{
    Address = long(p);
    Offset = 0;
    Symbol = "";
    File = "";
}
#endif


static void output_stack_trace( FILE *out, void **BacktracePointers )
{
	if( BacktracePointers[0] == BACKTRACE_METHOD_NOT_AVAILABLE )
	{
		fprintf( out, "No backtrace method available.\n");
		return;
	}

	if( !BacktracePointers[0] )
	{
		fprintf( out, "Backtrace was empty.\n");
		return;
	}

    for( int i = 0; BacktracePointers[i]; ++i)
    {
        BacktraceNames bn;
        bn.FromAddr( BacktracePointers[i] );
        bn.Demangle();

        if( bn.Symbol == "__libc_start_main" )
            break;

        fprintf( out, "%s\n", bn.Format().c_str() );
    }
}

/* Once we get here, we should be * safe to do whatever we want;
* heavyweights like malloc and CString are OK. (Don't crash!) */
static void child_process()
{
    int ret;

    /* 1. Read the backtrace pointers. */
    void *BacktracePointers[BACKTRACE_MAX_SIZE];
    ret = read(3, BacktracePointers, sizeof(void *)*BACKTRACE_MAX_SIZE);

    /* 2. Read the signal. */
    int SignalReceived;
    ret = read(3, &SignalReceived, sizeof(SignalReceived));

    /* 3. Read info. */
    int size;
    ret = read(3, &size, sizeof(size));
    char *Info = new char [size];
    ret = read(3, Info, size);

    /* 4. Read AdditionalLog. */
    ret = read(3, &size, sizeof(size));
    char *AdditionalLog = new char [size];
    ret = read(3, AdditionalLog, size);

    /* 5. Read RecentLogs. */
    int cnt = 0;
    ret = read(3, &cnt, sizeof(cnt));
    char *Recent[1024];
    for( int i = 0; i < cnt; ++i )
    {
        ret = read(3, &size, sizeof(size));
        Recent[i] = new char [size];
        ret = read(3, Recent[i], size);
    }

    /* 6. Read CHECKPOINTs. */
    ret = read(3, &size, sizeof(size));
    char *temp = new char [size];
    ret = read(3, temp, size);
    CStringArray Checkpoints;
    split(temp, "$$", Checkpoints);

    /* Wait for the child to either finish cleaning up or die.  XXX:
        * This should time out, in case something deadlocks. */

    char x;
    ret = read(3, &x, sizeof(x));
    if( (ret == -1 && errno != EPIPE) || ret != 0 )
    {
        /* We expect an EOF or EPIPE.  What happened? */
        fprintf(stderr, "Unexpected child read() result: %i (%s)\n", ret, strerror(errno));
        /* keep going */
    }

    FILE *CrashDump = fopen("/tmp/crashinfo.txt", "w+");
    if(CrashDump == NULL)
    {
        fprintf(stderr, "Couldn't open crashinfo.txt: %s", strerror(errno));
        exit(1);
    }

    fprintf(CrashDump, "%s crash report", PRODUCT_NAME_VER );
#if defined(HAVE_VERSION_INFO)
    fprintf(CrashDump, " (build %u)", version_num);
#endif
    fprintf(CrashDump, "\n");
    fprintf(CrashDump, "--------------------------------------\n");
    fprintf(CrashDump, "\n");

    CString Signal;
#if !defined(DARWIN)
#define X(a) case a: Signal = #a; break;
    switch(SignalReceived)
    {
        case SIGALRM: Signal = "Alarm"; break;
        case SIGBUS: Signal = "Bus error"; break;
        case SIGFPE: Signal = "Floating point exception"; break;
        X(SIGHUP)
        case SIGILL: Signal = "Illegal instruction"; break;
        X(SIGINT)
        case SIGPIPE: Signal = "Broken pipe"; break;
        case SIGABRT: Signal = "Aborted"; break;
        X(SIGQUIT)
        case SIGSEGV: Signal = "Segmentation fault"; break;
        X(SIGTRAP) X(SIGTERM) X(SIGVTALRM) X(SIGXCPU) X(SIGXFSZ)
#if defined(HAVE_DECL_SIGPWR) && HAVE_DECL_SIGPWR
        X(SIGPWR)
#endif
        default: Signal = ssprintf("Unknown signal %i", SignalReceived); break;
    }
#else
#define X(code) case k##code: Signal = #code; break;
    switch (SignalReceived)
    {
        X(UnknownException)
        X(IllegalInstructionException)
        X(TrapException)
        X(AccessException)
        X(UnmappedMemoryException)
        X(ExcludedMemoryException)
        X(ReadOnlyMemoryException)
        X(UnresolvablePageFaultException)
        X(PrivilegeViolationException)
        X(TraceException)
        X(InstructionBreakpointException)
        X(DataBreakpointException)
        X(FloatingPointException)
        X(StackOverflowException)
        default: Signal = ssprintf("Unknown Exception %i", SignalReceived); break;
    }
#endif
    
    fprintf( CrashDump, "Crash reason: %s\n\n", Signal.c_str() );

    fprintf(CrashDump, "Checkpoints:\n");
    for (unsigned i=0; i<Checkpoints.size(); ++i)
        fprintf(CrashDump, Checkpoints[i]);
    fprintf(CrashDump, "\n");

    output_stack_trace( CrashDump, BacktracePointers );
    fprintf(CrashDump, "\n");

    fprintf(CrashDump, "Static log:\n");
    fprintf(CrashDump, "%s", Info);
    fprintf(CrashDump, "%s", AdditionalLog);
    fprintf(CrashDump, "\nPartial log:\n");
    for( int i = 0; i < cnt; ++i )
        fprintf(CrashDump, "%s\n", Recent[i] );
    fprintf(CrashDump, "\n");
    fprintf(CrashDump, "-- End of report\n");
    fclose(CrashDump);

#if defined(DARWIN)
    InformUserOfCrash();
#else
    fprintf(stderr,
            "\n"
            PRODUCT_NAME " has crashed.  Debug information has been output to\n"
            "\n"
            "    /tmp/crashinfo.txt\n"
            "\n"
            "Please report a bug at:\n"
            "\n"
            "    http://sourceforge.net/tracker/?func=add&group_id=37892&atid=421366\n"
            "\n"
            );
#endif

    /* Forcibly kill our parent. */
    //	kill( getppid(), SIGKILL );
}


void CrashHandlerHandleArgs()
{
    if(g_argc == 2 && !strcmp(g_argv[1], CHILD_MAGIC_PARAMETER))
    {
        child_process();
        exit(0);
    }
}


