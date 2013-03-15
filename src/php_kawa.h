#ifndef PHP_KAWA_H
#define PHP_KAWA_H

#include "uv.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"

typedef struct _kawa_pool_instance {
	zend_object				zo;
	uv_loop_t				*loop;
} kawa_pool_instance;

typedef struct _php_callback {
	zend_fcall_info			fci;
	zend_fcall_info_cache   fci_cache;
} php_callback;

extern zend_class_entry *kawa_pool_class_entry;

PHP_MINIT_FUNCTION(kawa);

PHP_METHOD(Pool, listen);
PHP_METHOD(Pool, run);
PHP_METHOD(Pool, stop);

extern zend_module_entry kawa_module_entry;

#endif//PHP_KAWA_H