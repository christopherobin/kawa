#include "php_kawa.h"

#include "zend_variables.h"

#define KAWA_EVENTEMITTER_MAX_LISTENERS 10

/** The class entry */
zend_class_entry *kawa_eventemitter_class_entry;

ZEND_BEGIN_ARG_INFO(arginfo_kawa_eventemitter_void, 0)
ZEND_END_ARG_INFO()

/** Our list of methods */
zend_function_entry kawa_eventemitter_functions[] = {
	PHP_ME(EventEmitter, __construct, arginfo_kawa_eventemitter_void, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
	PHP_ME(EventEmitter, __destruct, arginfo_kawa_eventemitter_void, ZEND_ACC_PUBLIC|ZEND_ACC_DTOR)
	PHP_ME(EventEmitter, getMaxListeners, arginfo_kawa_eventemitter_void, ZEND_ACC_PUBLIC)
	PHP_ME(EventEmitter, setMaxListeners, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(EventEmitter, emit, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(EventEmitter, on, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(EventEmitter, once, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(EventEmitter, all, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(EventEmitter, off, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(EventEmitter, getListeners, NULL, ZEND_ACC_PUBLIC)
	PHP_FE_END
};

static zend_object_handlers kawa_eventemitter_instance_handlers;

/* {{{ void kawa_eventemitter_init()
 * Register the \Kawa\Pool into PHP */
void kawa_eventemitter_init()
{
	zend_class_entry kawa_eventemitter_ce;

	// setup default handlers
	memcpy(&kawa_eventemitter_instance_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));

	INIT_NS_CLASS_ENTRY(kawa_eventemitter_ce, "Kawa", "EventEmitter", kawa_eventemitter_functions);
	kawa_eventemitter_class_entry = zend_register_internal_class(&kawa_eventemitter_ce TSRMLS_CC);

	zend_declare_property_null(kawa_eventemitter_class_entry, "events", strlen("events"), ZEND_ACC_PRIVATE);
}
/* }}} */

zval *fetch_property_events(zval *this_ptr, zend_bool create) {
	zval *events = zend_read_property(Z_OBJCE_P(this_ptr), this_ptr, "events", strlen("events"), 1 TSRMLS_CC);
	// instanciate property if necessary
	if (create == 1) {
		if (Z_TYPE_P(events) != IS_ARRAY) {
			array_init(events);
		}
		zend_update_property(Z_OBJCE_P(this_ptr), this_ptr, "events", strlen("events"), events TSRMLS_CC);
	}
	return events;
}

/* {{{ proto public 
 * */
PHP_METHOD(EventEmitter, __construct)
{
	php_printf("debug: EventEmitter::__construct called\n");
}
/* }}} */

/* {{{ proto public 
 * */
PHP_METHOD(EventEmitter, __destruct)
{
	zval *events = zend_read_property(Z_OBJCE_P(getThis()), getThis(), "events", strlen("events"), 1 TSRMLS_CC);
	if (Z_TYPE_P(events) == IS_ARRAY) {
		zend_hash_destroy(HASH_OF(events));
		FREE_HASHTABLE(HASH_OF(events));
	}
}
/* }}} */

/* {{{ proto public 
 * */
PHP_METHOD(EventEmitter, setMaxListeners)
{
}
/* }}} */

/* {{{ proto public 
 * */
PHP_METHOD(EventEmitter, getMaxListeners)
{
}
/* }}} */

/* {{{ proto public 
 * */
PHP_METHOD(EventEmitter, emit)
{
	char						*name = NULL;
	int							name_len = 0, num_varargs = 0;
	zval						***varargs = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s*", &name, &name_len, &varargs, &num_varargs) == FAILURE) {
		RETURN_FALSE;
	}

	if (varargs) {
		efree(varargs);
	}
}
/* }}} */

/* {{{ proto public EventEmitter::on($event_name, $callback)
 * Register a callback for a given event name */
PHP_METHOD(EventEmitter, on)
{
	char						*name = NULL;
	int							name_len = 0;
	php_callback				callback;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sf", &name, &name_len, &callback.fci, &callback.fci_cache) == FAILURE) {
		RETURN_FALSE;
	}

	zval *events = fetch_property_events(getThis(), 1);
	// check for an entry that works
	HashTable *events_ht = HASH_OF(events);
	zval *listeners;
	if (zend_hash_find(events_ht, name, name_len, (void**)&listeners) == FAILURE) {
		// then create that shit
		add_assoc_null(events, name, name_len);
	}
	
	zend_create_closure(return_value, callback.fci_cache.function_handler, callback.fci_cache.calling_scope, getThis() TSRMLS_CC);
}
/* }}} */

/* {{{ proto public 
 * */
PHP_METHOD(EventEmitter, once)
{
}
/* }}} */

/* {{{ proto public 
 * */
PHP_METHOD(EventEmitter, all)
{
}
/* }}} */

/* {{{ proto public 
 * */
PHP_METHOD(EventEmitter, off)
{
}
/* }}} */

/* {{{ proto public 
 * */
PHP_METHOD(EventEmitter, getListeners)
{
}
/* }}} */
