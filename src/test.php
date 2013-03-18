<?php

$pool = \Kawa\Pool::getDefault();
$tcp = new \Kawa\Network\TCP($pool);

if ($tcp->listen(7000, "127.0.0.1")) {
	$tcp->on('connection', function() {
		var_dump(func_get_args());
	});
	$pool->run();
}