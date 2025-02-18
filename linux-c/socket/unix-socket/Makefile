BUILDDIR = build
VPATH = $(BUILDDIR)

CC = gcc
CFLAGS = -Wall -Wextra -Wfloat-equal -fno-common

define test_msg
	@echo -e -n '\033[0;32m[$@]:$(1)\033[0m\n'
endef

$(BUILDDIR)/server: server.c unix_socket_demo.h
	$(CC) $(CFLAGS) -o $@ $<

$(BUILDDIR)/client: client.c unix_socket_demo.h
	$(CC) $(CFLAGS) -o $@ $<

prepare:
	@mkdir -p $(BUILDDIR)

test: prepare client server
	$(call test_msg, started)
	$(BUILDDIR)/server & s_pid=$$! ; sleep 1 ; $(BUILDDIR)/client & c_pid=$$! ; wait $$s_pid && wait $$c_pid
	$(call test_msg, passed\n)

clean:
	- rm -rf $(BUILDDIR)

.PHONY: clean test prepare
.DEFAULT_GOAL = test
