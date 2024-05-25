BUILDDIR = build
VPATH = $(BUILDDIR)

CC = gcc
# NOTE: unused functions warning functions are suppressed before we want to simply the code sharing
# between two test demos.
CFLAGS = -Wall -Wextra -Wfloat-equal -fno-common -Wno-unused-function

define test_msg
	@echo -e -n '\033[0;32m[$@]:$(1)\033[0m\n'
endef

$(BUILDDIR)/named-semaphore: named-semaphore.c semaphore-utils.h
	$(CC) $(CFLAGS) -o $@ $<

$(BUILDDIR)/unnamed-semaphore: unnamed-semaphore.c semaphore-utils.h
	$(CC) $(CFLAGS) -o $@ $<

prepare:
	@mkdir -p $(BUILDDIR)

test-named-semaphore: prepare named-semaphore
	$(call test_msg, started)
	./test-named-semaphore.sh
	$(call test_msg, passed\n)

test-unnamed-semaphore: prepare unnamed-semaphore
	$(call test_msg, started)
	$(BUILDDIR)/unnamed-semaphore
	$(call test_msg, passed\n)

test: test-named-semaphore test-unnamed-semaphore

clean:
	- rm -rf $(BUILDDIR)

.PHONY: clean test prepare
.DEFAULT_GOAL = test
