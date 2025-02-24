#!/bin/bash

REMOTE_CODE_DIR=/root/code

if ! test -n $1
then
	# the VM referred to here must be properly setup in ~/.ssh/config
	echo "expect 1st argument: VM config"
	exit 1
fi
vm=$1

if ! test -n $2 || ! test -d $2
then
	echo "expect 2nd argument to be valid code directory"
	exit 1
fi
local_code=$(realpath $2)

rsync -rlpzPq --delete $local_code/ $vm:$REMOTE_CODE_DIR/$(basename ${local_code})

echo "done"
