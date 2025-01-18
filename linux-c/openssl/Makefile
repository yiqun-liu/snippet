BUILDDIR = build
VPATH = $(BUILDDIR)

SUBDIRS = semaphore

CC = gcc
CFLAGS = -Wall -Wextra -Wfloat-equal -fno-common

define test_msg
	@echo -e -n '\033[0;32m[$@]:$(1)\033[0m\n'
endef

$(BUILDDIR)/digest: digest.c
	$(CC) $(CFLAGS) -o $@ $< -lssl -lcrypto

test-digest: prepare digest
	$(call test_msg, started)
	./$(BUILDDIR)/digest
	$(call test_msg, passed\n)

prepare:
	@mkdir -p $(BUILDDIR)

test: test-digest

clean:
	- rm -rf $(BUILDDIR)

.PHONY: clean prepare test test-digest
.DEFAULT_GOAL = test
