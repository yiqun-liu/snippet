KDIR ?= /lib/modules/`uname -r`/build

PWD := $(CURDIR)

default:
	$(info Building production version...)
	$(MAKE) -C $(KDIR) M=$(PWD) modules

kunit:
	$(info Building KUNIT test version...)
	$(MAKE) -C $(KDIR) M=$(PWD) KUNIT=1 modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
