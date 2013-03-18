#include "php_kawa.h"

/** The class entry */
zend_class_entry *kawa_network_socket_class_entry;

ZEND_BEGIN_ARG_INFO(arginfo_kawa_network_socket_construct, 1)
	ZEND_ARG_OBJ_INFO(1, pool, Kawa\\Pool, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_kawa_network_socket_listen, 1)
	ZEND_ARG_INFO(0, address_or_port)
	ZEND_ARG_INFO(0, port)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_kawa_network_socket_get_pool, 0)
ZEND_END_ARG_INFO()

/** Our list of methods */
zend_function_entry kawa_network_socket_functions[] = {
	PHP_ME(Socket, __construct, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
	PHP_ME(Socket, connect, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Socket, read, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Socket, write, NULL, ZEND_ACC_PUBLIC)
	PHP_FE_END
};

static zend_object_handlers kawa_network_socket_instance_handlers;

/** Called when an instance of Pool is destroyed */
static void kawa_network_socket_free(void *object TSRMLS_DC)
{
	kawa_network_socket_instance *instance = (kawa_network_socket_instance *)object;

	// destroy eventemitter stuff
	kawa_eventemitter_destroy((kawa_eventemitter_instance*)instance TSRMLS_CC);

	zend_object_std_dtor(&instance->zo TSRMLS_CC);
	efree(object);
}

/** This is our Pool constructor */
static zend_object_value kawa_network_socket_new_ex(zend_class_entry *class_type, uv_tcp_t *socket, kawa_network_socket_instance **ptr TSRMLS_DC)
{
	zval *tmp;
	zend_object_value retval;
	kawa_network_socket_instance *instance;

	// Allocate memory for it
	instance = (kawa_network_socket_instance*)emalloc(sizeof(kawa_network_socket_instance));
	memset(instance, 0, sizeof(kawa_network_socket_instance));

	if (ptr)
	{
		*ptr = instance;
	}

	// setup eventemitter
	kawa_eventemitter_setup((kawa_eventemitter_instance*)instance);

	// then init the class
	zend_object_std_init(&instance->zo, class_type TSRMLS_CC);
	object_properties_init(&instance->zo, class_type);

	retval.handle = zend_objects_store_put(instance, NULL, (zend_objects_free_object_storage_t) kawa_network_socket_free, NULL TSRMLS_CC);
	retval.handlers = (zend_object_handlers *) &kawa_network_socket_instance_handlers;

	return retval;
}

static zend_object_value kawa_network_socket_new(zend_class_entry *class_type TSRMLS_DC)
{
	return kawa_network_socket_new_ex(class_type, NULL, NULL TSRMLS_CC);
}

void kawa_network_socket_init()
{
	zend_class_entry kawa_network_socket_ce;

	memcpy(&kawa_network_socket_instance_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));

	INIT_NS_CLASS_ENTRY(kawa_network_socket_ce, "Kawa\\Network", "Socket", kawa_network_socket_functions);
	kawa_network_socket_class_entry = zend_register_internal_class_ex(&kawa_network_socket_ce, kawa_eventemitter_class_entry, NULL TSRMLS_CC);
	kawa_network_socket_class_entry->create_object = kawa_network_socket_new;
}

PHP_METHOD(Socket, __construct)
{

}

PHP_METHOD(Socket, connect)
{

}

PHP_METHOD(Socket, read)
{

}

PHP_METHOD(Socket, write)
{

}
