/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * timer_wheel.h - Hashed Timer Wheel Class
 *
 * Copyright (c) 2010 Sudhi Herle <sw at herle.net>
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
 * Creation date: Oct 20, 2010 09:55:01 CDT
 *
 * Implementation based on Scheme-7 of the design outlined
 * in George Vargehese's paper: 
 *    www.cs.columbia.edu/~nahum/w6998/papers/ton97-timing-wheels.pdf
 *
 * This implementation is inspired by the NetBSD callout
 * implementation (sys/callout.h, kern_timeout.c).
 *
 * This implementation deals with units of "ticks". The duration of
 * each tick is left for the derived classes to define and
 * implement.
 *
 * The class 'hashed_timer_wheel' is therefore a pure virtual class.
 * It requires a derived class to implement two things:
 *    o background thread that counts "ticks"
 *    o notion of 'tick'
 */

#ifndef ___TIMER_WHEEL_H_9547274_1287586501__
#define ___TIMER_WHEEL_H_9547274_1287586501__ 1

#include <map>
#include <list>
#include <string>
#include "utils/mutex.h"
#include "utils/lock.h"




/* Base class that implements the functionality of hashed timer
 * wheel. Other than the CTOR, nothing else is exposed. All the
 * interesting functions are available for the derived classes to
 * fill up.
 */
class hashed_timer_wheel
{
public:
    typedef void*   timer_id_t;

    hashed_timer_wheel();
    virtual ~hashed_timer_wheel();

protected:
    struct timeout;
    typedef std::list<timeout *> timeout_list_type;
    typedef std::map<timer_id_t, timeout*> active_map_type;
    typedef putils::scoped_lock<putils::mutex>  timer_lock;

protected:
    // Add a new timer to expire in 'interval' ticks from now.
    // if repeat is true, then making this a repeating timer (that
    // rearms itself after expiry. The function returns a unique ID
    // for the timer.
    timer_id_t add(unsigned long interval, void (*func)(void*), void* cookie, bool repeat=false);
    void del(timer_id_t);

    // Internal functions
    void insert(timeout*);
    void do_callbacks(timeout_list_type* expired);
    void expire(timeout_list_type* expired, unsigned long w, unsigned long tm);

    // Internal functions to map a timer_id_t to a timeout struct
    // and vice versa.
    timer_id_t timeout2id(timeout*);
    timeout*   id2timeout(timer_id_t);

    // Same as id2timeout(), but also invalidates the timer_id when
    // the lookup is performed
    timeout*   invalidate_id(timer_id_t);

    // Debug function to dump all timer wheels
    void       dump_wheels(const std::string& msg);

    // To be called by the derived class per "tick".
    // If the concrete implementation cannot call at every tick
    // (expensive), then call tick() in a loop for as many elapsed
    // ticks.
    void tick();

    // Function to return the current time in units of "ticks"
    virtual unsigned long ticks_now() = 0;

protected:

    // XXX Implement fixed size allocator for this class
    struct timeout
    {
        unsigned int  flags;    // flags
        unsigned long time;     // when this fires
        unsigned long interval; // repeat interval
        void (*callback)(void *);
        void *cookie;
        timeout_list_type* head;

        timeout(unsigned long exp, void (*fp)(void *), void * opaq, 
                 bool repeat = false);
    };


    /*
     * Notes on locking:
     *   This class uses one lock per instance. This may be OK for a
     *   small list of timers. However, for scalable uses, the
     *   locking may have to be per hash-bucket.
     */

    timeout_list_type   wheel[WHEEL_BUCKETS];

    unsigned long       ticks;  // elapsed ticks
    active_map_type     valid;  // map<> of valid timers

    putils::mutex        lock;   // to protect list integrity

    // counters to measure delays/slips
    unsigned long       nlate;
    unsigned long       ntimers;


protected:

    unsigned long hash(unsigned long w, unsigned long tm)
    {
        return WHEEL_MASK & (tm >> (w * WHEEL_BITS));
    }


    // Return the hash bucket given a relative expiry time and an
    // absolute expiry time
    timeout_list_type* bucket(unsigned long rel, unsigned long abs);

};

#endif /* ! ___TIMER_WHEEL_H_9547274_1287586501__ */

/* EOF */
