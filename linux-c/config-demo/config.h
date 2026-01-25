#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define CONFIG_DEMO_NAME		"config-demo"
#define CONFIG_DEMO_VERSION		"1.0.0"
#define CONFIG_DEFAULT_CONFIG_FILE	"/etc/config-demo.conf"
#define CONFIG_USER_CONFIG_FILE		".config/config-demo.conf"

typedef enum {
	CONFIG_SOURCE_CLI,
	CONFIG_SOURCE_ENV,
	CONFIG_SOURCE_FILE,
	CONFIG_SOURCE_DEFAULTS,
	CONFIG_SOURCE_UNKNOWN
} config_source_type_t;

typedef struct {
	char *address;
	uint16_t port;
	int log_level;
	bool debug;
	char *config_file;
} app_config_t;

static inline void config_set_string(char **field, const char *value)
{
	free(*field);
	*field = value ? strdup(value) : NULL;
}

int config_init(app_config_t *cfg);
void config_destroy(app_config_t *cfg);
int config_load(app_config_t *cfg, int argc, char *argv[]);
void config_print(const app_config_t *cfg);

#endif
