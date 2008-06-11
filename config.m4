dnl $Id: config.m4,v 1.2 2002/08/08 09:45:05 alan_k Exp $
dnl config.m4 for extension threads

 
PHP_ARG_WITH(threads, for threads support,
Make sure that the comment is aligned:
[  --with-threads             Include threads support])


if test "$PHP_THREADS" != "no"; then
  dnl Write more examples of tests here...

  dnl # --with-threads -> check with-path
  dnl SEARCH_PATH="/usr/local /usr"     # you might want to change this
  dnl SEARCH_FOR="/include/threads.h"  # you most likely want to change this
  dnl if test -r $PHP_THREADS/; then # path given as parameter
  dnl   THREADS_DIR=$PHP_THREADS
  dnl else # search default path list
  dnl   AC_MSG_CHECKING([for threads files in default path])
  dnl   for i in $SEARCH_PATH ; do
  dnl     if test -r $i/$SEARCH_FOR; then
  dnl       THREADS_DIR=$i
  dnl       AC_MSG_RESULT(found in $i)
  dnl     fi
  dnl   done
  dnl fi
  dnl
  dnl if test -z "$THREADS_DIR"; then
  dnl   AC_MSG_RESULT([not found])
  dnl   AC_MSG_ERROR([Please reinstall the threads distribution])
  dnl fi

  dnl # --with-threads -> add include path
  dnl PHP_ADD_INCLUDE($THREADS_DIR/include)

  dnl # --with-threads -> chech for lib and symbol presence
  dnl LIBNAME=threads # you may want to change this
  dnl LIBSYMBOL=threads # you most likely want to change this 

  dnl PHP_CHECK_LIBRARY($LIBNAME,$LIBSYMBOL,
  dnl [
  dnl   PHP_ADD_LIBRARY_WITH_PATH($LIBNAME, $THREADS_DIR/lib, THREADS_SHARED_LIBADD)
  dnl   AC_DEFINE(HAVE_THREADSLIB,1,[ ])
  dnl ],[
  dnl   AC_MSG_ERROR([wrong threads lib version or lib not found])
  dnl ],[
  dnl   -L$THREADS_DIR/lib -lm -ldl
  dnl ])
  dnl
  dnl PHP_SUBST(THREADS_SHARED_LIBADD)
  
  
  dnl if test `uname` = "Linux"; then
    THREADS_PTHREAD=pthread
  dnl else
  dnl   THREADS_PTHREAD=
  dnl fi
  
  dnl PHP_ADD_LIBRARY(pthread)
  
  
  PHP_NEW_EXTENSION(threads, threads.c threadapi.c, $ext_shared)
fi

