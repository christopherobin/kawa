#include "php_kawa.h"

#include "zend_interfaces.h"

/** The class entry */
zend_class_entry *kawa_network_tcp_class_entry;

ZEND_BEGIN_ARG_INFO(arginfo_kawa_network_tcp_construct, 1)
	ZEND_ARG_OBJ_INFO(1, pool, Kawa\\Pool, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_kawa_network_tcp_listen, 1)
	ZEND_ARG_INFO(0, address_or_port)
	ZEND_ARG_INFO(0, port)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_kawa_network_tcp_get_pool, 0)
ZEND_END_ARG_INFO()

/** Our list of methods */
zend_function_entry kawa_network_tcp_functions[] = {
	PHP_ME(TCP, __construct, arginfo_kawa_network_tcp_construct, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
	PHP_ME(TCP, listen, arginfo_kawa_network_tcp_listen, ZEND_ACC_PUBLIC)
	PHP_ME(TCP, getPool, arginfo_kawa_network_tcp_get_pool, ZEND_ACC_PUBLIC)
	{ NULL, NULL, NULL }
};

static zend_object_handlers kawa_network_tcp_instance_handlers;

/** Called when an instance of Pool is destroyed */
static void kawa_network_tcp_free(void *object TSRMLS_DC)
{
	kawa_network_tcp_instance *intern = (kawa_network_tcp_instance *)object;

	// do we have linked pool?
	if (intern->pool != NULL) {
		// free it
		zval_ptr_dtor(&intern->pool);
	}

	zend_object_std_dtor(&intern->zo TSRMLS_CC);
	efree(object);
}

/** This is our Pool constructor */
static zend_object_value kawa_network_tcp_new_ex(zend_class_entry *class_type, kawa_network_tcp_instance **ptr TSRMLS_DC)
{
	zval *tmp;
	zend_object_value retval;
	kawa_network_tcp_instance *intern;

	// Allocate memory for it
	intern = (kawa_network_tcp_instance*)emalloc(sizeof(kawa_network_tcp_instance));
	memset(intern, 0, sizeof(kawa_network_tcp_instance));

	if (ptr)
	{
		*ptr = intern;
	}

	// set that thing to null
	intern->pool = NULL;

	// then init the class
	zend_object_std_init(&intern->zo, class_type TSRMLS_CC);
	object_properties_init(&intern->zo, class_type);

	retval.handle = zend_objects_store_put(intern, NULL, (zend_objects_free_object_storage_t) kawa_network_tcp_free, NULL TSRMLS_CC);
	retval.handlers = (zend_object_handlers *) &kawa_network_tcp_instance_handlers;

	return retval;
}

static zend_object_value kawa_network_tcp_new(zend_class_entry *class_type TSRMLS_DC)
{
	return kawa_network_tcp_new_ex(class_type, NULL TSRMLS_CC);
}

void kawa_network_tcp_init()
{
	zend_class_entry kawa_network_tcp_ce;

	memcpy(&kawa_network_tcp_instance_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));

	INIT_NS_CLASS_ENTRY(kawa_network_tcp_ce, "Kawa\\Network", "TCP", kawa_network_tcp_functions);
	kawa_network_tcp_class_entry = zend_register_internal_class_ex(&kawa_network_tcp_ce, kawa_eventemitter_class_entry, NULL TSRMLS_CC);
	kawa_network_tcp_class_entry->create_object = kawa_network_tcp_new;
}

PHP_METHOD(TCP, __construct)
{
	kawa_network_tcp_instance *intern;
	zval *pool;

	// parse argument
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "O", &pool, kawa_pool_class_entry) == FAILURE) {
		return;
	}

	intern = (kawa_network_tcp_instance*)zend_object_store_get_object(getThis() TSRMLS_CC);
	// store the pool in our object
	intern->pool = pool;
	// and increment pool refcount
	Z_ADDREF_P(pool);
}

PHP_METHOD(TCP, listen)
{
	kawa_network_tcp_instance *intern;
	intern = (kawa_network_tcp_instance*)zend_object_store_get_object(getThis() TSRMLS_CC);
	// do nothing
	RETURN_TRUE;
}

PHP_METHOD(TCP, getPool)
{
	kawa_network_tcp_instance *intern;
	intern = (kawa_network_tcp_instance*)zend_object_store_get_object(getThis() TSRMLS_CC);
	// return instance of pool
	RETURN_ZVAL(intern->pool, 1, 0)
}

/*
$pool = new \Kawa\Pool();
$tcp = new \Kawa\Network\TCP($pool);
if ($tcp->listen("0.0.0.0", 7000)) {
	$tcp->on('connection', function(\Kawa\Network\Socket $client) {
		$client->on('data', function($buffer) use ($client) {
			$client->write($buffer);
		});
		$client->write("HELO\r\n");
	});
}
*/