#include "config.h"
#include "config_source.h"
#include <stdlib.h>
#include <string.h>

/*
 * Thread-safety: getenv() returns a pointer to internal static storage.
 * The function itself is not thread-safe.
 */
int apply_env_config(app_config_t *cfg)
{
	const char *env_value;

	if ((env_value = getenv("CONFIG_DEMO_ADDRESS")) != NULL) {
		config_set_string(&cfg->address, env_value);
	}

	if ((env_value = getenv("CONFIG_DEMO_PORT")) != NULL) {
		cfg->port = (uint16_t)atoi(env_value);
	}

	if ((env_value = getenv("CONFIG_DEMO_LOG_LEVEL")) != NULL) {
		cfg->log_level = atoi(env_value);
	}

	if ((env_value = getenv("CONFIG_DEMO_DEBUG")) != NULL) {
		cfg->debug = (strcmp(env_value, "1") == 0 ||
			     strcasecmp(env_value, "true") == 0);
	}

	if ((env_value = getenv("CONFIG_DEMO_CONFIG")) != NULL) {
		config_set_string(&cfg->config_file, env_value);
	}

	return 0;
}
