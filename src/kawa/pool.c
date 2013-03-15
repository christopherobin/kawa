#include "php_kawa.h"

PHP_METHOD(Pool, run)
{
	kawa_pool_instance *intern;
	intern = (kawa_pool_instance*)zend_object_store_get_object(getThis() TSRMLS_CC);
	uv_run(intern->loop, UV_RUN_DEFAULT);
}

PHP_METHOD(Pool, once)
{
	kawa_pool_instance *intern;
	intern = (kawa_pool_instance*)zend_object_store_get_object(getThis() TSRMLS_CC);
	uv_run(intern->loop, UV_RUN_ONCE);
}

PHP_METHOD(Pool, stop)
{
	kawa_pool_instance *intern;
	intern = (kawa_pool_instance*)zend_object_store_get_object(getThis() TSRMLS_CC);
	uv_stop(intern->loop);
}

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