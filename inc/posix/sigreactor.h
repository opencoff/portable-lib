/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * sigreactor.h - Reactor to manage reliable management of signals
 *                *THIS IS A SINGLETON* class
 *
 * Note: This header file is part of the generic process framework.
 *       It is not meant for public exposure (by other classes).
 *
 * Licensing Terms: GPLv2 
 *
 * If you need a commercial license for this work, please contact
 * the author.
 *
 * This software does not come with any express or implied
 * warranty; it is provided "as is". No claim  is made to its
 * suitability for any purpose.
 */

#ifndef __SIGREACTOR_H_1130267966__
#define __SIGREACTOR_H_1130267966__ 1

#include <sys/types.h>
#include <signal.h>
#include <setjmp.h>

#include <vector>
#include <string>

#include "utils/noncopyable.h"
#include "utils/mutex.h"
#include "utils/lock.h"

namespace posix {

// Handy encapsulation of a signal set; a sort of OO frontend to
// sigset_t.
class sigset
{
public:
    sigset(int s = 0)
    {
        sigemptyset(&m_sig);
        if (s)
            sigaddset(&m_sig, s);
    }

    // initialization with raw sigset_t
    sigset(const sigset_t& s)
    {
        m_sig = s;
    }

    sigset(const sigset& s)
    {
        m_sig = s.m_sig;
    }

    sigset& operator=(const sigset& s)
    {
        if (&s != this)
            m_sig = s.m_sig;
        return *this;
    }

    void fill()        { sigfillset(&m_sig); }
    void add(int s)    { sigaddset(&m_sig, s); }
    void del(int s)    { sigdelset(&m_sig, s); }
    void remove(int s) { del(s); }

    bool has(int s) const { return !!sigismember(&m_sig, s); }

    // allow people to use this as a sigset_t
    operator sigset_t() const { return m_sig; }

    // Or as sigset_t*
    operator sigset_t* () { return &m_sig; }
    operator const sigset_t* () const { return &m_sig; }

    // string representation of signals in the set
    std::string str();
    const char* c_str() { return str().c_str(); }

private:
    sigset_t   m_sig;
};


// Generic signal handler.
//   Applications interested in handling signals must derive from this
//   class and provide an implementation for handler(). The signal
//   handler allows an implementor to use the same derived class to
//   handle multiple signals.
//
//   The ctor argument 'name' is used by the reactor framework to log
//   errors (if any).
//
//   Since the same thread is used to dispatch multiple signals, the
//   callers must NOT block excessively. It is best if an
//   implementation of handler() restricts themselves to the same
//   sort of limitations as OS signal handlers (ala sigaction(2) or
//   signal(2)).
class signal_handler
{
public:
    // Users of this class must give us a descriptive name for this
    // signal handler.
    signal_handler(const std::string& name) : m_name(name) {}
    virtual ~signal_handler();

    virtual void handler(int signum, const siginfo_t&) = 0;

    // Return human readable name for this signal handler
    const std::string& name() { return m_name; }
    const char* c_name()      { return m_name.c_str(); }

private:
    // human readable name of this signal handler
    std::string m_name;

private:
    signal_handler();
    signal_handler(const signal_handler&);
    signal_handler& operator=(const signal_handler&);
};




// This is the signal handling "reactor" framework.
// It is implemented as a thread that responds to events posted by
// the low-level "OS signal handlers".
class sig_reactor : public putils::non_copyable
{
private:
    typedef std::vector<signal_handler *> db_type;

    // messages the outside world can send to the signal handling
    // thread
    enum msg_t
    {
          REACTOR_QUIT  = 101
        , REACTOR_QUIT_ACK
        , REACTOR_SIG
    };

    // messages exchanged between signal handler and the thread
    struct msg
    {
        msg_t type;

        // anon union
        union
        {
            // valid iff type is M_SIG
            siginfo_t si;
        };
    };

public:

    // singleton accessor
    static sig_reactor& instance();


    // Convenience functions that work on single signals
    void block(const int sig);
    void unblock(const int sig);

    void ignore(const int sig);
    void unignore(const int sig);

    void add(const int sig, signal_handler *);
    void remove(const int sig);

    // Stop the reactor
    void stop();

private: // to implement singleton behavior
    sig_reactor();
    ~sig_reactor();
    sig_reactor(const sig_reactor&);
    sig_reactor& operator=(const sig_reactor&);

    // Log a message via the system logger
    void log(const char * fmt, ...);

    // virtual function for use by Thread
    virtual void thread_entry();

    // send a message to the thread
    void queue_msg(const sig_reactor::msg &, int flags = 0);

    // Return my jump buffer
    sigjmp_buf& jmpbuf() { return m_jmp; }

    // initialize the sig_reactor thread
    void thread_init();

    // Call a registered signal handler (if one exists)
    void call_handler(int, const siginfo_t&);

    // wakeup the sigreactor thread
    void wakeup(int sig);


    //  OS interface to deliver signals
    static void sig_handler(int s, siginfo_t *, void *);

    // thread specific handler for thread-synchronous signals such
    // as SEGV, ILL, BUS, FPE
    static void reactor_sig_handler(int, siginfo_t *, void *);

    // This member function is used to create the singleton and
    // assign it to sig_reactor::reactor_instance.
    // Furthermore, this function can only be called by
    // Process::Start().
    static sig_reactor* create();

    void  wait_for_create();

private:
    // For synchronized access to all datastructures below
    putils::mutex           m_mutex;

    // signals we are interested in, blocked and ignored
    sigset m_blocked;
    sigset m_ignored;


    db_type m_sig_handlers;

    // For communication with real sighandler
    int     m_socketpair[2];

    // Jump buffer for siglongjmp()
    sigjmp_buf m_jmp;
};

}

#endif /* ! __SIGREACTOR_H_1130267966__ */

/* EOF */
