BUILDDIR = build
VPATH = $(BUILDDIR)
CC = gcc
CFLAGS = -Wall -Wextra -Wfloat-equal -fno-common

KDIR ?= /lib/modules/`uname -r`/build
PWD := $(CURDIR)

$(BUILDDIR)/demo_reader: demo_reader.c
	$(CC) $(CFLAGS) -o $@ $<

default:
	- mkdir build
	$(MAKE) demo_reader
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	- rm -rf $(BUILDDIR)

.PHONY: clean default prepare
.DEFAULT_GOAL = default
