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

#include "threadapi.h"
#include <stdio.h>
#include "zend.h" 
#ifdef TSRM_WIN32
#include <process.h>
#endif

THR_RW_LOCK *thr_create_rwlock(void)
{
	THR_RW_LOCK *l = (THR_RW_LOCK *)malloc(sizeof(THR_RW_LOCK));
#ifdef TSRM_WIN32
	l->lock = tsrm_mutex_alloc();
	l->event = thr_create_event();
#elif defined(PTHREADS)

	l->lock =  (pthread_mutex_t *) malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(l->lock,NULL);
	l->event =  thr_create_event();

       
        
        
#else
#endif
	l->count = 0;
	l->reset = 0;
	return l;
}

void thr_close_rwlock(THR_RW_LOCK *rwlock)
{
#ifdef TSRM_WIN32
	tsrm_mutex_free(rwlock->lock);
	thr_close_event(rwlock->event);
#elif defined(PTHREADS)
	pthread_mutex_destroy(rwlock->lock);
	thr_close_event(rwlock->event);
	free(rwlock->lock);
#else
#endif
	free(rwlock);
}

void thr_acquire_write_lock(THR_RW_LOCK *rwlock)
{
#ifdef TSRM_WIN32
	HANDLE muxHandle[2];
	muxHandle[0] = rwlock->event;
	muxHandle[1] = rwlock->lock;
	WaitForSingleObject(rwlock->lock,INFINITE);
	while (rwlock->count != 0) {
		rwlock->reset = 0;
		ReleaseMutex(rwlock->lock);
		WaitForMultipleObjects(2, muxHandle, TRUE, INFINITE);
		if (rwlock->reset == 0) {
			rwlock->reset = 1;
			ResetEvent(rwlock->event);
		}
	}
	rwlock->count = -1;
	ReleaseMutex(rwlock->lock);
#elif defined(PTHREADS)
	pthread_mutex_lock(rwlock->lock);
	while (rwlock->count != 0)
		pthread_cond_wait(rwlock->event->cond, rwlock->lock);
	rwlock->count = -1;
	pthread_mutex_unlock(rwlock->lock);
#else
#endif
}

void thr_release_write_lock(THR_RW_LOCK *rwlock)
{
#ifdef TSRM_WIN32
	WaitForSingleObject(rwlock->lock, INFINITE);
	rwlock->count = 0;
	SetEvent(rwlock->event);
	ReleaseMutex(rwlock->lock);
#elif defined(PTHREADS)
	pthread_mutex_lock(rwlock->lock);
	rwlock->count = 0;
	pthread_cond_broadcast(rwlock->event->cond);
	pthread_mutex_unlock(rwlock->lock);
#else
#endif
}

void thr_acquire_read_lock(THR_RW_LOCK *rwlock)
{
#ifdef TSRM_WIN32
	HANDLE muxHandle[2];
	muxHandle[0] = rwlock->event;
	muxHandle[1] = rwlock->lock;
	WaitForSingleObject(rwlock->lock,INFINITE);
	while (rwlock->count < 0) {
		rwlock->reset = 0;
		ReleaseMutex(rwlock->lock);
		WaitForMultipleObjects(2, muxHandle, TRUE, INFINITE);
		if (rwlock->reset == 0) {
			rwlock->reset = 1;
			ResetEvent(rwlock->event);
		}
	}
	++rwlock->count;
	ReleaseMutex(rwlock->lock);
#elif defined(PTHREADS)
	pthread_mutex_lock(rwlock->lock);
	while (rwlock->count < 0)
		pthread_cond_wait(rwlock->event->cond, rwlock->lock);
	++rwlock->count;
	pthread_mutex_unlock(rwlock->lock);
#else
#endif
}

void thr_release_read_lock(THR_RW_LOCK *rwlock)
{
#ifdef TSRM_WIN32
	WaitForSingleObject(rwlock->lock, INFINITE);
	--rwlock->count;
	if (!rwlock->count) SetEvent(rwlock->event);
	ReleaseMutex(rwlock->lock);
#elif defined(PTHREADS)
	pthread_mutex_lock(rwlock->lock);
	--rwlock->count;
	if (!rwlock->count)
		pthread_cond_broadcast(rwlock->event->cond);
	pthread_mutex_unlock(rwlock->lock);
#else
#endif
}
 

/* create an event or condition var */
THR_EVENT thr_create_event(void)
{
#ifdef TSRM_WIN32
	return CreateEvent(NULL,FALSE,FALSE,NULL);
#elif defined(PTHREADS)
	THR_EVENT c = (THR_EVENT) malloc(sizeof(THR_COND_VAR));
	c->mutex =  (pthread_mutex_t *) malloc(sizeof(pthread_mutex_t));
	c->cond = (pthread_cond_t *)  malloc(sizeof(pthread_cond_t));
	pthread_mutex_init(c->mutex,NULL);
	pthread_cond_init(c->cond,NULL);
	
	return c;
#elif defined(NETWARE)
	return NULL;
#elif defined(GNUPTH)
	return NULL;
#elif defined(NSAPI)
	return NULL;
#elif defined(PI3WEB)
	return NULL;
#elif defined(TSRM_ST)
	return NULL;
#else
	return NULL;
#endif
}

int thr_wait_event(THR_EVENT event, int timeout)
{
#ifdef TSRM_WIN32
	return WaitForSingleObject(event, timeout);
#elif defined(PTHREADS) 
       
	 
	if (timeout == THR_INFINITE) {
		int ret;
		pthread_mutex_lock(event->mutex);
		ret=pthread_cond_wait(event->cond, event->mutex);
		pthread_mutex_unlock(event->mutex);
		return ret;
	} else {
		int ret;
		struct timespec ts_timeout;
		struct timeval now;
		gettimeofday(&now);
		ts_timeout.tv_sec = now.tv_sec;
		ts_timeout.tv_nsec = now.tv_usec + timeout;
		pthread_mutex_lock(event->mutex);
		ret = pthread_cond_timedwait(event->cond, event->mutex, &ts_timeout);
		pthread_mutex_unlock(event->mutex);
		return ret;
	}
#elif defined(NETWARE)
	return -1;
#elif defined(GNUPTH)
	return -1;
#elif defined(NSAPI)
	return -1;
#elif defined(PI3WEB)
	return -1;
#elif defined(TSRM_ST)
	return -1;
#else
	return -1;
#endif
}

int thr_set_event(THR_EVENT event)
{
#ifdef TSRM_WIN32
	return SetEvent(event);
#elif defined(PTHREADS)
	int ret;
	pthread_mutex_lock(event->mutex);
	ret =pthread_cond_broadcast(event->cond);
	pthread_mutex_unlock(event->mutex);
	return ret;
#elif defined(NETWARE)
	return -1;
#elif defined(GNUPTH)
	return -1;
#elif defined(NSAPI)
	return -1;
#elif defined(PI3WEB)
	return -1;
#elif defined(TSRM_ST)
	return -1;
#else
	return -1;
#endif
}

int thr_reset_event(THR_EVENT event)
{
#ifdef TSRM_WIN32
	return ResetEvent(event);
#elif defined(PTHREADS)
	/* no need to reset condition variables */
	return -1;
#elif defined(NETWARE)
	return -1;
#elif defined(GNUPTH)
	return -1;
#elif defined(NSAPI)
	return -1;
#elif defined(PI3WEB)
	return -1;
#elif defined(TSRM_ST)
	return -1;
#else
	return -1;
#endif
}

void thr_close_event(THR_EVENT event)
{
#ifdef TSRM_WIN32
	CloseHandle(event);
#elif defined(PTHREADS)
	pthread_cond_destroy(event->cond);
	pthread_mutex_destroy(event->mutex);
	free(event->cond);
	free(event->mutex);
	free(event); 
#elif defined(NETWARE)
#elif defined(GNUPTH)
#elif defined(NSAPI)
#elif defined(PI3WEB)
#elif defined(TSRM_ST)
#else
	return;
#endif
}


int thr_thread_create(THR_THREAD *thread, THR_THREAD_PROC_P callback)
{
	thread->id = 0;
#ifdef TSRM_WIN32
	thread->thread=(HANDLE)_beginthreadex(NULL,0,callback,(void *)thread,0,&thread->id);
#elif defined(PTHREADS)
	pthread_create(  &(thread->thread), NULL, callback, (void *)thread);
#elif defined(NETWARE)
#elif defined(GNUPTH)
#elif defined(NSAPI)
#elif defined(PI3WEB)
#elif defined(TSRM_ST)
#endif
	return thread->id;
}

void thr_thread_exit(unsigned int retval)
{
#ifdef TSRM_WIN32
	_endthreadex(retval);
#elif defined(PTHREADS)
	pthread_exit((void *) &retval);
#elif defined(NETWARE)
#elif defined(GNUPTH)
#elif defined(NSAPI)
#elif defined(PI3WEB)
#elif defined(TSRM_ST)
#else
	return;
#endif
}

void thr_wait_exit(THR_THREAD *thread)
{
#ifdef TSRM_WIN32
	WaitForSingleObject(thread->thread, THR_INFINITE);
#elif defined(PTHREADS)
	pthread_join(thread->thread,NULL);
#elif defined(NETWARE)
#elif defined(GNUPTH)
#elif defined(NSAPI)
#elif defined(PI3WEB)
#elif defined(TSRM_ST)
#else
	return;
#endif
}
