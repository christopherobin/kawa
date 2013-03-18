#ifndef PHP_KAWA_H
#define PHP_KAWA_H

#include "uv.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"

#define KAWA_MODULE_VERSION "0.1.0-dev"
#define KAWA_FOREACH(ht) for (zend_hash_internal_pointer_reset(ht); zend_hash_has_more_elements(ht) == SUCCESS; zend_hash_move_forward(ht))
#define KAWA_FOREACH_ENTRY(ht,dest) if (zend_hash_get_current_data(ht,(void**)& dest) == FAILURE) {\
	continue;\
}
#define KAWA_DEFAULT_BIND_ADDRESS "0.0.0.0"

/** A pool in Kawa is a uv_loop instance, with it's associated streams */
typedef struct _kawa_pool_instance {
	zend_object				zo;
	uv_loop_t				*loop;
	uv_signal_t				sigint;
} kawa_pool_instance;

/** This is the EventEmitter instance */
typedef struct _kawa_eventemitter_instance {
	zend_object				zo;
	HashTable				*events;
	int						max_listeners;
} kawa_eventemitter_instance;

#define KAWA_USE_EVENTEMITTER union {\
	zend_object					zo;\
	kawa_eventemitter_instance  event;\
}

/** This is the TCP server instance */
typedef struct _kawa_network_tcp_instance {
	KAWA_USE_EVENTEMITTER;
	zval						*pool;
	uv_tcp_t					server;
} kawa_network_tcp_instance;

/** This is the TCP socket instance */
typedef struct _kawa_network_socket_instance {
	KAWA_USE_EVENTEMITTER;
	uv_tcp_t					socket;
} kawa_network_socket_instance;

/** A small helper to store php callbacks */
typedef struct _php_callback {
	zend_fcall_info			fci;
	zend_fcall_info_cache   fci_cache;
	// delete this callback after one use?
	zend_bool				once;
} php_callback;

typedef struct _php_call_arguments {
	zval ***varargs;
	int num_vararg;
} php_call_arguments;

/** Those are our globals */
ZEND_BEGIN_MODULE_GLOBALS(kawa)
	zval *default_loop;
ZEND_END_MODULE_GLOBALS(kawa)

#ifdef ZTS
#define KAWA_G(v) TSRMG(kawa_globals_id, zend_kawa_globals *, v)
#else
#define KAWA_G(v) (kawa_globals.v)
#endif

/** Emit a signal from an EventEmitter class */
KAWA_EMIT(signal, vararg)\
};

/** Our module init function */
PHP_MINIT_FUNCTION(kawa);
PHP_MSHUTDOWN_FUNCTION(kawa);
PHP_RSHUTDOWN_FUNCTION(kawa);
PHP_MINFO_FUNCTION(kawa);

/** Here is the descriptions for the \Kawa\Pool class */
extern zend_class_entry *kawa_pool_class_entry;

/** Finally the \Kawa\Pool constructor and methods */
extern void kawa_pool_init();
extern void kawa_pool_shutdown();
PHP_METHOD(Pool, run);
PHP_METHOD(Pool, once);
PHP_METHOD(Pool, stop);
PHP_METHOD(Pool, getDefault);

/** Here is the descriptions for the \Kawa\EventEmitter class */
extern zend_class_entry *kawa_eventemitter_class_entry;

/** Finally the \Kawa\EventEmitter constructor and methods */
extern void kawa_eventemitter_init();
extern void kawa_eventemitter_setup(kawa_eventemitter_instance *instance TSRMLS_DC);
extern void kawa_eventemitter_destroy(kawa_eventemitter_instance *instance TSRMLS_DC);
PHP_METHOD(EventEmitter, setMaxListeners);
PHP_METHOD(EventEmitter, getMaxListeners);
PHP_METHOD(EventEmitter, emit);
PHP_METHOD(EventEmitter, on);
PHP_METHOD(EventEmitter, once);
PHP_METHOD(EventEmitter, all);
PHP_METHOD(EventEmitter, off);
PHP_METHOD(EventEmitter, getListeners);

/** Here is the descriptions for the \Kawa\Pool class */
extern zend_class_entry *kawa_network_tcp_class_entry;

/** Finally the \Kawa\Network\TCP constructor and methods */
extern void kawa_network_tcp_init();
PHP_METHOD(TCP, __construct);
PHP_METHOD(TCP, listen);
PHP_METHOD(TCP, getPool);

/** Here is the descriptions for the \Kawa\Network\Socket class */
extern zend_class_entry *kawa_network_socket_class_entry;

/** Finally the \Kawa\Network\Socket constructor and methods */
extern void kawa_network_socket_init();
PHP_METHOD(Socket, __construct);
PHP_METHOD(Socket, connect);
PHP_METHOD(Socket, read);
PHP_METHOD(Socket, write);

/** This is the module entry */
extern zend_module_entry kawa_module_entry;

#endif//PHP_KAWA_H