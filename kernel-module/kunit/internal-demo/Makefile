# NOTE: this is merely a test helper, which should never be put in the official release

KDIR ?= /lib/modules/`uname -r`/build

PWD := $(CURDIR)
INTREE_DIR := drivers/misc/internal-demo
DST_DIR := $(KDIR)/$(INTREE_DIR)

default:
	# safe guards
	! test -d $(DST_DIR)
	git diff-index --quiet HEAD -- && echo "kernel repo is clean"
	# helper scripts
	cp -r $(PWD)/ $(DST_DIR)
	# this Makefile is actually a helper script; for in-tree compilation, the file should not be
	# present
	mv $(DST_DIR)/Kbuild $(DST_DIR)/Makefile
	cd $(DST_DIR)/.. && sed -i "/endmenu/i\source \"$(INTREE_DIR)/Kconfig\"" Kconfig
	echo "obj-\$$(CONFIG_INTERNAL_DEMO)	+= internal-demo/" >> $(DST_DIR)/../Makefile
	cd $(KDIR) && git add . && git commit -m "demo kunit test `date +%Y%m%d-%H%M%S`"
	cd $(KDIR) && make ARCH=um mrproper
	$(KDIR)/tools/testing/kunit/kunit.py run --kunitconfig $(DST_DIR)/tests/.kunitconfig

clean:
	# safe guards
	test -d $(DST_DIR)
	cd $(KDIR) && git log --oneline -1 | grep "demo kunit test"
	# helper scripts
	cd $(KDIR) && git reset --hard HEAD^
	rm -rf $(DST_DIR)
	rm -rf $(KDIR)/.kunit
