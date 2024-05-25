#/usr/bin/bash

set -e
set -x

! test -f /dev/shm/sem.demo-semaphore && echo "sem file does not exist."

./build/named-semaphore create
./build/named-semaphore get

ls -l /dev/shm/sem.demo-semaphore && echo "sem file detected."

./build/named-semaphore post
./build/named-semaphore get

./build/named-semaphore wait
./build/named-semaphore get

./build/named-semaphore wait &
./build/named-semaphore get

./build/named-semaphore wait &
./build/named-semaphore get

./build/named-semaphore post
./build/named-semaphore get

./build/named-semaphore post
./build/named-semaphore get

wait

./build/named-semaphore unlink

! test -f /dev/shm/sem.demo-semaphore && echo "sem file disappeared."
