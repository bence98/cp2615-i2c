#!/usr/bin/make
obj-m += cp2615_i2c.o
cp2615_i2c-objs := cp2615_drv.o cp2615_iop.o

modules:
	make -C /lib/modules/${shell uname -r}/build M=${shell pwd} $@

clean:
	make -C /lib/modules/${shell uname -r}/build M=${shell pwd} $@
	rm -rf cp2615.bin

cp2615.bin: cp2615_iop.c cp2615_usb.c
	gcc -DUSER_MODE -g $^ -lusb-1.0 -o $@
