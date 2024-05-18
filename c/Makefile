BUILDDIR = build
VPATH = $(BUILDDIR)

CC = gcc
CFLAGS = -Wall -Wextra -Wfloat-equal -fno-common

define test_msg
	@echo -e -n '\033[0;32m[$@]:$(1)\033[0m\n'
endef

$(BUILDDIR)/struct-demo: struct-demo.c
	$(CC) $(CFLAGS) -o $@ $<

$(BUILDDIR)/union-demo: union-demo.c
	$(CC) $(CFLAGS) -o $@ $<

test-struct-demo: prepare struct-demo
	$(call test_msg, started)
	./$(BUILDDIR)/struct-demo
	$(call test_msg, passed\n)

test-union-demo: prepare union-demo
	$(call test_msg, started)
	./$(BUILDDIR)/union-demo
	$(call test_msg, passed\n)

prepare:
	@mkdir -p $(BUILDDIR)

test: test-struct-demo test-union-demo

clean:
	- rm -rf $(BUILDDIR)

.PHONY: clean test prepare
.DEFAULT_GOAL = test
