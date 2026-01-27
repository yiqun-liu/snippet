obj-m += memdev.o

KDIR ?= /lib/modules/$(shell uname -r)/build

PWD := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -f test/test_memdev

test:
	gcc -Wall -Wextra -g -O2 -o test/test_memdev test/test_memdev.c

.PHONY: all clean test
