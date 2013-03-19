#include "php_kawa.h"

#include "zend_variables.h"

#define KAWA_EVENTEMITTER_MAX_LISTENERS 10

/** The class entry */
zend_class_entry *kawa_eventemitter_class_entry;

ZEND_BEGIN_ARG_INFO(arginfo_kawa_eventemitter_void, 0)
ZEND_END_ARG_INFO()

/** Our list of methods */
zend_function_entry kawa_eventemitter_functions[] = {
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

void kawa_eventemitter_free_object(void *object TSRMLS_DC)
{
	kawa_eventemitter_instance *instance = (kawa_eventemitter_instance *)object;

	// free our stuff
	kawa_eventemitter_destroy(instance TSRMLS_CC);

	zend_object_std_dtor(&instance->zo TSRMLS_CC);
	efree(object);
}

zend_object_value kawa_eventemitter_create_object(zend_class_entry *class_entry TSRMLS_DC)
{
	zend_object_value retval;

	// alloc memory for this instance
	kawa_eventemitter_instance *instance = emalloc(sizeof(kawa_eventemitter_instance));
	memset(instance, 0, sizeof(kawa_eventemitter_instance));

	// setup our zend object
	zend_object_std_init(&instance->zo, class_entry TSRMLS_CC);
	object_properties_init(&instance->zo, class_entry);

	// setup our internals
	kawa_eventemitter_setup(instance TSRMLS_CC);

	// setup the return value
	retval.handle = zend_objects_store_put(instance, NULL, kawa_eventemitter_free_object, NULL TSRMLS_CC);
	retval.handlers = &kawa_eventemitter_instance_handlers;

	return retval;
}

void kawa_eventemitter_setup(kawa_eventemitter_instance *instance TSRMLS_DC)
{
	instance->max_listeners = KAWA_EVENTEMITTER_MAX_LISTENERS;
	instance->add_cb = NULL;
	instance->del_cb = NULL;

	// this hashtable store our events
	ALLOC_HASHTABLE(instance->events);
	// 10 events is a safe default, php will automatically alloc more if necessary
	zend_hash_init(instance->events, 10, NULL, NULL, 0 TSRMLS_CC);
}

void kawa_eventemitter_destroy(kawa_eventemitter_instance *instance TSRMLS_DC)
{
	// iterate on each events
	KAWA_FOREACH(instance->events) {
		HashTable *listeners;
		KAWA_FOREACH_ENTRY(instance->events, listeners);
		// remove our references to the callback names
		KAWA_FOREACH(listeners) {
			php_callback *callback;
			KAWA_FOREACH_ENTRY(listeners, callback);
			zval_ptr_dtor(&callback->fci.function_name);
		}
		zend_hash_destroy(listeners);
		// no need to free, destroying the parent hashtable will do it for us
	}

	zend_hash_destroy(instance->events);
	FREE_HASHTABLE(instance->events);
}

/* {{{ void kawa_eventemitter_init()
 * Register the \Kawa\Pool into PHP */
void kawa_eventemitter_init()
{
	zend_class_entry kawa_eventemitter_ce;

	// init the class entry
	INIT_NS_CLASS_ENTRY(kawa_eventemitter_ce, "Kawa", "EventEmitter", kawa_eventemitter_functions);
	kawa_eventemitter_class_entry = zend_register_internal_class(&kawa_eventemitter_ce TSRMLS_CC);
	kawa_eventemitter_class_entry->create_object = kawa_eventemitter_create_object;

	// setup default handlers
	memcpy(&kawa_eventemitter_instance_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
}
/* }}} */

/* {{{ int kawa_eventemitter_apply_emit(void *pDest, void *argument TSRMLS_DC)
 * Hash apply function that emit a signal, and delete callbacks if necessary */
int kawa_eventemitter_apply_emit(void *pDest, void *argument TSRMLS_DC)
{
	php_callback *callback = (php_callback*)pDest;
	php_call_arguments *arguments = (php_call_arguments*)argument;

	zval *retval;
	callback->fci.params = arguments->varargs;
	callback->fci.param_count = arguments->num_vararg;
	callback->fci.retval_ptr_ptr = &retval;

	// call it
	zend_call_function(&callback->fci, &callback->fci_cache TSRMLS_CC);

	// we don't care about the return value atm, just destroy it
	zval_ptr_dtor(&retval);

	// if we only capture once, free memory by removing that callback
	if (callback->once) {
		zval_ptr_dtor(&callback->fci.function_name);
		return ZEND_HASH_APPLY_REMOVE;
	}

	return ZEND_HASH_APPLY_KEEP;
}
/* }}} */

/* {{{ int kawa_eventemitter_apply_off(void *pDest, void *argument TSRMLS_DC)
 * Iterate on a listener list and remove listeners matching the one provided */
int kawa_eventemitter_apply_off(void *pDest, void *argument TSRMLS_DC)
{
	php_callback *stored_callback = (php_callback*)pDest;
	php_callback *target_callback = (php_callback*)argument;

	// remove callback if it match, and reduce references on function name
	if (stored_callback->fci_cache.function_handler == target_callback->fci_cache.function_handler) {
		zval_ptr_dtor(&stored_callback->fci.function_name);
		return ZEND_HASH_APPLY_REMOVE;
	}

	return ZEND_HASH_APPLY_KEEP;
}
/* }}} */

/* {{{ proto public 
 * */
PHP_METHOD(EventEmitter, setMaxListeners)
{
	kawa_eventemitter_instance *instance;
	long requested_listeners;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &requested_listeners) == FAILURE) {
		RETURN_FALSE;
	}

	instance = (kawa_eventemitter_instance*)zend_object_store_get_object(getThis() TSRMLS_CC);
	instance->max_listeners = requested_listeners;
	RETURN_TRUE;
}
/* }}} */

/* {{{ proto public 
 * */
PHP_METHOD(EventEmitter, getMaxListeners)
{
	kawa_eventemitter_instance *instance = (kawa_eventemitter_instance*)zend_object_store_get_object(getThis() TSRMLS_CC);
	RETURN_LONG(instance->max_listeners);
}
/* }}} */

/* {{{ proto public 
 * */
PHP_METHOD(EventEmitter, emit)
{
	char						*name = NULL;
	int							name_len = 0;
	php_call_arguments			arguments;
	kawa_eventemitter_instance  *instance;

	// set that to 0 to prevent... fun
	arguments.varargs = NULL;
	arguments.num_vararg = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s*", &name, &name_len, &arguments.varargs, &arguments.num_vararg) == FAILURE) {
		RETURN_FALSE;
	}

	instance = (kawa_eventemitter_instance*)zend_object_store_get_object(getThis() TSRMLS_CC);

	HashTable *listeners;
	if (zend_hash_find(instance->events, name, name_len, (void**)&listeners) == FAILURE) {
		RETURN_NULL();
	}

	// apply the callback for each listeners
	zend_hash_apply_with_argument(listeners, kawa_eventemitter_apply_emit, &arguments TSRMLS_CC);

	if (arguments.varargs != NULL) {
		efree(arguments.varargs);
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
	kawa_eventemitter_instance  *instance;

	callback.once = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sf", &name, &name_len, &callback.fci, &callback.fci_cache) == FAILURE) {
		RETURN_FALSE;
	}

	instance = (kawa_eventemitter_instance*)zend_object_store_get_object(getThis() TSRMLS_CC);

	// check if we already have a hashtable stored
	HashTable *listeners;
	if (zend_hash_find(instance->events, name, name_len, (void**)&listeners) == FAILURE) {
		// otherwise create it
		HashTable tmp;
		zend_hash_init(&tmp, KAWA_EVENTEMITTER_MAX_LISTENERS, NULL, NULL, 0 TSRMLS_CC);
		if (zend_hash_add(instance->events, name, name_len, &tmp, sizeof(HashTable), (void**)&listeners) == FAILURE) {
			// we couldn't add this event :(
			RETURN_FALSE;
		}
	}

	// check if we already have the maximum amount of listeners on this event
	if (zend_hash_num_elements(listeners) >= instance->max_listeners) {
		// throw a nice exception
		zend_throw_exception(zend_exception_get_default(TSRMLS_C), "Tried to add one too many listeners on an event.", 0 TSRMLS_CC);
	}

	// call any callback set
	if (instance->add_cb != NULL) {
		instance->add_cb(instance, name, name_len, &callback);
	}

	// now store the callback
	Z_ADDREF_P(callback.fci.function_name);
	if (zend_hash_next_index_insert(listeners, &callback, sizeof(php_callback), NULL) == FAILURE) {
		RETURN_FALSE;
	}

	RETURN_TRUE;
}
/* }}} */

/* {{{ proto public 
 * */
PHP_METHOD(EventEmitter, once)
{
	char						*name = NULL;
	int							name_len = 0;
	php_callback				callback;
	kawa_eventemitter_instance  *instance;

	callback.once = 1;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sf", &name, &name_len, &callback.fci, &callback.fci_cache) == FAILURE) {
		RETURN_FALSE;
	}

	instance = (kawa_eventemitter_instance*)zend_object_store_get_object(getThis() TSRMLS_CC);

	// check if we already have a hashtable stored
	HashTable *listeners;
	if (zend_hash_find(instance->events, name, name_len, (void**)&listeners) == FAILURE) {
		// otherwise create it
		HashTable tmp;
		zend_hash_init(&tmp, KAWA_EVENTEMITTER_MAX_LISTENERS, NULL, NULL, 0 TSRMLS_CC);
		if (zend_hash_add(instance->events, name, name_len, &tmp, sizeof(HashTable), (void**)&listeners) == FAILURE) {
			// we couldn't add this event :(
			RETURN_FALSE;
		}
	}

	// check if we already have the maximum amount of listeners on this event
	if (zend_hash_num_elements(listeners) >= instance->max_listeners) {
		// throw a nice exception
		zend_throw_exception(zend_exception_get_default(TSRMLS_C), "Tried to add one too many listeners on an event.", 0 TSRMLS_CC);
	}

	// call any callback set
	if (instance->add_cb != NULL) {
		instance->add_cb(instance, name, name_len, &callback);
	}

	// now store the callback
	Z_ADDREF_P(callback.fci.function_name);
	if (zend_hash_next_index_insert(listeners, &callback, sizeof(php_callback), NULL) == FAILURE) {
		RETURN_FALSE;
	}

	RETURN_TRUE;
}
/* }}} */

/* {{{ proto public 
 * */
PHP_METHOD(EventEmitter, all)
{
	zend_throw_exception(zend_exception_get_default(TSRMLS_C), "Not implemented.", 0 TSRMLS_CC);
}
/* }}} */

/* {{{ proto public 
 * */
PHP_METHOD(EventEmitter, off)
{
	char						*name = NULL;
	int							name_len = 0;
	php_callback				callback;
	kawa_eventemitter_instance  *instance;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sf", &name, &name_len, &callback.fci, &callback.fci_cache) == FAILURE) {
		RETURN_FALSE;
	}

	instance = (kawa_eventemitter_instance*)zend_object_store_get_object(getThis() TSRMLS_CC);

	// check if we already have a hashtable stored
	HashTable *listeners;
	if (zend_hash_find(instance->events, name, name_len, (void**)&listeners) == FAILURE) {
		// what the hell are you trying to do? removing a callback that doesn't exists
		RETURN_FALSE;
	}

	// call any callback set
	if (instance->del_cb != NULL) {
		instance->del_cb(instance, name, name_len, &callback);
	}

	// try to find that callback and remove it
	zend_hash_apply_with_argument(listeners, kawa_eventemitter_apply_off, &callback TSRMLS_CC);
}
/* }}} */

/* {{{ proto public 
 * */
PHP_METHOD(EventEmitter, getListeners)
{
	char						*name = NULL;
	int							name_len = 0;
	kawa_eventemitter_instance  *instance;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &name, &name_len) == FAILURE) {
		RETURN_FALSE;
	}

	instance = (kawa_eventemitter_instance*)zend_object_store_get_object(getThis() TSRMLS_CC);

	// find that listener list
	HashTable *listeners;
	if (zend_hash_find(instance->events, name, name_len, (void**)&listeners) == FAILURE) {
		// what the hell are you trying to do? removing a callback that doesn't exists
		RETURN_NULL();
	}

	// return listeners
	array_init(return_value);

	KAWA_FOREACH(listeners) {
		php_callback *callback;
		KAWA_FOREACH_ENTRY(listeners, callback);

		// create closure
		zval *closure;
		MAKE_STD_ZVAL(closure);
		zend_create_closure(closure, callback->fci_cache.function_handler, callback->fci_cache.calling_scope, getThis() TSRMLS_CC);

		// store it
		add_next_index_zval(return_value, closure);
	}
}
/* }}} */
