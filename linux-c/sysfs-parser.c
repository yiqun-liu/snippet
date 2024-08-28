/*
 * A minimalist lib to parse some typical sysfs output format.
 */
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#define BUFFER_SIZE 64

int read_sysfs_str(char *str, size_t len, const char *path)
{
	printf("\n[%s]: got argument %s.\n", __func__, path);
	int fd = open(path, O_RDONLY);
	if (fd == -1) {
		int ret = errno;
		fprintf(stderr, "failed to open %s, errmsg='%s'.\n", path, strerror(errno));
		return ret;
	}

	ssize_t retval = read(fd, str, len);
	if (retval == -1) {
		int ret = errno;
		fprintf(stderr, "failed to read %s, errmsg='%s'.\n", path, strerror(errno));
		return ret;
	}

	close(fd);

	/* num bytes exclude \0, the last char is \n */
	printf("%zd bytes read from sysfs file %s, strlen()=%zu.\n", retval, path, strlen(str));
	printf("str='%s'\nlast char %s \\n\n", str, str[retval - 1] == '\n' ? "==" : "!=");

	return 0;
}

typedef uint64_t demo_mask_t;

int parse_array_to_bitmask(demo_mask_t *demo_mask, const char *str)
{
	const char *cursor = str;
	*demo_mask = 0;
	printf("\n[%s]: got input '%s'.\n", __func__, str);
	while (true) {
		unsigned long start_value, end_value;
		demo_mask_t bit;
		char *end;

		/* read the first value */
		start_value = strtoul(cursor, &end, 10);
		if (end == cursor) {
			fprintf(stderr, "parsing failed at offset %lu.\n", cursor - str);
			return -1;
		}
		cursor = end;
		end_value = start_value;

		/* read the second value if there is one */
		if (*cursor == '-') {
			cursor++;
			end_value = strtoul(cursor, &end, 10);
			if (end == cursor || end_value <= start_value) {
				fprintf(stderr, "parsing failed at offset %lu.\n", cursor - str);
				return -1;
			}
			cursor = end;
		}

		/* record the input range */
		bit = (demo_mask_t)1 << start_value;
		for (unsigned long i = start_value; i <= end_value; i++) {
			*demo_mask |= bit;
			bit <<= 1;
		}

		/* check next value */
		if (*cursor == ',') {
			cursor++;
		} else if (*cursor == '\n') {
			break;
		} else {
			fprintf(stderr, "parsing failed at offset %lu.\n", cursor - str);
			return -1;
		}
	}

	printf("parsing completed: bitmask=0x%lx.\n", *demo_mask);

	return 0;
}

int main(void)
{
	int ret;
	char str[BUFFER_SIZE];

	ret = read_sysfs_str(str, BUFFER_SIZE, "/sys/devices/system/cpu/online");
	if (ret)
		return ret;

	demo_mask_t mask;
	ret = parse_array_to_bitmask(&mask, str);
	if (ret)
		return ret;
	ret = parse_array_to_bitmask(&mask, "0-1,3,5-8,10\n");
	if (ret)
		return ret;

	return 0;
}
