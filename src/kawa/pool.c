#include "php_kawa.h"

ZEND_DECLARE_MODULE_GLOBALS(kawa);

/** The class entry */
zend_class_entry *kawa_pool_class_entry;

ZEND_BEGIN_ARG_INFO(arginfo_kawa_pool_void, 0)
ZEND_END_ARG_INFO()

/** Our list of methods */
zend_function_entry kawa_pool_functions[] = {
	PHP_ME(Pool, run, arginfo_kawa_pool_void, ZEND_ACC_PUBLIC)
	PHP_ME(Pool, once, arginfo_kawa_pool_void, ZEND_ACC_PUBLIC)
	PHP_ME(Pool, stop, arginfo_kawa_pool_void, ZEND_ACC_PUBLIC)
	PHP_ME(Pool, getDefault, arginfo_kawa_pool_void, ZEND_ACC_STATIC|ZEND_ACC_PUBLIC)
	PHP_FE_END
};

static zend_object_handlers kawa_pool_instance_handlers;

void kawa_pool_on_signal(uv_signal_t *handle, int signum)
{
	// kill the loop if we get a SIGINT
	if (signum == SIGINT) {
		uv_stop(handle->loop);
	}
}

/** Called when an instance of Pool is destroyed */
static void kawa_pool_free(void *object TSRMLS_DC)
{
	kawa_pool_instance *intern = (kawa_pool_instance *)object;

	// don't delete the dedault loop!
	if (intern->loop != uv_default_loop()) {
		uv_loop_delete(intern->loop);
	}

	zend_object_std_dtor(&intern->zo TSRMLS_CC);
	efree(object);
}

/** This is our Pool constructor */
static zend_object_value kawa_pool_new_ex(zend_class_entry *class_type, uv_loop_t *loop, kawa_pool_instance **ptr TSRMLS_DC)
{
	zval *tmp;
	zend_object_value retval;
	kawa_pool_instance *intern;

	// Allocate memory for it
	intern = (kawa_pool_instance*)emalloc(sizeof(kawa_pool_instance));
	memset(intern, 0, sizeof(kawa_pool_instance));

	if (ptr)
	{
		*ptr = intern;
	}

	// use the loop provided or create a uv loop for this object
	intern->loop = loop;
	if (intern->loop == NULL) {
		intern->loop = uv_loop_new();
	}

	// listen for sigints
	uv_signal_init(intern->loop, &intern->sigint);
	uv_signal_start(&intern->sigint, kawa_pool_on_signal, SIGINT);
	uv_unref((uv_handle_t*) &intern->sigint);

	// then init the class
	zend_object_std_init(&intern->zo, class_type TSRMLS_CC);

	retval.handle = zend_objects_store_put(intern, NULL, (zend_objects_free_object_storage_t) kawa_pool_free, NULL TSRMLS_CC);
	retval.handlers = (zend_object_handlers *) &kawa_pool_instance_handlers;

	return retval;
}

static zend_object_value kawa_pool_new(zend_class_entry *class_type TSRMLS_DC)
{
	return kawa_pool_new_ex(class_type, NULL, NULL TSRMLS_CC);
}

/* {{{ int kawa_pool_compare(zval *object1, zval *object2 TSRMLS_DC)
 * Because we store some extra pointers in our objects, we must augment the default comparator */
int kawa_pool_compare(zval *object1, zval *object2 TSRMLS_DC)
{
	// first run the standard check
	int different = zend_get_std_object_handlers()->compare_objects(object1, object2);
	if (different) return different;

	// then check that pools are the same
	kawa_pool_instance *intern1;
	intern1 = (kawa_pool_instance*)zend_object_store_get_object(object1 TSRMLS_CC);

	kawa_pool_instance *intern2;
	intern2 = (kawa_pool_instance*)zend_object_store_get_object(object2 TSRMLS_CC);

	return (intern1->loop == intern2->loop ? 0 : 1);
}

/* {{{ void kawa_pool_init()
 * Register the \Kawa\Pool into PHP */
void kawa_pool_init()
{
	zend_class_entry kawa_pool_ce;

	// setup default handlers
	memcpy(&kawa_pool_instance_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
	kawa_pool_instance_handlers.compare_objects = kawa_pool_compare;

	INIT_NS_CLASS_ENTRY(kawa_pool_ce, "Kawa", "Pool", kawa_pool_functions);
	kawa_pool_ce.create_object = kawa_pool_new;
	kawa_pool_class_entry = zend_register_internal_class(&kawa_pool_ce TSRMLS_CC);
}
/* }}} */

/* {{{ void kawa_pool_init()
 * Register the \Kawa\Pool into PHP */
void kawa_pool_shutdown()
{
	if (KAWA_G(default_loop) != NULL) {
		Z_DELREF_P(KAWA_G(default_loop));
		FREE_ZVAL(KAWA_G(default_loop));
	}
}
/* }}} */

/* {{{ proto public int \Kawa\Pool::run()
 * This function runs the event loop until the reference count drops to
 * zero. Always returns zero. */
PHP_METHOD(Pool, run)
{
	kawa_pool_instance *intern;
	intern = (kawa_pool_instance*)zend_object_store_get_object(getThis() TSRMLS_CC);
	RETURN_LONG(uv_run(intern->loop, UV_RUN_DEFAULT));
}
/* }}} */

/* {{{ proto public int \Kawa\Pool::once()
 * This function runs the event loop once. Note that this function blocks if
 * there are no pending events. Returns zero when done (no active handles
 * or requests left), or non-zero if more events are expected (meaning you
 * should run the event loop again sometime in the future). */
PHP_METHOD(Pool, once)
{
	kawa_pool_instance *intern;
	intern = (kawa_pool_instance*)zend_object_store_get_object(getThis() TSRMLS_CC);
	RETURN_LONG(uv_run(intern->loop, UV_RUN_ONCE));
}
/* }}} */

/* {{{ proto public int \Kawa\Pool::run()
 * This function will stop the event loop by forcing uv_run to end
 * as soon as possible, but not sooner than the next loop iteration. */
PHP_METHOD(Pool, stop)
{
	kawa_pool_instance *intern;
	intern = (kawa_pool_instance*)zend_object_store_get_object(getThis() TSRMLS_CC);
	uv_stop(intern->loop);
	RETURN_NULL();
}
/* }}} */

/* {{{ proto static public int \Kawa\Pool::default()
 * Returns the default loop. */
PHP_METHOD(Pool, getDefault)
{
	// instanciate the class if it doesn't exists
	if (KAWA_G(default_loop) == NULL) {
		zval *tmp;

		ALLOC_ZVAL(tmp);
		Z_OBJVAL_P(tmp) = kawa_pool_new_ex(kawa_pool_class_entry, uv_default_loop(), NULL TSRMLS_CC);
		Z_SET_REFCOUNT_P(tmp, 1);
		Z_SET_ISREF_P(tmp);
		Z_TYPE_P(tmp) = IS_OBJECT;
		KAWA_G(default_loop) = tmp;
	}

	RETVAL_ZVAL(KAWA_G(default_loop), 1, 0);
}
/* }}} */

/*
$pool = new \Kawa\Pool();
$tcp = new \Kawa\Network\TCP\Server($pool);
if ($tcp->listen("0.0.0.0", 7000)) {
	$tcp->on('connection', function(\Kawa\Network\TCP\Client $client) {
		$client->on('data', function($buffer) use ($client) {
			$client->write($buffer);
		});
		$client->write("HELO\r\n");
	});
}
*/