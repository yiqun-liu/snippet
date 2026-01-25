#include "config.h"
#include "config_source.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_LINE_LENGTH 512

static int find_config_file(app_config_t *cfg)
{
	if (cfg->config_file != NULL)
		return 0;

	if (access(CONFIG_DEFAULT_CONFIG_FILE, F_OK) == 0) {
		config_set_string(&cfg->config_file, CONFIG_DEFAULT_CONFIG_FILE);
		return 0;
	}

	const char *home = getenv("HOME");
	if (home != NULL) {
		char user_config[512];
		snprintf(user_config, sizeof(user_config), "%s/%s",
			 home, CONFIG_USER_CONFIG_FILE);
		if (access(user_config, F_OK) == 0) {
			config_set_string(&cfg->config_file, user_config);
			return 0;
		}
	}

	return -1;
}

int apply_file_config(app_config_t *cfg)
{
	if (find_config_file(cfg) != 0)
		return -1;

	if (cfg->config_file == NULL)
		return -1;

	FILE *fp = fopen(cfg->config_file, "r");
	if (fp == NULL)
		return -1;

	char line[MAX_LINE_LENGTH];
	char key[MAX_LINE_LENGTH];
	char value[MAX_LINE_LENGTH];

		while (fgets(line, sizeof(line), fp) != NULL) {
		char *trimmed = line;
		while (isspace((unsigned char)*trimmed))
			trimmed++;

		if (*trimmed == '#' || *trimmed == ';')
			continue;

		/* eq_pos: equal position -- pointer to the '=' delimiter separating key and value */
		char *eq_pos = strchr(trimmed, '=');
		if (eq_pos == NULL)
			continue;

		size_t key_len = eq_pos - trimmed;
		strncpy(key, trimmed, key_len);
		key[key_len] = '\0';

		/* Trim trailing whitespace from key */
		char *key_end = key + key_len - 1;
		while (key_end >= key && isspace((unsigned char)*key_end))
			*key_end-- = '\0';

		char *val_start = eq_pos + 1;
		while (isspace((unsigned char)*val_start))
			val_start++;

		strncpy(value, val_start, MAX_LINE_LENGTH - 1);
		value[MAX_LINE_LENGTH - 1] = '\0';

		char *value_end = value + strlen(value) - 1;
		while (value_end >= value && isspace((unsigned char)*value_end))
			*value_end-- = '\0';

		if (strcmp(key, "address") == 0) {
			config_set_string(&cfg->address, value);
		} else if (strcmp(key, "port") == 0) {
			cfg->port = (uint16_t)atoi(value);
		} else if (strcmp(key, "log_level") == 0) {
			cfg->log_level = atoi(value);
		} else if (strcmp(key, "debug") == 0) {
			cfg->debug = (strcmp(value, "1") == 0 ||
				     strcasecmp(value, "true") == 0);
		}
	}

	fclose(fp);
	return 0;
}
