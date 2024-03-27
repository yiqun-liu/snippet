#!/usr/bin/bash
# a fork of stackoverflow question 6309918
# suggested demo: ./options.sh -l -p anything ./options.sh

VERSION="Shell Option Handling Demo v1.0"

print_usage() {
	cat << EOF
Usage: $0 [options] [file...]

Arguments:

  -h, --help
	show usage

  -v, --version
	show version

  -l	use a long listing format

  -p value
	print the value as is

  file...
    file to analyze.
EOF
}

if test $# -eq 0
then
	echo "No arguments: run the demo"

	echo ""
	echo "Run: $0 -v"
	$0 -v

	echo ""
	echo "Run: $0 $0"
	$0 $0

	echo ""
	echo "Run: $0 -l -p anything $0"
	$0 -l -p anything $0

	exit 0
fi

# default options
ls_options="-h"

while test $# -gt 0
do
	arg=$1
	case $arg in
		-h|--help) print_usage; exit 0 ;;
		-v|--version) echo $VERSION; exit 0 ;;
		-l) ls_options="$ls_options -l" ;;
		-p) shift; value=$1; echo $value;;
		*) break;;
	esac
	shift
done
files="$*"

ls $ls_options $files
