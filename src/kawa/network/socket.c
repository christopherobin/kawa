#include "php_kawa.h"

#include "zend_interfaces.h"

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
	PHP_ME(Socket, write, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Socket, close, NULL, ZEND_ACC_PUBLIC)
	PHP_FE_END
};

zend_object_handlers kawa_network_socket_instance_handlers;

/** Called when an instance of Pool is destroyed */
void kawa_network_socket_free(void *object TSRMLS_DC)
{
	kawa_network_socket_instance *instance = (kawa_network_socket_instance *)object;

	// destroy eventemitter stuff
	kawa_eventemitter_destroy((kawa_eventemitter_instance*)instance TSRMLS_CC);

	// destroy socket
	if (instance->socket != NULL) {
		uv_close((uv_handle_t*)instance->socket, NULL);
	}

	zend_object_std_dtor(&instance->zo TSRMLS_CC);
	efree(object);
}

uv_buf_t kawa_network_socket_on_alloc(uv_handle_t *handle, size_t suggested_size)
{
	return uv_buf_init((char*)emalloc(suggested_size), suggested_size);
}

void kawa_network_socket_on_read(uv_stream_t *stream, ssize_t nread, uv_buf_t buf)
{
	if (nread == -1) {
		// do something :(
	} else {
		if (nread > 0) {
			zval *this_ptr = (zval*)stream->data;
			kawa_network_socket_instance *instance = (kawa_network_socket_instance*)zend_object_store_get_object(this_ptr TSRMLS_CC);
			if (instance == NULL) return; // wat ?

			// emit signal
			// first alloc the signal name
			zval *retval;
			zval *signal_name;
			MAKE_STD_ZVAL(signal_name);
			ZVAL_STRING(signal_name, "data", 1);
			// then the data buffer
			zval *buffer;
			MAKE_STD_ZVAL(buffer);
			ZVAL_STRINGL(buffer, buf.base, nread, 1);
			// then emit that signal
			zend_call_method_with_2_params(&this_ptr, Z_OBJCE_P(this_ptr), NULL, "emit", &retval, signal_name, buffer);
			zval_ptr_dtor(&signal_name);
			zval_ptr_dtor(&buffer);
			zval_ptr_dtor(&retval);
		}
	}
	if (buf.base)
		efree(buf.base);
}

void kawa_network_socket_listener_add(void *pInstance, char *name, int name_len, php_callback *cb)
{
	kawa_network_socket_instance *instance = (kawa_network_socket_instance*)pInstance;

	if (strncmp(name, "data", 4) == 0) {
		if (instance->reading == 1) return;
		if (instance->socket == NULL) return; // something is clearly wrong
		// start listening on read data
		uv_read_start((uv_stream_t*)instance->socket, kawa_network_socket_on_alloc, kawa_network_socket_on_read);
		instance->reading = 1;
	}
}

/** This is our Pool constructor */
zend_object_value kawa_network_socket_new_ex(zend_class_entry *class_type, uv_tcp_t *socket, kawa_network_socket_instance **ptr TSRMLS_DC)
{
	zval *tmp;
	zend_object_value retval;
	kawa_network_socket_instance *instance;

	// Allocate memory for it
	instance = (kawa_network_socket_instance*)emalloc(sizeof(kawa_network_socket_instance));
	memset(instance, 0, sizeof(kawa_network_socket_instance));

	if (ptr) {
		*ptr = instance;
	}

	// setup eventemitter
	kawa_eventemitter_setup((kawa_eventemitter_instance*)instance);
	// add a callback on add that setup the right listeners
	instance->event.add_cb = kawa_network_socket_listener_add;

	// store the socket if it exists, create it otherwise
	instance->socket = socket;
	instance->pool = NULL;

	// then init the class
	zend_object_std_init(&instance->zo, class_type TSRMLS_CC);
	object_properties_init(&instance->zo, class_type);

	retval.handle = zend_objects_store_put(instance, NULL, (zend_objects_free_object_storage_t) kawa_network_socket_free, NULL TSRMLS_CC);
	retval.handlers = (zend_object_handlers *) &kawa_network_socket_instance_handlers;

	return retval;
}

zend_object_value kawa_network_socket_new(zend_class_entry *class_type TSRMLS_DC)
{
	return kawa_network_socket_new_ex(class_type, NULL, NULL TSRMLS_CC);
}

void kawa_network_socket_on_close(uv_handle_t *handle)
{
	// remove reference to socket when closing
	zval *socket = (zval*)handle->data;
	zval_ptr_dtor(&socket);
}

void kawa_network_socket_setup(zval *socket)
{
	kawa_network_socket_instance *instance = (kawa_network_socket_instance*)zend_object_store_get_object(socket TSRMLS_CC);

	instance->socket->data = socket;
	instance->reading = 0;
	instance->socket->close_cb = kawa_network_socket_on_close;
	// add a ref to ourself
	Z_ADDREF_P(socket);
}

void kawa_network_socket_init()
{
	zend_class_entry kawa_network_socket_ce;

	memcpy(&kawa_network_socket_instance_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));

	INIT_NS_CLASS_ENTRY(kawa_network_socket_ce, "Kawa\\Network", "Socket", kawa_network_socket_functions);
	kawa_network_socket_class_entry = zend_register_internal_class_ex(&kawa_network_socket_ce, kawa_eventemitter_class_entry, NULL TSRMLS_CC);
	kawa_network_socket_class_entry->create_object = kawa_network_socket_new;
	kawa_network_socket_class_entry->ce_flags |= ZEND_ACC_FINAL_CLASS;
}

PHP_METHOD(Socket, __construct)
{
	kawa_network_socket_instance *instance;
	kawa_pool_instance *pool_instance;
	zval *pool;

	// parse argument
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "O", &pool, kawa_pool_class_entry) == FAILURE) {
		return;
	}

	instance = (kawa_network_socket_instance*)zend_object_store_get_object(getThis() TSRMLS_CC);
	pool_instance = (kawa_pool_instance*)zend_object_store_get_object(pool TSRMLS_CC);
	// store the pool in our object
	instance->pool = pool;
	// and increment pool refcount
	Z_ADDREF_P(pool);

	if (instance->socket == NULL) {
		//setup tcp client if necessary
		uv_tcp_init(pool_instance->loop, instance->socket);
		// store our instance in the server
	}

	kawa_network_socket_setup(getThis());
}

PHP_METHOD(Socket, connect)
{

}

PHP_METHOD(Socket, write)
{

}

PHP_METHOD(Socket, close)
{
	kawa_network_socket_instance *instance;
	instance = (kawa_network_socket_instance*)zend_object_store_get_object(getThis() TSRMLS_CC);
	uv_close((uv_handle_t*)instance->socket, NULL);
	instance->socket = NULL;
}