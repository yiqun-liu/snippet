#!/usr/bin/bash

BASE_DIR=$(dirname ${BASH_SOURCE[0]})

cd $BASE_DIR
make clean
make

lsmod | grep genl_demo && rmmod genl_demo

set -e
set -x

insmod genl_demo.ko

$BASE_DIR/build/listener 2 &
listener_pid=$!

$BASE_DIR/build/reporter 1
$BASE_DIR/build/reporter 7

wait $listener_pid

rmmod genl_demo
