#include "config.h"
#include "config_source.h"
#include <stdio.h>
#include <string.h>

static const char *log_level_names[] = {
	"ERROR",
	"WARN",
	"INFO"
};

int config_init(app_config_t *cfg)
{
	memset(cfg, 0, sizeof(*cfg));
	return 0;
}

void config_destroy(app_config_t *cfg)
{
	if (cfg->address)
		free(cfg->address);
	if (cfg->config_file)
		free(cfg->config_file);
	memset(cfg, 0, sizeof(*cfg));
}

int config_load(app_config_t *cfg, int argc, char *argv[])
{
	config_init(cfg);

	/* processing high priority configuration source later to override those already-processed. */
	apply_defaults(cfg);
	/* read the config file option only */
	apply_cli_args_for_config_file(cfg, argc, argv);
	apply_file_config(cfg);
	apply_env_config(cfg);
	apply_cli_args(cfg, argc, argv);

	return 0;
}

void config_print(const app_config_t *cfg)
{
	printf("Configuration:\n");
	printf("  address:     %s\n", cfg->address);
	printf("  port:        %u\n", cfg->port);
	printf("  log_level:   %d (%s)\n", cfg->log_level,
	       cfg->log_level >= 1 && cfg->log_level <= 3 ?
	       log_level_names[cfg->log_level - 1] : "UNKNOWN");
	printf("  debug:       %s\n", cfg->debug ? "true" : "false");
	printf("  config:      %s\n", cfg->config_file ? cfg->config_file : "(none)");
}

int main(int argc, char *argv[])
{
	app_config_t cfg;

	printf("=== %s v%s ===\n\n", CONFIG_DEMO_NAME, CONFIG_DEMO_VERSION);

	printf("Loading configuration (priority: CLI > ENV > File > Defaults)...\n\n");

	config_load(&cfg, argc, argv);

	config_print(&cfg);

	printf("\nConfiguration sources demo:\n");
	printf("  1. CLI options override all\n");
	printf("  2. Environment variables override file/defaults\n");
	printf("  3. Config file: %s\n",
	       cfg.config_file ? cfg.config_file : "(not found)");
	printf("  4. Built-in defaults\n");

	config_destroy(&cfg);

	return 0;
}
