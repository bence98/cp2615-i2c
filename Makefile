#!/usr/bin/make
obj-m += i2c-cp2615.o
i2c-cp2615-objs := cp2615_drv.o cp2615_iop.o

modules:
	make -C /lib/modules/${shell uname -r}/build M=${shell pwd} $@

clean:
	make -C /lib/modules/${shell uname -r}/build M=${shell pwd} $@
	rm -rf cp2615.bin

insmod:
	sudo rmmod i2c-cp2615 || true
	sudo insmod ./i2c-cp2615.ko

cp2615.bin: cp2615_iop.c cp2615_usb.c
	gcc -DUSER_MODE -g $^ -lusb-1.0 -o $@
