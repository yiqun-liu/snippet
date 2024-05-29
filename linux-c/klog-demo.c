/*
 * A demo of klogctl. Manipulate dmesg in a C program.
 *
 * Reference: man klogctl
 */
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <unistd.h>
#include <sys/klog.h>

/* not exported by the library, but is provided by the manual */
#define SYSLOG_ACTION_READ_ALL		3
#define SYSLOG_ACTION_READ_CLEAR	4
#define SYSLOG_ACTION_CLEAR		5
#define SYSLOG_ACTION_UNREAD		9

#define BUF_SIZE 512

int write_kmesg(const char *str);

int klog_clear(void);
int klog_read_new(void);

int klog_unread_demo(void);
int klog_read_new_demo(void);
int klog_read_clear_demo(void);
int klog_read_all_repeated(void);

int main()
{
	return klog_unread_demo() || klog_read_new_demo() ||
		klog_read_clear_demo() || klog_read_all_repeated();
}

int write_kmesg(const char *str)
{
	FILE *fp = fopen("/dev/kmsg", "a");
	if (fp == NULL) {
		fprintf(stderr, "failed to open /dev/kmsg.\n");
		return -1;
	}
	fprintf(fp, "%s\n", str);
	fclose(fp);
	return 0;
}

int klog_clear(void)
{
	int ret = klogctl(SYSLOG_ACTION_CLEAR, NULL, 0);
	if (ret == -1)
		fprintf(stderr, "failed to run klog.clear(). %s.\n", strerror(errno));
	return ret;
}

int klog_read_new(void)
{
	static char buf1[BUF_SIZE] = {'\0'}, buf2[BUF_SIZE] = {'\0'};
	static char *next_buf = buf1, *last_line = buf2;
	static int last_line_length = 0, round = 0;

	int ret = klogctl(SYSLOG_ACTION_READ_ALL, next_buf, BUF_SIZE);
	if (ret == -1) {
		fprintf(stderr, "failed to run klog.read_all(). %s.\n", strerror(errno));
		return -1;
	}

	int next_length = ret;
	/* identify the boundary line in the newly read buffer */
	/* another way would be write your own separator into dmesg */
	char *start_line = last_line_length == 0 ? next_buf : NULL;
	for (int i = 0; i < next_length && start_line == NULL; i++) {
		/* match the substring against last line */
		_Bool match = true;
		for (int j = 0; j < last_line_length && match; j++)
			match = next_buf[i + j] == last_line[j];

		/* set the first line */
		if (match)
			start_line = next_buf + i + last_line_length;
	}

	if (start_line == NULL) {
		fprintf(stderr, "unable to identify last read line. Maybe the "
			"demo-buffer is not large enough. Or the historical "
			"records have been cleared by a third party.\n");
		start_line = next_buf;
	}

	/* output the new logs */
	printf("\n[read-new] round %d:\n", round++);
	int new_length = strlen(start_line);
	if (new_length == 0) {
		printf("empty.\n");
		last_line = next_buf;
		last_line_length = 0;
		next_buf = next_buf == buf1 ? buf2 : buf1;
		return 0;
	}

	write(1, start_line, strlen(start_line));

	/* update last_line & last_line_length */
	last_line = next_buf + next_length - 1;
	while (last_line != start_line && *(last_line - 1) != '\n')
		last_line--;
	last_line_length = next_buf + next_length - last_line;

	/* update buffer */
	next_buf = next_buf == buf1 ? buf2 : buf1;

	return 0;
}

int klog_unread_demo(void)
{
	printf("\n[klog_unread_demo].\n");

	int ret = 0;
	ret |= write_kmesg("klog-demo: test klog_unread.");
	ret |= klogctl(SYSLOG_ACTION_UNREAD, NULL, 0);
	printf("SYSLOG_ACTION_UNREAD returns %d.\n", ret);
	if (ret == -1) {
		fprintf(stderr, "failed to run klog.unread. %s.\n", strerror(errno));
		return -1;
	}

	return klog_clear();
}

int klog_read_all_repeated(void)
{
	printf("\n[klog_read_all_repeated].\n");

	if (write_kmesg("klog-demo: test klog_read_all_repeated"))
		return -1;

	/* non-clear read is repeatable */
	char buf1[BUF_SIZE], buf2[BUF_SIZE];
	int ret1 = klogctl(SYSLOG_ACTION_READ_ALL, buf1, BUF_SIZE);
	int ret2 = klogctl(SYSLOG_ACTION_READ_ALL, buf2, BUF_SIZE);
	if (ret1 == -1 || ret2 == -1) {
		fprintf(stderr, "failed to run klog.read_all(). %s.\n", strerror(errno));
		return -1;
	}

	if (ret1 != ret2 || strncmp(buf1, buf2, ret1) != 0) {
		fprintf(stderr, "klog.read_all found not repeatable.\n");
		return -1;
	}

	/* dump the kmesg to stdout */
	printf("SYSLOG_ACTION_READ_ALL reads %d bytes into a %d-byte buffer."
		" The read is repeatable.\n", ret1, BUF_SIZE);
	write(1, buf1, ret1);
	putchar('\n');

	return 0;
}

int klog_read_new_demo(void)
{
	printf("\n[klog_read_new_demo]\n");
	return write_kmesg("klog-demo: test klog_read_new log1") ||
		klog_read_new() ||
		write_kmesg("klog-demo: test klog_read_new log2") ||
		klog_read_new() ||
		write_kmesg("klog-demo: test klog_read_new log3") ||
		write_kmesg("klog-demo: test klog_read_new log4") ||
		write_kmesg("klog-demo: test klog_read_new log5") ||
		write_kmesg("klog-demo: test klog_read_new log6") ||
		klog_read_new() ||
		klog_clear();
}

int klog_read_clear_demo(void)
{
	printf("\n[klog_read_clear_demo]\n");
	/* the demo shows taht clear-read is not repeatable */
	if (write_kmesg("klog-demo: test klog_read_clear log1") ||
		write_kmesg("klog-demo: test klog_read_clear log2"))
		return -1;

	int ret1, ret2;
	char buf1[BUF_SIZE], buf2[BUF_SIZE];
	ret1 = klogctl(SYSLOG_ACTION_READ_CLEAR, buf1, BUF_SIZE);
	ret2 = klogctl(SYSLOG_ACTION_READ_ALL, buf2, BUF_SIZE);
	if (ret1 == -1 || ret2 == -1) {
		fprintf(stderr, "failed to run klog.read_all(). %s.\n", strerror(errno));
		return -1;
	}

	printf("SYSLOG_ACTION_READ_CLEAR reads %d bytes into a %d-byte buffer.\n",
		ret1, BUF_SIZE);
	printf("SYSLOG_ACTION_READ_ALL reads %d bytes into a %d-byte buffer.\n",
		ret2, BUF_SIZE);
	if (ret1 == 0 || ret2 != 0) {
		fprintf(stderr, "klog.read_clear does not properly clear the buffer.\n");
		return -1;
	}

	return klog_clear();
}
