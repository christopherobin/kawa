--TEST--
Basic async Pool
--FILE--
<?php
$pool = new \Kawa\Pool();
$pool->run();
--EXPECTF--


