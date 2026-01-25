BUILDDIR = build

CC = gcc
CFLAGS = -Wall -Wextra -Wfloat-equal -fno-common

SOURCES = config-demo.c cli_source.c env_source.c file_source.c defaults_source.c cli_options.c
HEADERS = config.h config_source.h cli_options.h
OBJECTS = $(addprefix $(BUILDDIR)/, $(SOURCES:.c=.o))

$(BUILDDIR)/config-demo: $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^

$(BUILDDIR)/%.o: %.c $(HEADERS) | $(BUILDDIR)
	$(CC) $(CFLAGS) -c -o $@ $<

test: test-config-demo.sh
	@echo -e -n '\033[0;32m[$@]:started\033[0m\n'
	./test-config-demo.sh
	@echo -e -n '\033[0;32m[$@]:passed\033[0m\n'

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

clean:
	- rm -rf $(BUILDDIR)

.PHONY: clean test
