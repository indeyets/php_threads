<?php

if (!extension_loaded('threads'))
    dl('threads.so');

$array = array(
0 => 'thread',
1 => 'hello',
2 => 'world');

thread_set('mySharedVar', $array);

thread_include('thread.php');

sleep(3);

echo 'Done';
