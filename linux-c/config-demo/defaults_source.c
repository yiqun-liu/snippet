#include "config.h"
#include "config_source.h"
#include <string.h>

int apply_defaults(app_config_t *cfg)
{
	if (cfg->address == NULL)
		config_set_string(&cfg->address, "localhost");

	if (cfg->port == 0)
		cfg->port = 8080;

	if (cfg->log_level == 0)
		cfg->log_level = 3;

	return 0;
}
