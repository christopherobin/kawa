<?php

$pool = \Kawa\Pool::getDefault();
$tcp = new \Kawa\Network\TCP($pool);

if ($tcp->listen("127.0.0.1", 7000)) {
	echo "Adding listener on event 'connection'\n";
	$foo = $tcp->on('connection', function() { var_dump(func_get_args()); });
	var_dump($foo);
	//$tcp->emit('connection', 42);
}

//$tcp->getPool()->run();