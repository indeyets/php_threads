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

#ifndef PHP_THREADS_H
#define PHP_THREADS_H

extern zend_module_entry threads_module_entry;
#define phpext_threads_ptr &threads_module_entry

#ifdef PHP_WIN32
#define PHP_THREADS_API __declspec(dllexport)
#else
#define PHP_THREADS_API
#endif

#ifdef ZTS
#include "TSRM.h"
#include "threadapi.h"
#endif

PHP_MINIT_FUNCTION(threads);
PHP_MSHUTDOWN_FUNCTION(threads);
PHP_RINIT_FUNCTION(threads);
PHP_RSHUTDOWN_FUNCTION(threads);
PHP_MINFO_FUNCTION(threads);

/* __function_declarations_here__ */
PHP_FUNCTION(thread_start);
PHP_FUNCTION(thread_include);
PHP_FUNCTION(thread_set);
PHP_FUNCTION(thread_isset);
PHP_FUNCTION(thread_unset);
PHP_FUNCTION(thread_get);
PHP_FUNCTION(thread_mutex_init);
PHP_FUNCTION(thread_mutex_destroy);
PHP_FUNCTION(thread_lock);
PHP_FUNCTION(thread_lock_try);
PHP_FUNCTION(thread_unlock);


typedef struct _thr_shared_vars {
	HashTable vars;
	THR_RW_LOCK *rwlock;
} THR_SHARED_VARS;


ZEND_BEGIN_MODULE_GLOBALS(threads)
	zend_llist children; /* list to hold list of child threads */
	THR_THREAD *self;
	//THR_SHARED_VARS *vars;
ZEND_END_MODULE_GLOBALS(threads)


/* In every utility function you add that needs to use variables
   in php_extname_globals, call TSRM_FETCH(); after declaring other
   variables used by that function, or better yet, pass in TSRMLS_CC
   after the last function argument and declare your utility function
   with TSRMLS_DC after the last declared argument.  Always refer to
   the globals in your function as EXTNAME_G(variable).  You are
   encouraged to rename these macros something shorter, see
   examples in any other php module directory.
*/

/* define our TSRM style macros to access parent thread data
   during thread startup.  Most of these are not used yet, but here
   for convenience until development is done. */

#define THR_TSRMG(ptsrm_ls, id, type, element)	(((type) (*((void ***) ptsrm_ls))[TSRM_UNSHUFFLE_RSRC_ID(id)])->element)
#define THR_SG(p,v) THR_TSRMG(p,sapi_globals_id, sapi_globals_struct *, v)
#define THR_CG(p,v) THR_TSRMG(p,compiler_globals_id, zend_compiler_globals *, v)
#define THR_EG(p,v) THR_TSRMG(p,executor_globals_id, zend_executor_globals *, v)
#define THR_AG(p,v) THR_TSRMG(p,alloc_globals_id, zend_alloc_globals *, v)
#define THR_LANG_SCNG(p,v) THR_TSRMG(p,language_scanner_globals_id, zend_scanner_globals *, v)
#define THR_INI_SCNG(p,v) THR_TSRMG(p,ini_scanner_globals_id, zend_scanner_globals *, v)
#define THR_PG(p,v) THR_TSRMG(p,core_globals_id, php_core_globals *, v)

#ifdef ZTS
#define THREADS_G(v) TSRMG(threads_globals_id, zend_threads_globals *, v)
#else
#define THREADS_G(v) (threads_globals.v)
#endif

#endif	/* PHP_EXTNAME_H */

/* __footer_here__ */