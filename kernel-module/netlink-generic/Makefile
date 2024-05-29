BUILDDIR ?= build
INCLUDEDIR += /usr/include/libnl3

VPATH += $(BUILDDIR)
KDIR ?= /lib/modules/`uname -r`/build
PWD := $(CURDIR)
CC ?= gcc

CFLAGS += -Wall -Wextra -Wfloat-equal -fno-common -I$(INCLUDEDIR)
LDFLAGS += -lnl-3 -lnl-genl-3

$(BUILDDIR)/reporter: reporter.c genl_demo_uapi.h
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

$(BUILDDIR)/listener: listener.c genl_demo_uapi.h
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

prepare:
	@mkdir -p $(BUILDDIR)

compile_commands.json: genl_demo.c reporter.c listener.c genl_demo_uapi.h
	bear -- $(MAKE) --always-make

define test_msg
	@echo -e -n '\033[0;32m[$@]:$(1)\033[0m\n'
endef

test:
	$(call test_msg, started)
	$(MAKE) clean
	rsync -azPq ./ local-vm:/tmp/test/
	ssh local-vm bash /tmp/test/run-test.sh
	$(call test_msg, passed)

main: prepare reporter listener
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	- rm -rf $(BUILDDIR)
	$(MAKE) -C $(KDIR) M=$(PWD) clean

.PHONY: prepare

.DEFAULT_GOAL = main
