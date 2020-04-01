/*----------------------------------------------------------------------------
 * Name:    README 
 * Author:  Desai Aneri Hiren (2019H1400582P)
 * Course : EEE G547 Device Drivers
 * Assignment : 2
 * System Detail : UBUNTU 16.04
                   Kernel Version :4.15.0-72-generic  
*----------------------------------------------------------------------------
*/

------------------------------------------------------------------------------------------------------------------------------------------
To avoid removing uas and usb_storage module every time when our module is inserted , I have black listed both above mentioned modules by following below mentioned steps :

1) Go to /etc/modprobe.d
2)Open blacklist.conf (Open as root user)
3) Add These two lines in  blacklist.conf
   
   blacklist uas
   blacklist usb_storage

             or

Can also execute below command everytime before inserting USB device
  
    $ sudo modprobe -rf uas usb_storage 

------------------------------------------------------------------------------------------------------------------------------------------

Compile & Insert Kernel Module:

$ make all
$ sudo insmod ./usb_driver.ko

Check Kernel Logs

$ dmesg


