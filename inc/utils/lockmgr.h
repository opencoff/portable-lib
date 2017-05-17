/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * lockmgr.h - Object oriented multithreaded lock interface.
 *
 * Copyright (c) 2005 Sudhi Herle <sw at herle.net>
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

#ifndef __LOCKMGR_H_1026925511__
#define __LOCKMGR_H_1026925511__ 1

    /* Provide C linkage for symbols declared here .. */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* Virtual lock structure */
struct lockmgr
{
    void (*create)(struct lockmgr*);
    void (*lock)(void *);
    void (*unlock)(void *);
    void (*dtor)(void*);

    void * opaq;
};
typedef struct lockmgr lockmgr;


/* Create a new lock */
#define lockmgr_create(m) do { \
                            if ( (m)->create ) \
                                (*(m)->create)(m); \
                            else { \
                                (m)->lock = 0; \
                                (m)->unlock = 0; \
                                (m)->dtor = 0; \
                            } \
                        } while (0)

/* obtain a lock */
#define lockmgr_lock(m) do { \
                            if ( (m)->lock ) \
                                (*(m)->lock)((m)->opaq); \
                        } while (0)



/* release a lock */
#define lockmgr_unlock(m)   do { \
                                if ( (m)->unlock ) \
                                    (*(m)->unlock)((m)->opaq); \
                            } while (0)



/* Delete a lock */
#define lockmgr_delete(m)   do { \
                                    if ((m)->dtor) (*(m)->dtor)((m)->opaq); \
                                    (m)->lock   = 0; \
                                    (m)->unlock = 0; \
                                    (m)->dtor   = 0; \
                            } while (0)



/* CTOR to create a NULL lock. */
#define lockmgr_new_empty(l)    do { \
                                   (l)->lock   = 0; \
                                   (l)->unlock = 0; \
                                } while (0)



/* Various lockers available to be cloned. */
extern const lockmgr Mutex_locker;
extern const lockmgr Null_locker;


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ! __LOCKMGR_H_1026925511__ */

/* EOF */
