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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/* debugging - use THR_PRINTF(("somthing %s", string)); - note the double brackets.. */
#define THR_DEBUG
 
#ifdef THR_DEBUG
#define THR_PRINTF(v) printf v; fflush(stdout);
#else
#define THR_PRINTF(v)
#endif

//#define USE_SERIALIZE

#include "php.h"
#include "php_globals.h"
#include "php_main.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_threads.h"
#ifdef USE_SERIALIZE
# include "ext/standard/php_smart_str.h"
# include "ext/standard/php_var.h"
#endif

#ifndef ZTS
/* ZTS REQUIRED */
#endif


/* Our shared vars storage */
THR_SHARED_VARS *shared_vars = NULL;

ZEND_DECLARE_MODULE_GLOBALS(threads)

/* True global resources - no need for thread safety here */
static int le_threads;

/* {{{ threads_functions[]
 *
 * Every user visible function must have an entry in threads_functions[].
 */
function_entry threads_functions[] = {
	PHP_FE(thread_start,	NULL)
	PHP_FE(thread_include,	NULL)
	PHP_FE(thread_set,	NULL)
	PHP_FE(thread_isset,	NULL)
	PHP_FE(thread_unset,	NULL)
	PHP_FE(thread_get,	NULL)
	PHP_FE(thread_mutex_init,NULL)
	PHP_FE(thread_mutex_destroy,NULL)
	PHP_FE(thread_lock,	NULL)
	PHP_FE(thread_lock_try,	NULL)
	PHP_FE(thread_unlock,	NULL)
	{NULL, NULL, NULL}	/* Must be the last line in threads_functions[] */
};
/* }}} */

/* {{{ threads_module_entry
 */
zend_module_entry threads_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
	STANDARD_MODULE_HEADER,
#endif
	"threads",
	threads_functions,
	PHP_MINIT(threads),
	PHP_MSHUTDOWN(threads),
	PHP_RINIT(threads),		/* Replace with NULL if there's nothing to do at request start */
	PHP_RSHUTDOWN(threads),	/* Replace with NULL if there's nothing to do at request end */
	PHP_MINFO(threads),
#if ZEND_MODULE_API_NO >= 20010901
	"0.1", /* Replace with version number for your extension */
#endif
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_THREADS
ZEND_GET_MODULE(threads)
#endif

/* {{{ PHP_INI
 */
/* Remove comments and fill if you need to have entries in php.ini
PHP_INI_BEGIN()
    STD_PHP_INI_ENTRY("threads.global_value",      "42", PHP_INI_ALL, OnUpdateInt, global_value, zend_threads_globals, threads_globals)
    STD_PHP_INI_ENTRY("threads.global_string", "foobar", PHP_INI_ALL, OnUpdateString, global_string, zend_threads_globals, threads_globals)
PHP_INI_END()
*/
/* }}} */

/* {{{ _join_children
 */
static void _php_threads_join_children(void *data)
{
	/* TODO: join or wait for children to end */
	THR_THREAD *thread = (THR_THREAD *) data;

	THR_PRINTF(("threads:_php_threads_join_children\n"));
#ifdef TSRM_WIN32
	thr_wait_exit(thread);
#endif

#ifdef PTHREADS
	pthread_join(thread->thread, NULL);
#endif
	
	thr_close_event(thread->start_event);
	

	//efree chrashes at shutdown, so "normal" free will be used
	//TSRM_FETCH seems to crash
	//efree(thread);
	THR_PRINTF(("threads:_php_threads_join_children ready\n"));
	
	thread = NULL;
}


/* }}} */

/* {{{ php_threads_init_globals
 */
 
static void _php_threads_init_globals(zend_threads_globals *threads_globals)
{
}
/* }}} */

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(threads)
{
	THR_PRINTF(("thread:minit\n"));
	ZEND_INIT_MODULE_GLOBALS(threads, _php_threads_init_globals, NULL);
	/* If you have INI entries, uncomment these lines 
	REGISTER_INI_ENTRIES();
	*/ 



	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(threads)
{
	/* uncomment this line if you have INI entries
	UNREGISTER_INI_ENTRIES();
	*/ 
	/* TODO: initialize some kind of shared memory for shared vars */
	THR_PRINTF(("module shutdown in thread\n"));
	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request start */
/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(threads)
{
	/* TODO: initialize a list for child threads */
	THR_PRINTF(("thread:rinit\n"));
	THREADS_G(self) = NULL;
	//shared_vars = NULL;

	if (!shared_vars) {
		/* only the master thread creates this */
                /* avoid emalloc ... - it gets totally confused with the threading stuff */
		shared_vars = (THR_SHARED_VARS *) malloc(sizeof(THR_SHARED_VARS));
		THR_PRINTF(("thread:rinit VARS\n"));
		shared_vars->rwlock = thr_create_rwlock();
		zend_hash_init(&shared_vars->vars, 0, NULL, NULL,0);
	}
	
	//zend_hash_init(&THREADS_G(children), 0, NULL, _php_threads_join_children, 1);	
	zend_llist_init(&THREADS_G(children), sizeof(THR_THREAD),_php_threads_join_children, 0);
	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request end */
/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
PHP_RSHUTDOWN_FUNCTION(threads)
{
	/* destroying the THREADS_G(children) makes this thread wait for
	   all THREADS_G(children) to exit */
	THR_PRINTF(("thread:rshutdown\n"));
	if(THREADS_G(children).count >= 1) {
		THR_PRINTF(("thread:THREADS_G(children) gefunden\n"));
		//TODO: wait for THREADS_G(children) to be executed; send KILL signal 
		//zend_hash_destroy(&THREADS_G(children));
		zend_llist_destroy(&THREADS_G(children));
	}
	if (!THREADS_G(self)) {
		THR_PRINTF(("thread:rshutdown:thread_self\n"));
		/* since self was never set, this is the master thread */
		//zend_hash_destroy(&shared_vars->vars);
		//thr_close_rwlock(shared_vars->rwlock);
		/* Daniel: Of course this will only be called if there are some shared vars */
		if(shared_vars && shared_vars->vars.nNumOfElements > 0) {
			THR_PRINTF(("thread:shared vars found: %d\n", shared_vars->vars.nNumOfElements));
		}
		zend_hash_destroy(&shared_vars->vars);
		thr_close_rwlock(shared_vars->rwlock);
		free(shared_vars);

	} else {
		//thr_wait_exit(THREADS_G(self));
		//Nothing to do at this point
		//thr_thread_exit(0);
	}
	THR_PRINTF(("request shutdown in thread\n"));
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(threads)
{
	php_info_print_table_start(); 
	php_info_print_table_header(2, "Threads support", "enabled");
	php_info_print_table_end();

	/* Remove comments if you have entries in php.ini
	DISPLAY_INI_ENTRIES();
	*/
}
/* }}} */


THR_THREAD_PROC(phpthreads_create)
{
	zval *result = NULL;
	zend_file_handle file_handle;
//	zval ret_val;
	zval *local_retval = NULL;
	zend_op_array *orig_op_array = NULL;
	zend_op_array *op_array = NULL;
	zval callback;
	zval args;
	zval *argv[1];
	int callback_len = 0;
	THR_THREAD *thread = (THR_THREAD *) data;
	TSRMLS_FETCH();
	THREADS_G(self) = thread;
	/* we need to have the same context as the parent thread */
	SG(server_context) = THR_SG(thread->parent_tls,server_context);

	/* 
	   TODO: copy system level data now.  We need copy the global zend
	   structures so that this thread has it's own copy of them. 
	*/
	SG(request_info).path_translated = estrdup(THR_SG(thread->parent_tls,request_info).path_translated);
	/* copy our arguments */
	COPY_PZVAL_TO_ZVAL(callback,(zval *)thread->callback);
	COPY_PZVAL_TO_ZVAL(args,(zval *)thread->args[0]);
	argv[0] = &args;
	/* notify we're done copying, don't touch 'thread' after this, 
	   it will be invalid memory! */
	thr_set_event(thread->start_event);	

	php_request_startup(TSRMLS_C);

#ifdef PHP_WIN32
	//UpdateIniFromRegistry(SG(request_info).path_translated TSRMLS_CC);
#endif

	THR_PRINTF(("Call User Func ??????\n\n==================n\n"));
	/* this can be an issue we have to deal with somehow */
	SG(headers_sent) = 1;
	SG(request_info).no_headers = 1;

	file_handle.handle.fp = VCWD_FOPEN(SG(request_info).path_translated, "rb");
	file_handle.filename = SG(request_info).path_translated;                       
	
	file_handle.type = ZEND_HANDLE_FP;
	file_handle.opened_path = NULL;
	file_handle.free_filename = 0;

	EG(exit_status) = 0;
	//PG(during_request_startup) = 0;

	op_array = zend_compile_file(&file_handle, ZEND_INCLUDE TSRMLS_CC);
	
//	EG(return_value_ptr_ptr) = &result;
	EG(active_op_array) = op_array;

    //zend_execute(op_array TSRMLS_CC);

	orig_op_array = EG(active_op_array);
//	new_op_array= EG(active_op_array) = zend_compile_file(&file_handle, ZEND_INCLUDE TSRMLS_CC);*/
	zend_destroy_file_handle(&file_handle TSRMLS_CC);
	if (EG(active_op_array)) {
		EG(return_value_ptr_ptr) = &local_retval;

		/* start the thread in php user land! Instead of calling 
		   php_execute_script, we've copied the stuff and can now
		   simply call into a user function */
		 
		//retval = (zend_execute_scripts(ZEND_REQUIRE TSRMLS_CC, NULL, 3, prepend_file_p, primary_file, append_file_p) == SUCCESS);
		//zend_execute(EG(active_op_array) TSRMLS_CC);
		if (!call_user_function(EG(function_table), NULL, &callback, local_retval, 1, argv TSRMLS_CC ))  {
			zend_error(E_ERROR, "Problem Starting thread with callback");
		}

		zval_dtor(&callback);
		zval_dtor(&args);
		//zval_dtor(&ret_val);
		//zval_ptr_dtor(EG(return_value_ptr_ptr));

		local_retval = NULL;
		
#ifdef ZEND_ENGINE_2
		destroy_op_array(EG(active_op_array) TSRMLS_CC);
#else
		destroy_op_array(EG(active_op_array));
#endif
		efree(op_array);
		EG(active_op_array) = orig_op_array;
	}

	php_request_shutdown(NULL);
 
	thr_thread_exit(0);
} 

/* {{{ proto string thread_start(string function_name, [any args])
   Return a string to confirm that the module is compiled in */
PHP_FUNCTION(thread_start)
{
	zval *callback;
	THR_THREAD *thread = NULL;
	zval *args;
	int threadid;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z|z", &callback, &args) == FAILURE) {
		return;
	}
	thread = (THR_THREAD *) emalloc(sizeof(THR_THREAD));
	thread->start_event = thr_create_event();
	thread->callback = callback;
	thread->parent_tls = tsrm_ls;
	thread->args[0] = (void *) args;

	threadid = thr_thread_create(thread, (void *) phpthreads_create);
	/* we wait here until the child thread has copied the
	   parent threads data */
	thr_wait_event(thread->start_event,THR_INFINITE);
	zend_llist_add_element(&THREADS_G(children), (void *)thread);
//	zend_hash_index_update(&THREADS_G(children), threadid, (void *)thread);
	RETURN_TRUE; 
}
/* }}} */


/* THR_THREAD_PROC(phpthreads_include) {  */
void phpthreads_include (void * data) {
    
	zval *result = NULL;
	zend_op_array *op_array;
	int callback_len = 0;
	zend_file_handle file_handle;
	THR_THREAD *thread = (THR_THREAD *) data;
	/* void **thr_data = thread->args; */
	char *script_file; 

/* Threads should be detached if necessary or set by user
#ifdef PTHREADS
	pthread_detach(pthread_self());
#endif
*/	
	/*char *script_file = (char *)estrdup((char *) thr_data[0]);*/
	//THR_SHARED_VARS *vars = (THR_SHARED_VARS *) thread->args[1];
	TSRMLS_FETCH();

	script_file = (char *) thread->args[0];
	THR_PRINTF(("starting thread %s \n",  script_file)) 

	//thread->args[0] = NULL;
	/* we need to have the same context as the parent thread */
	SG(server_context) = THR_SG(thread->parent_tls,server_context);
	
	/* do standard stuff just like cli sapi */

	php_request_startup(TSRMLS_C);

	THREADS_G(self) = thread; /* is ok */
	//shared_vars = vars; /* is ok */
	
	THR_PRINTF(("I'm done doing the unsafe stuff\n")); 
	thr_set_event(thread->start_event);
	
	SG(request_info).path_translated = script_file;

	/* this can be an issue we have to deal with somehow */
	SG(headers_sent) = 1;
	SG(request_info).no_headers = 1;

	file_handle.handle.fp = VCWD_FOPEN(script_file, "rb");
	file_handle.filename = script_file;                       
	
	file_handle.type = ZEND_HANDLE_FP;
	file_handle.opened_path = NULL;
	file_handle.free_filename = 0;
	/* notify we're done copying, don't touch 'thread' after this, 
	   it will be invalid memory! */
	
	THR_PRINTF(("sent - running script \n")); 
	
	op_array = zend_compile_file(&file_handle, ZEND_INCLUDE TSRMLS_CC);
	zend_destroy_file_handle(&file_handle TSRMLS_CC);

	EG(return_value_ptr_ptr) = &result;
	EG(active_op_array) = op_array;

	zend_execute(op_array TSRMLS_CC);

	destroy_op_array(op_array TSRMLS_CC);
	efree(op_array);
	op_array = NULL;
	efree(*EG(return_value_ptr_ptr));

	//php_execute_script(&file_handle TSRMLS_CC);
	
	THR_PRINTF(("done running script \n")); 
	
	/* self destruct  DOES NOT WORK FOR SOME REASON! */

	php_request_shutdown(NULL); /* NOW IT WORKS */
	
	THR_PRINTF(("done request shutdown \n")); 

	/* found in tsrm.c and is extremly useful */
	ts_free_thread();

	//free(thread);
	thr_thread_exit(0);

} 

/* {{{ proto string thread_start(string filename)
   Return a string to confirm that the module is compiled in */
PHP_FUNCTION(thread_include)
{
	char *script_file=NULL;
	char *script_file_in;
	int len;
	int threadid;

	THR_THREAD *thread = NULL;
	//THR_SHARED_VARS *vars = shared_vars;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &script_file_in,&len) == FAILURE) {
		RETURN_FALSE;
	} 
	 
//	if (!shared_vars) {
		/* only the master thread creates this */
                /* avoid emalloc ... - it gets totally confused with the threading stuff */
//		shared_vars = (THR_SHARED_VARS *) malloc(sizeof(THR_SHARED_VARS));
//		shared_vars->rwlock = thr_create_rwlock();
//		zend_hash_init(&shared_vars->vars, 0, NULL, NULL,0);
//		vars = shared_vars;
//	}
	script_file= (char *) emalloc(len + 1);
	strncpy(script_file , script_file_in,  len+1);
         /* avoid emalloc ... - it gets totally confused with the threading stuff */
	thread = (THR_THREAD *) malloc(sizeof(THR_THREAD));
	thread->start_event = thr_create_event();
	thread->callback = NULL;
	thread->parent_tls = tsrm_ls;
	thread->args[0] =  script_file;
	thread->args[1] =  NULL;
	THR_PRINTF(("value of args is %x\n", thread->args));
	 
	thread->id = thr_thread_create(thread, (void *) phpthreads_include); 
	/* we wait here until the child thread has copied the
	   parent threads data */
	
	THR_PRINTF(("done creating thread  - waiting for all clear  \n")) 

    
	/* waiting for thread to start... */
#ifdef TSRM_WIN32
	thr_wait_event(thread->start_event, THR_INFINITE);
#endif

	THR_PRINTF(("got message that thread had finished initializing   \n"))
	
//	thread->args[0] = NULL;

	//zend_hash_index_update(&THREADS_G(children), threadid, (void *) thread, sizeof(THR_THREAD *), NULL);
	zend_llist_add_element(&THREADS_G(children), (void *)thread);	

	free(thread);
	efree(script_file);

	RETURN_TRUE;
}
/* }}} */

/* {{{ proto boolean thread_set(string name, mixed variable)
   set a value into shared thread storage */
PHP_FUNCTION(thread_set)
{
	char *name;
	unsigned int len;
	zval *var;
	zval *newvar = NULL, **xvar=NULL;
	THR_SHARED_VARS *vars = shared_vars;
#ifdef USE_SERIALIZE	
	smart_str new_var = {0};
	char *ser_str = NULL;
	php_serialize_data_t var_hash;
#else
	zval *target;
#endif

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sz", &name,&len,&var) == FAILURE) {
		RETURN_FALSE;
	} 

	THR_PRINTF(("thread:thread_set==%s==\n", name));

#ifdef USE_SERIALIZE	
	/* serialize variables that will be shared */
	PHP_VAR_SERIALIZE_INIT(var_hash);
	php_var_serialize(&new_var, &var, &var_hash TSRMLS_CC);
	PHP_VAR_SERIALIZE_DESTROY(var_hash);
	ser_str = estrndup(new_var.c,new_var.len);

	thr_acquire_write_lock(shared_vars->rwlock);
	zend_hash_update(&shared_vars->vars, name, len+1, ser_str, new_var.len + 1,NULL);
	thr_release_write_lock(shared_vars->rwlock);
    
	THR_PRINTF(("thread_set: setting serialized var:\n %s \n\n\n", ser_str));

	smart_str_free(&new_var);
#else
	/* attempt to save an actual zval to avoid serialization overhead,
	   and to allow for resources to be shared (if possible?) */
	/* TODO: need to make a complete copy of var */
	thr_acquire_write_lock(shared_vars->rwlock);
	
	switch(Z_TYPE_PP(&var)) {
		case IS_RESOURCE:
			//TODO: special Handling for Resources
			THR_PRINTF(("RESOURCE ====================================================\n"));
			break;
		case IS_OBJECT:
			THR_PRINTF(("OBJECT ====================================================\n"));
			//TODO: Special Handling for Objects
			break;
		default:
			//TODO: This doesn't work for calls like thread_set('var', 'value')
			//Only for calls like thread_set('var', $var);
			/*MAKE_STD_ZVAL(target);
			*target = *var;
			zval_copy_ctor(target);*/ //Commenting this in does produce memleaks reported by DEBUG-mode
			zend_hash_update(&shared_vars->vars, name, len+1, (void *) &var, sizeof(zval *),NULL);
			break;

	}



	/*if (zend_hash_find(&shared_vars->vars, name, len+1, (void **) &xvar) == SUCCESS) {
		if(Z_TYPE_PP(xvar) == IS_STRING) {
			convert_to_string_ex(xvar);
		}
		THR_PRINTF(("thread_set: hash find ok \n"));
	}*/


	thr_release_write_lock(shared_vars->rwlock);
#endif
	RETURN_TRUE; 
}
/* }}} */

/* {{{ proto mixed thread_get(string name[, int wait])
   get a value from shared thread storage */
PHP_FUNCTION(thread_get)
{
	char *name;
	unsigned int len;
#ifdef USE_SERIALIZE
	char *new_var;
	zval *tmp = NULL;
#else
	zval **tmp;
#endif
	//Not neededTHR_SHARED_VARS *vars = shared_vars;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &name,&len) == FAILURE) {
		RETURN_FALSE;
	}
    
    THR_PRINTF(("thread_get: Trying to return %s\n", name));

#ifdef USE_SERIALIZE
	thr_acquire_read_lock(shared_vars->rwlock);
	if (zend_hash_find(&shared_vars->vars, name, len+1, (void **) &new_var) == SUCCESS) {
			php_unserialize_data_t var_hash;
			const char *p = (const char*)new_var;
			MAKE_STD_ZVAL(tmp);
			PHP_VAR_UNSERIALIZE_INIT(var_hash);
			if (!php_var_unserialize(&tmp, &p, p+strlen(p), &var_hash TSRMLS_CC)) {
				zend_error(E_WARNING, "%s(): message corrupted", get_active_function_name(TSRMLS_C));
				RETVAL_FALSE;
			}
		

			REPLACE_ZVAL_VALUE(&return_value, tmp, 0);
			zval_copy_ctor(return_value);
			INIT_PZVAL(return_value);
			FREE_ZVAL(tmp);
			PHP_VAR_UNSERIALIZE_DESTROY(var_hash);
			THR_PRINTF(("thread_get: successfully returned %s\n", name));
	} else {
		THR_PRINTF(("thread_get: not successfully found %s\n", name));
	}
	thr_release_read_lock(shared_vars->rwlock);
#else
	/* for some reason, we cannot get zval's back if set from a different 
	   thread */
	thr_acquire_read_lock(shared_vars->rwlock);
	if (zend_hash_find(&shared_vars->vars, name, len+1, (void **)&tmp) == SUCCESS) {
		switch(Z_TYPE_PP(tmp)) {
			case IS_RESOURCE:
				//TODO: special Handling for Resources
				break;
			case IS_OBJECT:
				//TODO: Special Handling for Objects
				break;
			default:
				REPLACE_ZVAL_VALUE(&return_value, *tmp, 1);
				break;

		}
	}
	thr_release_read_lock(shared_vars->rwlock);
#endif
}
/* }}} */

/* {{{ proto boolean thread_isset(string name)
   check existense of a shared var */
PHP_FUNCTION(thread_isset)
{

	char *name;
	unsigned int len;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &name,&len) == FAILURE) {
		RETURN_FALSE;
	}

	if(zend_hash_exists(&shared_vars->vars, name, len + 1)) {
		THR_PRINTF(("thread_isset: %s exists\n", name));
		RETURN_TRUE;
	} else {
		RETURN_FALSE;
	}

}
/* }}} */


/* {{{ proto boolean thread_isset(string name)
   check existense of a shared var */
PHP_FUNCTION(thread_unset)
{

	char *name;
	unsigned int len;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &name,&len) == FAILURE) {
		RETURN_FALSE;
	}

	if(zend_hash_exists(&shared_vars->vars, name, len + 1)) {
		THR_PRINTF(("thread_unset: unsetting %s \n", name));
		zend_hash_del(&shared_vars->vars, name, len + 1);
	}
	
	RETURN_TRUE;

}
/* }}} */


/* {{{ proto boolean thread_mutex_init(string name)
   create a mutex resource */
PHP_FUNCTION(thread_mutex_init)
{
	/* NOTE: we create the mutex, stuff it into shared thread storage
	   under the provided name.  Any thread can then use that mutex
	   by using the name of the mutex, rather than the value */
	RETURN_FALSE; 
}
/* }}} */

/* {{{ proto boolean thread_mutex_destroy(string name)
   destroy a mutex resource */
PHP_FUNCTION(thread_mutex_destroy)
{
	RETURN_FALSE; 
}
/* }}} */

/* {{{ proto boolean thread_lock(string name)
   blocking lock on a mutex */
PHP_FUNCTION(thread_lock)
{
	RETURN_FALSE; 
}
/* }}} */

/* {{{ proto boolean thread_lock_try(string name)
   non-blocking lock on a mutex */
PHP_FUNCTION(thread_lock_try)
{
	RETURN_FALSE; 
}
/* }}} */

/* {{{ proto boolean thread_unlock(string name)
   unlock a locked mutex */
PHP_FUNCTION(thread_unlock)
{
	RETURN_FALSE; 
}
/* }}} */




/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
