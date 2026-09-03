obj-m += privesc_module.o

KDIR := /lib/modules/$(shell uname -r)/build
PWD  := $(shell pwd)

all: module exploit

module:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

exploit:
	gcc -Wall -Wextra -o exploit exploit.c

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -f exploit