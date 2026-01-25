#ifndef CONFIG_SOURCE_H
#define CONFIG_SOURCE_H

#include "config.h"

int apply_cli_args(app_config_t *cfg, int argc, char *argv[]);
int apply_cli_args_for_config_file(app_config_t *cfg, int argc, char *argv[]);
int apply_env_config(app_config_t *cfg);
int apply_file_config(app_config_t *cfg);
int apply_defaults(app_config_t *cfg);

#endif
