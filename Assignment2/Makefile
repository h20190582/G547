#/*----------------------------------------------------------------------------
# * Name:    Makefile
# * Author:  Desai Aneri Hiren (2019H1400582P)
# * Course : EEE G547 Device Drivers
# * Assignment : 2
# * System Detail : UBUNTU 16.04
#                   Kernel Version :4.15.0-72-generic  
#*----------------------------------------------------------------------------
#*/

obj-m := usb_driver.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
