dnl config.m4 for kawa-php

PHP_ARG_WITH(kawa, for kawa support,
[	--with-kawa[=libuv]	Include the Kawa library. Libuv is the optional path for the libuv library.])

dnl Check if the user enabled the extension
if test "$PHP_KAWA" != "no"; then

	dnl Check if the user gave a path for libuv
	AC_MSG_CHECKING([for libuv])
	if test "$PHP_KAWA" != "yes"; then
		LIBUV_PATH=$PHP_KAWA
	else
		LIBUV_PATH="../libuv"
	fi

	dnl Check that libuv is compiled
	if test -f "$LIBUV_PATH/libuv.a" && test -f "$LIBUV_PATH/include/uv.h"; then
		AC_MSG_RESULT([$LIBUV_PATH])

		dnl Check that the lib works properly
		AC_CHECK_LIB([pthread], [uv_get_free_memory], [
			PHP_ADD_INCLUDE("$LIBUV_PATH/include")
			UV_LIB="$LIBUV_PATH/libuv.a"
		],[
			dnl Bail out
			AC_MSG_ERROR([libuv not found. Check config.log for more information.])
		], ["$LIBUV_PATH/libuv.a"])
	else
		dnl Invalid path, bail out
		AC_MSG_RESULT([not found])
		AC_MSG_ERROR([Please check that you retrieved and compiled libuv.])
	fi


	PHP_NEW_EXTENSION(kawa, kawa.c kawa/pool.c kawa/events.c kawa/network/tcp.c kawa/network/socket.c, $ext_shared)
	PHP_SUBST(KAWA_SHARED_LIBADD)
	KAWA_SHARED_LIBADD="$KAWA_SHARED_LIBADD $LIBUV_PATH/libuv.a"
fi