--TEST--
Basic async Pool
--FILE--
<?php
$pool = new \Kawa\Pool();
var_dump($pool);
var_dump(\Kawa\Pool::getDefault());
var_dump($pool == \Kawa\Pool::getDefault());
var_dump(\Kawa\Pool::getDefault() == \Kawa\Pool::getDefault());
var_dump($pool->run());
var_dump(\Kawa\Pool::getDefault()->run());
\Kawa\Pool::getDefault()->run();
--EXPECTF--
object(Kawa\Pool)#1 (0) {
}
object(Kawa\Pool)#2 (0) {
}
bool(false)
bool(true)
int(0)
int(0)
