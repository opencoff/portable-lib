/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * sigwait_reactor.cpp - Signal handling framework based on
 * sigwaitinfo.
 *
 * (c) 2005-2010 Sudhi Herle <sw at herle.net>
 *
 * Licensing Terms: GPLv2 
 *
 * If you need a commercial license for this work, please contact
 * the author.
 *
 * This software does not come with any express or implied
 * warranty; it is provided "as is". No claim  is made to its
 * suitability for any purpose.
 *
 *
 * Some design notes
 * =================
 * The "reactor" pattern allows us to separate the notion of signal
 * delivery and signal dispatch into two contexts.
 *
 * Signal delivery is done by the underlying kernel and it is
 * completely asynchronous. Thus, it comes with a long list of
 * functions that simply CANNOT be called from the signal handler
 * context. Secondly, signals and threads don't mix. They are like
 * oil and water. However, signals are a necessary evil that every
 * non-trivial POSIX program must somehow deal with. Thus, out of
 * this desperate need is born this elegant framework that turns an
 * async notification into a synchronous one.
 *
 * The term "signal dispatch" is used to mean the deferred calling
 * of signal handlers from a _synchronous_ context - such as a
 * separate signal handling thread.
 *
 * Implementation
 * --------------
 * The notion above is realized in this implementation via the
 * following C++ objects:
 *
 *  - signal_handler: C++ interface class that defines how an
 *    application's signal handler looks to the reactor.
 *
 *  - sig_reactor: The *singleton* reactor framework which manages
 *    the reliable delivery of signals. The reactor is derived from
 *    the common Thread class. The dispatch thread is implemented
 *    via the "thread_entry()" member function of this class.
 *
 * The reactor thread waits for signals synchronously via
 * sigwaitinfo(2). It then calls the appropriate registered signal
 * handlers.
 *
 * Requests to stop the reactor cause a signal to be queued via
 * sigqueue(2). The reactor thread treats self-generated signals
 * specially (see thread_entry() and from_self()).
 */

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>
#include <assert.h>
#include <setjmp.h>
#include <syslog.h>

#include <exception>
#include <stdexcept>

#include "posix/sigreactor.h"


// Is this macro POSIX or SvR4?
#ifndef NSIG
#define NSIG    (8 * sizeof (sigset_t))
#endif



/*
 * Magic cookie when we send a SIGTERM to ourselves as a result of 
 * calling sig_reactor::stop()
 */
#define MAGIC_COOKIE    int(('D' << 24) | ('I' << 16) | ('E' << 8) | '!')

using namespace std;
using namespace posix;
using namespace putils;


namespace posix {


// singleton instance
sig_reactor* sig_reactor::reactor_instance = 0;


// Static method to return global instance
// XXX Should never call from signal handler!
sig_reactor&
sig_reactor::instance()
{
    // When is ctor called?
    static sig_reactor reactor;


    return reactor;
}



sig_reactor::sig_reactor()
        : Thread(),
           m_mutex(),
           m_blocked(),
           m_ignored(),
           m_sig_handlers()
{
    m_sig_handlers.reserve(NSIG);

    for (int i = 0; i < NSIG; ++i)
        m_sig_handlers.push_back(0);

    // Update the list of blocked signals - these are propogated
    // from the parent that created us.
    sigset old;
    old.fill();
    m_blocked.fill();

    ::pthread_sigmask(SIG_BLOCK, old, 0);

    //::openlog("sigreactor", LOG_NDELAY|LOG_NOWAIT|LOG_PID, LOG_DAEMON);

    // Start the underlying reactor thread
    start();
}

sig_reactor::~sig_reactor()
{
    // XXX is this a phoenix singleton?
    // Stop the reactor?
#if 0
    char buf[128];
    int n = snprintf(buf, sizeof buf, "sig_reactor DTOR running ..\n");
    write(2, buf, n);
#endif
}


// Tell the OS to stop sending us this signal
void
sig_reactor::block(const int sig)
{
    switch (sig)
    {
        // These should NEVER be blocked. POSIX says:
        //    "If any of these are generated while they are blocked,
        //     then the result is undefined"
        case SIGSEGV:
        case SIGBUS:
        case SIGILL:
        case SIGFPE:
            return;

        default:
            break;
    }

    scoped_lock lock(m_mutex);

    m_blocked.add(sig);
}


// Tell the OS we are ready to receive 'sig' again
void
sig_reactor::unblock(const int sig)
{
    switch (sig)
    {
        // These should NEVER be blocked. POSIX says:
        //    "If any of these are generated while they are blocked,
        //     then the result is undefined"
        case SIGSEGV:
        case SIGBUS:
        case SIGILL:
        case SIGFPE:
            return;

        default:
            break;
    }

    scoped_lock lock(m_mutex);

    m_blocked.del(sig);
}


// ignore this signal. i.e., exclude from processing it even if you
// receive it.
void
sig_reactor::ignore(const int sig)
{
    scoped_lock lock(m_mutex);

    m_ignored.add(sig);
}


// Un-ignore a signal; i.e., if this signal were previously ignored,
// stop doing that.
void
sig_reactor::unignore(const int sig)
{
    scoped_lock lock(m_mutex);

    m_ignored.del(sig);
}


// Add a handler for signal 'sig'
void
sig_reactor::add(const int sig, signal_handler * fp)
{
    scoped_lock lock(m_mutex);

    // Overwrite the previous entry
    m_sig_handlers[sig] = fp;

    // Adding a signal handler doesn't unblock that signal. One
    // needs to explicitly unblock it (if needed).
    // The best we can do is to unignore it - that way the reactor
    // thread will look at it. If the signal is blocked at the OS
    // level, then the reactor won't even get it.
    unignore(sig);
}


// Remove a handler for signal 'sig'
void
sig_reactor::remove(const int sig)
{
    scoped_lock lock(m_mutex);

    // XXX Posix doesn't provide a means to remove a signal handler.
    //     No. SIG_DFL doesn't count. It only works for handlers
    //     that are registered via sigaction::sa_handler. But, we
    //     use sigaction::sa_sigaction.

    // So, as a hack, we just ignore the signal.
    ignore(sig);
}



// wakeup the reactor thread
void
sig_reactor::wakeup(int sig)
{
    const pid_t self = getpid();

    union sigval sv;

    sv.sival_int = MAGIC_COOKIE;

    //log("sigreactor: Waking up thread with %s\n", signame(sig).c_str());

    // We queue a signal to indicate that we want the reactor to
    // stop.
    sigqueue(self, sig, sv);
}


// Stop the reactor!
void
sig_reactor::stop()
{
    wakeup(SIGTERM);

    //log("sigreactor: Waiting for thread to end..\n");

    //::closelog();

    //
    // Delete the singleton. It will be re-created later if needed.
    // Note: "this" and "reactor_instance" are one and the same.
    sig_reactor* ptr = reactor_instance;
    reactor_instance = 0;

    //log("sigreactor: Deleting instance!\n");

    delete ptr;

    // Don't do _anything_ else after this!
}


void
sig_reactor::log(const char * fmt, ...)
{
    char buf[1024];
    va_list ap;

    va_start(ap, fmt);
    int n = vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);

#if 1
    genesis::EventLog* logger = genesis::EventLog::Instance();

    logger->Warning(1, buf);
#else
    write(2, buf, n);
#endif
}


// Wait for reactor thread to be successfully initialized and
// running
void
sig_reactor::wait_for_create()
{
}


// Initialize the reactor thread
void
sig_reactor::thread_init()
{
    struct sigaction sa;
    memset(&sa, 0, sizeof sa);


    // We MUST install a _thread specific_ signal handler to catch
    // these hardware-synchronous signals. This allows us to recover
    // when registered signal handlers (via the "add()" interface)
    // cause SEGV etc.
    //
    // In order to break out of that inf. loop, this signal handler
    // will use siglongjmp() to get control back into the reactor
    // thread.
    sa.sa_sigaction = (void (*)(int, siginfo_t *, void *))reactor_sig_handler;
    sa.sa_flags     = SA_SIGINFO;
    sigaddset(&sa.sa_mask, SIGSEGV);
    sigaddset(&sa.sa_mask, SIGILL);
    sigaddset(&sa.sa_mask, SIGBUS);
    sigaddset(&sa.sa_mask, SIGFPE);

    sigaction(SIGSEGV, &sa, 0);
    sigaction(SIGBUS, &sa, 0);
    sigaction(SIGILL, &sa, 0);
    sigaction(SIGFPE, &sa, 0);

    //log("sigreactor: thread running ..\n");
}



// call a registered signal handler - but with some sanity and
// safety checks.
void
sig_reactor::call_handler(int sig, const siginfo_t& si)
{
    sigset * volatile blocked = &m_blocked;
    sigset * volatile ignored = &m_ignored;
    signal_handler * fp = 0;
    bool quit = false;

    // we want the signal handler to execute without the Mutex
    // lock held. So, we create this inner scope.
    m_mutex.lock();

        fp   = m_sig_handlers[sig];
        quit = blocked->has(sig) || ignored->has(sig);

    m_mutex.unlock();

    if (quit)
        return;
    else if (!fp)
    {
        log("sigreactor: recieved %s; no signal handler!\n", signame(sig).c_str());
        return;
    }

    // Using sigsetjmp(), we can catch SEGV etc. (if caused by
    // fp->handler()) and get back to this place.
    int e = sigsetjmp(m_jmp, 1);
    if ( e == 0 )
        fp->handler(sig, si);
    else
    {
        log("sigreactor: sighandler(for %s) %s caused %s\n",
                signame(sig).c_str(), fp->c_name(), signame(e).c_str());
    }
}


// small predicate that returns true if the signal was sent by
// ourselves via sigqueue(2).
static inline bool
from_self_p(const siginfo_t& si)
{
    const pid_t self = ::getpid();

    return si.si_code == SI_QUEUE &&
            MAGIC_COOKIE == si.si_value.sival_int && si.si_pid == self;
}



// Thread entry point to manage synchronous signal handling
void
sig_reactor::thread_entry()
{
    thread_init();

    while (1)
    {
        siginfo_t si;
        int sig = 0;
        sigset p;

        //log("sigreactor: blocking & waiting for signals..\n");
        p.fill();
        sig = sigwaitinfo(p, &si);
        if (sig < 0)
        {
            if (errno != EINTR)
                log("sigreactor: sigwaitinfo failed: %s (%d)\n", strerror(errno), errno);
            continue;
        }

        //log("sigreactor: Got %s\n", signame(sig).c_str());

        // Self generated signals get special treatment
        if (from_self_p(si))
        {
            if (SIGTERM == sig)
                break;
            else if (SIGHUP == sig)
                continue;   // re-enter the loop
        }


        // Before we go dispatch 'sig' see if there are other
        // signals that are pending.
        int err = sigpending(p);
        if (err < 0)
        {
            if (errno != EINTR)
                log("sigreactor: sigpending failed: %s (%d)\n", strerror(errno), errno);
            continue;
        }
        else
            p.add(sig); // add the newly recieved signal to the set

        //log("sigreactor: pending sigs: %s\n", p.c_str());

        // Tedious task of looping through pending sigs
        memset(&si, 0, sizeof si);
        for (int i = 1; i < NSIG; ++i)
        {
            if (p.has(i))
            {
                si.si_signo = i;
                call_handler(i, si);
            }
        }
    }

    //log("sigreactor: ending thread..\n");
}




// thread specific OS signal handler for hardware-synchronous signals such
// as SEGV, ILL, BUS, FPE. In this case, we don't poke any messages
// as it is quite useless. Instead, we use siglongjmp() to stop the
// offending signal handler (registered via "add()") from causing
// any more problems.
// Note:
//   This signal handler is _only_ executed for SEGV, ILL, BUS, FPE.
void
sig_reactor::reactor_sig_handler(int signum, siginfo_t *, void *)
{
#if 0
    static int count = 0;
    char buf[64];

    count++;

    int n = snprintf(buf, sizeof buf, "\n**THREAD-SIG-%d  [%d]\n", signum, count);
    write(2, buf, n);
#endif

    sig_reactor& r = sig_reactor::instance();
    siglongjmp(r.jmpbuf(), signum);
}


// - implementation of sigset's string rep -
string
sigset::str()
{
    string x;

    for (int i = 1; i < NSIG; ++i)
    {
        if (sigismember(&m_sig, i))
        {
            x += signame(i);
            x += " ";
        }
    }
    return x;
}



}

/* EOF */
