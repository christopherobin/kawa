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
	PHP_FE_END
};

static zend_object_handlers kawa_network_tcp_instance_handlers;

/** Called when an instance of Pool is destroyed */
static void kawa_network_tcp_free(void *object TSRMLS_DC)
{
	kawa_network_tcp_instance *instance = (kawa_network_tcp_instance *)object;

	// destroy eventemitter stuff
	kawa_eventemitter_destroy((kawa_eventemitter_instance*)instance TSRMLS_CC);

	// do we have linked pool?
	if (instance->pool != NULL) {
		// free it
		zval_ptr_dtor(&instance->pool);
	}

	zend_object_std_dtor(&instance->zo TSRMLS_CC);
	efree(object);
}

/** This is our Pool constructor */
static zend_object_value kawa_network_tcp_new_ex(zend_class_entry *class_type, kawa_network_tcp_instance **ptr TSRMLS_DC)
{
	zval *tmp;
	zend_object_value retval;
	kawa_network_tcp_instance *instance;

	// Allocate memory for it
	instance = (kawa_network_tcp_instance*)emalloc(sizeof(kawa_network_tcp_instance));
	memset(instance, 0, sizeof(kawa_network_tcp_instance));

	if (ptr)
	{
		*ptr = instance;
	}

	// set that thing to null
	instance->pool = NULL;

	// setup eventemitter
	kawa_eventemitter_setup((kawa_eventemitter_instance*)instance);

	// then init the class
	zend_object_std_init(&instance->zo, class_type TSRMLS_CC);
	object_properties_init(&instance->zo, class_type);

	retval.handle = zend_objects_store_put(instance, NULL, (zend_objects_free_object_storage_t) kawa_network_tcp_free, NULL TSRMLS_CC);
	retval.handlers = (zend_object_handlers *) &kawa_network_tcp_instance_handlers;

	return retval;
}

static zend_object_value kawa_network_tcp_new(zend_class_entry *class_type TSRMLS_DC)
{
	return kawa_network_tcp_new_ex(class_type, NULL TSRMLS_CC);
}

void kawa_network_tcp_on_new_connection(uv_stream_t *server, int status)
{
	kawa_network_tcp_instance *instance;
	kawa_pool_instance *pool_instance;

	php_printf("debug: connection incoming %p (status: %d)\n", server, status);
	if (status == -1) {
		// oups something is wrong :sadface:
		return;
	}

	zval *this_ptr = (zval*)server->data;

	instance = (kawa_network_tcp_instance*)zend_object_store_get_object(this_ptr TSRMLS_CC);
	pool_instance = (kawa_pool_instance*)zend_object_store_get_object(instance->pool TSRMLS_CC);

	// accept the connection
	uv_tcp_t *client = (uv_tcp_t*)malloc(sizeof(uv_tcp_t));
	uv_tcp_init(pool_instance->loop, client);

	if (uv_accept(server, (uv_stream_t*)client) == 0) {
		// emit connection signal
		zval *retval;
		// first alloc the signal name
		zval *signal_name;
		MAKE_STD_ZVAL(signal_name);
		ZVAL_STRING(signal_name, "connection", 1);
		// then instanciate a \Kawa\Network\Socket with our socket
		zend_call_method_with_1_params(&this_ptr, Z_OBJCE_P(this_ptr), NULL, "emit", &retval, signal_name);
		zval_ptr_dtor(&signal_name);
		zval_ptr_dtor(&retval);

		uv_close((uv_handle_t*) client, NULL);
	} else {
		uv_close((uv_handle_t*) client, NULL);
	}
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
	kawa_network_tcp_instance *instance;
	kawa_pool_instance *pool_instance;
	zval *pool;

	// parse argument
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "O", &pool, kawa_pool_class_entry) == FAILURE) {
		return;
	}

	instance = (kawa_network_tcp_instance*)zend_object_store_get_object(getThis() TSRMLS_CC);
	pool_instance = (kawa_pool_instance*)zend_object_store_get_object(pool TSRMLS_CC);
	// store the pool in our object
	instance->pool = pool;
	// and increment pool refcount
	Z_ADDREF_P(pool);

	//setup tcp server
	uv_tcp_init(pool_instance->loop, &instance->server);
	// store our instance in the server
	instance->server.data = getThis();
}

PHP_METHOD(TCP, listen)
{
	//char *default_bind_address = KAWA_DEFAULT_BIND_ADDRESS;
	char *bind_address = KAWA_DEFAULT_BIND_ADDRESS;
	int bind_address_len = strlen(KAWA_DEFAULT_BIND_ADDRESS);
	long bind_port;
	kawa_network_tcp_instance *instance;
	kawa_pool_instance *pool_instance;

	// try first to parse string 
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l|s", &bind_port, &bind_address, &bind_address_len) == FAILURE) {
		RETURN_FALSE;
	}

	instance = (kawa_network_tcp_instance*)zend_object_store_get_object(getThis() TSRMLS_CC);

	// create socket bind data
	char *c_bind_address = estrndup(bind_address, bind_address_len);
	struct sockaddr_in bind_addr = uv_ip4_addr(c_bind_address, bind_port);
	// we don't need the bind adress anymore
	efree(c_bind_address);

	// try to bind and listen
	uv_tcp_bind(&instance->server, bind_addr);
	if (uv_listen((uv_stream_t*)&instance->server, 128, kawa_network_tcp_on_new_connection)) {
		// retrieve our loop
		pool_instance = (kawa_pool_instance*)zend_object_store_get_object(instance->pool TSRMLS_CC);
		zend_throw_exception(zend_exception_get_default(TSRMLS_C), uv_err_name(uv_last_error(pool_instance->loop)), 0 TSRMLS_CC);
	}

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