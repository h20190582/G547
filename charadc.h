 /*--------------------------------------------------------------------------- 
 * Name:   charadc.h : the header file with the ioctl definitions.
 * Author:  Desai Aneri Hiren (2019H1400582P)
 * Course : EEE G547 Device Drivers
 * Assignment : 1
 * System Detail : UBUNTU 16.04
                   Kernel Version :4.15.0-72-generic  
 *----------------------------------------------------------------------------
*/

#ifndef CHARADC_H
#define CHARADC_H

#include <linux/ioctl.h>

#define PRIN_MAGIC 'A'
 

#define IOCTL_SEL_CHANNEL _IOW(PRIN_MAGIC, 0, int *)


#define IOCTL_SEL_ALIGN _IOW(PRIN_MAGIC, 1, int *) 


/*
 * The name of the device file
 */
#define DEVICE_FILE_NAME "/dev/adc8"


#endif
