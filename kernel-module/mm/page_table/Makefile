BUILDDIR ?= build
VPATH += $(BUILDDIR)

KDIR ?= /lib/modules/`uname -r`/build
PWD := $(CURDIR)
CC ?= gcc

CFLAGS += -Wall -Wextra -Wfloat-equal -fno-common

$(BUILDDIR)/issue_queries: issue_queries.c page_table_query.h
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

prepare:
	@mkdir -p $(BUILDDIR)

main: prepare issue_queries
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	- rm -rf $(BUILDDIR)
	$(MAKE) -C $(KDIR) M=$(PWD) clean

.PHONY: prepare

.DEFAULT_GOAL = main
