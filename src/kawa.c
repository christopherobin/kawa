#include "php_kawa.h"

zend_module_entry kawa_module_entry = {
	STANDARD_MODULE_HEADER,
	"kawa",
	NULL,
	PHP_MINIT(kawa),
	PHP_MSHUTDOWN(kawa),
	NULL,
	PHP_RSHUTDOWN(kawa),
	PHP_MINFO(kawa),
	KAWA_MODULE_VERSION,
	STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_KAWA
ZEND_GET_MODULE(kawa)
#endif

ZEND_DECLARE_MODULE_GLOBALS(kawa);

PHP_MINIT_FUNCTION(kawa)
{
	// setup globals
	KAWA_G(default_loop) = NULL;

	// register class \Kawa\Pool
	kawa_pool_init();

	// register class \Kawa\EventEmitter
	kawa_eventemitter_init();

	// register class \Kawa\Network\TCP
	kawa_network_tcp_init();

	// register class \Kawa\Network\Socket
	kawa_network_socket_init();

	return SUCCESS;
}

PHP_RSHUTDOWN_FUNCTION(kawa)
{
	// take care of destructing any instance of Pool::default()
	kawa_pool_shutdown();
}

PHP_MSHUTDOWN_FUNCTION(kawa)
{

}

PHP_MINFO_FUNCTION(kawa)
{
	php_info_print_table_start();
	php_info_print_table_row(2, "kawa", "enabled");
	php_info_print_table_row(2, "version", KAWA_MODULE_VERSION);
	php_info_print_table_end();
}