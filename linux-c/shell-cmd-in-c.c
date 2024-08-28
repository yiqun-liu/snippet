/*
 * Run shell command in C
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

static void run_cmd_system(const char *cmd)
{
	/* a wrapper of execl */
	int ret = system(cmd);
	printf("%s: '%s' executed, ret=%d.\n", __func__, cmd, ret);
}

static void run_cmd_popen(const char *cmd)
{
	/* @cmd: command passed to /bin/sh using -c flag
	 * @type: "r" for read, "w" for write */
	char line[256];
	FILE *fp = popen(cmd, "r");
	if (fp == NULL) {
		printf("%s: failed to run '%s', errmsg='%s'.\n", __func__, cmd, strerror(errno));
		return;
	}
	printf("%s: '%s' executed.\n", __func__, cmd);

	while (fgets(line, sizeof(line), fp) != NULL)
		printf("%s", line);

	pclose(fp);
}

int main()
{
	int ret;

	/* use system(str) if you care about the return value */
	run_cmd_system("ps -l");
	run_cmd_system("! ps -l");

	/* use popen(str) if you care about the output, stderr will also be returned */
	run_cmd_popen("pwd");
	run_cmd_popen("ls -l /non-existent-path");

	return 0;
}
