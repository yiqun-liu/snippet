#!/usr/bin/bash

RED='\033[0;31m'
GREEN='\033[0;32m'
COLOR_OFF='\033[0m'
pr_green()	{ echo -e "$GREEN$*$COLOR_OFF"; }
pr_red()	{ echo -e "$RED$*$COLOR_OFF"; }

log_time_str()	{ date +%Y-%m-%d\|%H:%M:%S; }
log()		{ echo "[$(log_time_str)]$*"; }
log_err()	{ echo "[$(log_time_str)] ERROR $*" >&2; }
log_fatal()	{ echo -e "[$(log_time_str)]$RED FATAL $*$COLOR_OFF" >&2; exit -1; }

run_demo()	{
	pr_green "$0: green demo"
	pr_red "$0: red demo"

	log "$0: log demo"
	log_err "$0: error demo"

	log_fatal "$0: dying message demo"
}

run_demo
