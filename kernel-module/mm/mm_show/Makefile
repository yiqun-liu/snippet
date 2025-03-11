KDIR ?= /lib/modules/`uname -r`/build

PWD := $(CURDIR)

default:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
