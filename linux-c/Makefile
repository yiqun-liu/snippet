BUILDDIR = build
VPATH = $(BUILDDIR)

SUBDIRS = semaphore

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

$(BUILDDIR)/va-to-pa: va-to-pa.c
	$(CC) $(CFLAGS) -o $@ $<

# require priviledge to run
test-va-to-pa: prepare va-to-pa
	$(call test_msg, started)
	timeout --signal=INT --preserve-status 1s ./$(BUILDDIR)/va-to-pa
	$(call test_msg, passed\n)

$(BUILDDIR)/libnuma-demo: libnuma-demo.c
	$(CC) $(CFLAGS) -o $@ $< -lnuma

test-libnuma-demo: prepare libnuma-demo
	$(call test_msg, started)
	./$(BUILDDIR)/libnuma-demo
	$(call test_msg, passed\n)

prepare:
	@mkdir -p $(BUILDDIR)

test: test-perf-count test-libnuma-demo test-klog-demo test-va-to-pa
	for subdir in $(SUBDIRS) ; do \
		$(MAKE) -C $$subdir $(@) ; \
	done

clean:
	- rm -rf $(BUILDDIR)
	for subdir in $(SUBDIRS) ; do \
		$(MAKE) -C $$subdir $(@) ; \
	done

.PHONY: clean test prepare
.DEFAULT_GOAL = test
