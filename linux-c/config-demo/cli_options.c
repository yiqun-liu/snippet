#include "cli_options.h"
#include <stdio.h>

struct option long_options[] = {
	{"address", required_argument, 0, 's'},
	{"port", required_argument, 0, 'p'},
	{"log-level", required_argument, 0, 'l'},
	{"debug", no_argument, 0, 'd'},
	{"config", required_argument, 0, 'c'},
	{"help", no_argument, 0, 'h'},
	{0, 0, 0, 0}
};

void print_usage(const char *prog_name)
{
	printf("Usage: %s [OPTIONS]\n", prog_name);
	printf("  -s, --address=ADDR       Server address\n");
	printf("  -p, --port=PORT          Server port\n");
	printf("  -l, --log-level=LEVEL    Log level 1-3\n");
	printf("  -d, --debug              Enable debug mode\n");
	printf("  -c, --config=FILE        Configuration file\n");
	printf("  -h, --help               Show help\n");
}
