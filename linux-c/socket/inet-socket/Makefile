BUILDDIR = build
VPATH = $(BUILDDIR)

CC = gcc
CFLAGS = -Wall -Wextra -Wfloat-equal -fno-common

define test_msg
	@echo -e -n '\033[0;32m[$@]:$(1)\033[0m\n'
endef

$(BUILDDIR)/%.o: %.c %.h
	$(CC) $(CFLAGS) -o $@ -c $<

$(BUILDDIR)/server.o: server.c inet_socket_demo.h inet_socket.h inet_io.h
	$(CC) $(CFLAGS) -o $@ -c $<
server_objs := server.o inet_io.o inet_socket.o
$(BUILDDIR)/server: $(addprefix $(BUILDDIR)/, $(server_objs))
	$(CC) $(CFLAGS) -o $@ $^

$(BUILDDIR)/client.o: client.c inet_socket_demo.h inet_socket.h inet_io.h
	$(CC) $(CFLAGS) -o $@ -c $<
client_objs := client.o inet_io.o inet_socket.o
$(BUILDDIR)/client: $(addprefix $(BUILDDIR)/, $(client_objs))
	$(CC) $(CFLAGS) -o $@ $^

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
