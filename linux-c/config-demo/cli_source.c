#include "config.h"
#include "config_source.h"
#include "cli_options.h"
#include <getopt.h>
#include <stdlib.h>
#include <string.h>

int apply_cli_args_for_config_file(app_config_t *cfg, int argc, char *argv[])
{
	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-c") == 0 && i + 1 < argc) {
			config_set_string(&cfg->config_file, argv[i + 1]);
		} else if (strncmp(argv[i], "--config=", 10) == 0) {
			config_set_string(&cfg->config_file, argv[i] + 10);
		}
	}
	return 0;
}

int apply_cli_args(app_config_t *cfg, int argc, char *argv[])
{
	int opt;
	int option_index = 0;

	while ((opt = getopt_long(argc, argv, "s:p:l:dch",
				  long_options, &option_index)) != -1) {
		switch (opt) {
		case 's':
			config_set_string(&cfg->address, optarg);
			break;
		case 'p':
			cfg->port = (uint16_t)atoi(optarg);
			break;
		case 'l':
			cfg->log_level = atoi(optarg);
			break;
		case 'd':
			cfg->debug = true;
			break;
		case 'c':
			config_set_string(&cfg->config_file, optarg);
			break;
		case 'h':
			print_usage(argv[0]);
			exit(0);
		default:
			break;
		}
	}

	return 0;
}
