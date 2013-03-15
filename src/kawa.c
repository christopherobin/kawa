#include "php_kawa.h"

zend_module_entry kawa_module_entry = {
	STANDARD_MODULE_HEADER,
	"kawa",
	NULL,
	PHP_MINIT(kawa),
	NULL,
	NULL,
	NULL,
	NULL,
	"0.1.0",
	STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_KAWA
ZEND_GET_MODULE(kawa)
#endif

zend_class_entry *kawa_pool_class_entry;

zend_function_entry kawa_pool_functions[] = {
	PHP_ME(Pool, run, NULL, ZEND_ACC_PUBLIC)
	{ NULL, NULL, NULL }
};

static zend_object_handlers kawa_instance_handlers;

static void kawa_pool_free(void *object TSRMLS_DC)
{
	kawa_pool_instance *intern = (kawa_pool_instance *)object;

	uv_loop_delete(intern->loop);

	zend_object_std_dtor(&intern->zo TSRMLS_CC);
	efree(object);
}

static zend_object_value kawa_pool_new_ex(zend_class_entry *class_type, kawa_pool_instance **ptr TSRMLS_DC)
{
	zval *tmp;
	zend_object_value retval;
	kawa_pool_instance *intern;

	/* Allocate memory for it */
	intern = (kawa_pool_instance*)emalloc(sizeof(kawa_pool_instance));
	memset(intern, 0, sizeof(kawa_pool_instance));

	if (ptr)
	{
		*ptr = intern;
	}

	// create a loop for this object
	intern->loop = uv_loop_new();

	zend_object_std_init(&intern->zo, class_type TSRMLS_CC);

	retval.handle = zend_objects_store_put(intern, NULL, (zend_objects_free_object_storage_t) kawa_pool_free, NULL TSRMLS_CC);
	retval.handlers = (zend_object_handlers *) &kawa_instance_handlers;

	return retval;
}

static zend_object_value kawa_pool_new(zend_class_entry *class_type TSRMLS_DC)
{
	return kawa_pool_new_ex(class_type, NULL TSRMLS_CC);
}

PHP_MINIT_FUNCTION(kawa)
{
	zend_class_entry kawa_pool_ce;

	INIT_NS_CLASS_ENTRY(kawa_pool_ce, "Kawa", "Pool", kawa_pool_functions);
	kawa_pool_ce.create_object = kawa_pool_new;
	kawa_pool_class_entry = zend_register_internal_class(&kawa_pool_ce TSRMLS_CC);

	return SUCCESS;
}
