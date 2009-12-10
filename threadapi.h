/*
   +----------------------------------------------------------------------+
   | PHP Version 4                                                        |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997-2003 The PHP Group                                |
   +----------------------------------------------------------------------+
   | This source file is subject to version 2.02 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available at through the world-wide-web at                           |
   | http://www.php.net/license/2_02.txt.                                 |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Author: Shane Caraveo <shane@php.net>                                |
   | Author: Alan Knowles <alan@akbkhome.com>                             |
   +----------------------------------------------------------------------+
*/

#ifndef THREADAPI_H
#define THREADAPI_H

#include "TSRM.h"


#ifdef TSRM_WIN32
# define THR_THREAD_T HANDLE
# define THR_THREAD_PROC(a) void a(void *data)
# define THR_THREAD_PROC_P void *
# define THR_EVENT HANDLE
# define THR_WAIT_ABANDONED WAIT_ABANDONED
# define THR_WAIT_DONE WAIT_OBJECT_0
# define THR_WAIT_TIMEOUT WAIT_TIMEOUT
# define THR_INFINITE INFINITE
# define THR_NOWAIT 0


#elif defined(PTHREADS)
# define THR_THREAD_T pthread_t
# define THR_THREAD_PROC(a) void a(void *data)
# define THR_THREAD_PROC_P  void *

typedef struct _cond_var {
	pthread_cond_t *cond;
	pthread_mutex_t *mutex;
} _thr_cond_var;

# define THR_COND_VAR _thr_cond_var
# define THR_EVENT _thr_cond_var *
# define THR_WAIT_ABANDONED WAIT_ABANDONED
# define THR_WAIT_DONE WAIT_OBJECT_0
# define THR_WAIT_TIMEOUT WAIT_TIMEOUT
# define THR_INFINITE -1
# define THR_NOWAIT 0

/* TODO: finish other platforms */
#elif defined(NETWARE)
#elif defined(GNUPTH)
#elif defined(NSAPI)
#elif defined(PI3WEB)
#elif defined(TSRM_ST)
#endif

typedef struct _read_write_lock {
	MUTEX_T lock;
	THR_EVENT event;
	unsigned int count;
	unsigned int reset;
} THR_RW_LOCK;

THR_RW_LOCK *thr_create_rwlock(void);
void thr_close_rwlock(THR_RW_LOCK *rwlock);
void thr_acquire_write_lock(THR_RW_LOCK *rwlock);
void thr_release_write_lock(THR_RW_LOCK *rwlock);
void thr_acquire_read_lock(THR_RW_LOCK *rwlock);
void thr_release_read_lock(THR_RW_LOCK *rwlock);

/* EXPERIMENTAL 
   this stuff is used only in the threads extension for now,
   but TSRM is realy the proper place for it */

/* create an event or condition var */
THR_EVENT thr_create_event(void);
/* wait for an event or condition to be signaled.  timeout in
   milliseconds */
int thr_wait_event(THR_EVENT event, int timeout);
/* signal an event */
int thr_set_event(THR_EVENT event);
/* reset an event */
int thr_reset_event(THR_EVENT event);
/* free or close an event */
void thr_close_event(THR_EVENT event);


/* EXPERIMENTAL 
   this stuff is used only in the threads extension for now,
   but TSRM is realy the proper place for it */

/* XXX will have to think about this more.  Right now
   this structure is used to pass zend structures to the
   thread so that the thread can make copies, it also is
   used for result data.  It is not persisted past the
   thread creation. */
typedef struct _THR_thread {
	THR_THREAD_T thread;
	unsigned long id;
	unsigned long exitcode;
	void ***parent_tls; /* set to tsrm_ls */
	void *callback; /* holds php function name */
	void *args[2]; /* holds variable number of args*/
	THR_EVENT start_event;
} THR_THREAD;

int thr_thread_create(THR_THREAD *thread, THR_THREAD_PROC_P callback);
void thr_thread_exit(unsigned int retval);
void thr_wait_exit(THR_THREAD *thread);

#endif /* THREADAPI_H */