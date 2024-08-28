/*
 * The corner cases which I would like to know about stdio.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>

static int sprintf_demo(void)
{
	static char buffer[64];
	static const char *strings[] = {
		"word",
		"|two words with newline\n",
		"",
		"|last words"
	};

	int num_strings = sizeof(strings) / sizeof(strings[0]);
	char *cursor = buffer;
	for (int i = 0; i < num_strings; i++) {
		int ret;
		size_t string_len = strlen(strings[i]);
		ret = sprintf(cursor, "%s", strings[i]);
		printf("round=%d: ret=%d, strlen()=%zu\n", i + 1, ret, string_len);
		if (ret < 0) {
			fprintf(stderr, "sprintf failed.\n");
			return ret;
		}

		cursor += ret;
	}
	printf("buffer='%s'.\n", buffer);

	return 0;
}

static int getline_demo(void)
{
	int round = 0;
	static char buffer[] = "This is the first line.\n" \
		"This is the second line.\r\n" \
		"This is the much much much much much much much much much much much much much much "
		"much much much much much much longer third line.\n" \
		"\n\n" \
	;

	FILE *fp = fmemopen(buffer, sizeof(buffer), "r");
	if (fp == NULL) {
		int ret = errno;
		perror("Failed to execute fmemopen()");
		return ret;
	}

	char *line = NULL;
	size_t len, line_len;
	ssize_t num_char;
	while (true) {
		printf("\nround: %d:\n", ++round);

		num_char = getline(&line, &len, fp);
		if (num_char == -1) {
			/* if EOF is encountered, errno is 0 */
			perror("Failed to run getline");
			break;
		}
		line_len = strlen(line);

		/*
		 * 1. Only when the line is empty, strlen != num_char
		 * 2. The function may re-allocate the buffer when it does not fit
		 * 3. The newline character is read into the line buffer
		 */
		printf("line_buf=%p, num_char=%zd, strlen=%zu, line_buf_size=%zu\n",
			line, num_char, line_len, len);
		if (num_char >= 0)
			printf("first char = %d, last char = %d.\n", line[0], line[line_len - 1]);
		printf("line='%s'\n", line);
	}
	free((void*)line);
	fclose(fp);

	return 0;
}

int main(void)
{
	int ret = 0;
	printf("\ngetline_demo:\n");
	ret |= getline_demo();
	printf("\nsprintf_demo:\n");
	ret |= sprintf_demo();
	return ret;
}
