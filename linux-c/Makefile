BUILDDIR = build
VPATH = $(BUILDDIR)

CC = gcc
CFLAGS = -Wall -Wextra -Wfloat-equal -fno-common

define test_msg
	@echo -e -n '\033[0;32m[$@]:$(1)\033[0m\n'
endef

$(BUILDDIR)/perf-count: perf-count.c
	$(CC) $(CFLAGS) -o $@ $<

test-perf-count: prepare perf-count
	$(call test_msg, started)
	./$(BUILDDIR)/perf-count
	$(call test_msg, passed\n)

$(BUILDDIR)/klog-demo: klog-demo.c
	$(CC) $(CFLAGS) -o $@ $<

# require priviledge to run
test-klog-demo: prepare klog-demo
	$(call test_msg, started)
	./$(BUILDDIR)/klog-demo
	$(call test_msg, passed\n)

prepare:
	@mkdir -p $(BUILDDIR)

test: test-perf-count

clean:
	- rm -rf $(BUILDDIR)

.PHONY: clean test prepare
.DEFAULT_GOAL = test
