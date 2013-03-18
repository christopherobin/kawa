<?php

$pool = \Kawa\Pool::getDefault();
$tcp = new \Kawa\Network\TCP($pool);

if ($tcp->listen("127.0.0.1", 7000)) {
	echo "Adding listener on event 'connection'\n";
	$cb = function() { var_dump(func_get_args()); };
	$tcp->on('connection', function() { var_dump(func_get_args()); });
	$tcp->on('connection', $cb);
	$tcp->on('connection', $cb);
	$tcp->once('connection', function() { var_dump(func_get_args()); });
	//var_dump($foo);
	$tcp->emit('connection', "foo", "bar", 42);
	$tcp->off('connection', $cb);
	$tcp->emit('connection', "foo2", "bar2", 84);

	var_dump($tcp->getListeners('connection'));
}

//$tcp->getPool()->run();