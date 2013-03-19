<?php

$pool = \Kawa\Pool::getDefault();
$tcp = new \Kawa\Network\TCP($pool);

if ($tcp->listen(7000, "127.0.0.1")) {
	$tcp->on('connection', function($socket) {
		$socket->on('data', function($data) use ($socket) {
			var_dump($data);
			$socket->close();
		});
	});
	$pool->run();
}