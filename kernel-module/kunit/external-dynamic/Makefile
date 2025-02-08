# two products: $(CURDIR)/dynamic_demo.ko and $(CURDIR)/tests/dynamic_demo_test.ko
KDIR ?= /lib/modules/`uname -r`/build

PWD := $(CURDIR)

dynamic_demo.ko Module.symvers:
	$(info Building demo module...)
	$(MAKE) -C $(KDIR) M=$(PWD) modules

kunit: Module.symvers
	$(info Building test module...)
	$(MAKE) -C $(KDIR) M=$(PWD)/tests DYNAMIC_DEMO_SYMBOLS=$(PWD)/Module.symvers modules

default: dynamic_demo.ko

all: dynamic_demo.ko kunit

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	$(MAKE) -C $(KDIR) M=$(PWD)/tests clean
