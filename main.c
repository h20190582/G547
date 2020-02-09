/*----------------------------------------------------------------------------
 * Name:    main.c :  Create an input/output character ADC
 * Author:  Desai Aneri Hiren (2019H1400582P)
 * Course : EEE G547 Device Drivers
 * Assignment : 1
 * System Detail : UBUNTU 16.04
                   Kernel Version :4.15.0-72-generic  
*----------------------------------------------------------------------------
*/


#include <linux/kernel.h>       
#include <linux/module.h>       
#include <linux/fs.h>
#include <linux/uaccess.h>         /* for get_user and put_user */
#include <linux/time.h>
#include<linux/random.h>
#include<linux/init.h>
#include<linux/types.h>
#include <linux/kdev_t.h>
#include <linux/device.h>
#include <linux/cdev.h>

#include "charadc.h"


#define SUCCESS 0
#define DEVICE_NAME "adc8"



/*
 * Is the device open right now? Used to prevent
 * concurent access into the same device
 */
static int adc_Open = 0;


//To store selected valiable

static int sel_ch;
static int sel_al;

static dev_t char_adc; // variable for device number
static struct cdev c_dev; // variable for the character device structure
static struct class *cls; // variable for the device class


/*get_adc_val() : Generate 10 bit random value */

uint16_t get_adc_val(void)
{
   uint16_t adc_val;
   struct timespec ts;
   ts = current_kernel_time();
   adc_val = (uint16_t)((ts.tv_nsec) % 1024);
   return adc_val;
}


/*
 * This is called whenever a process attempts to open the device file
 */

static int adc_open(struct inode *inode, struct file *file)
{
#ifdef DEBUG
        printk(KERN_INFO "adc_open(%p)\n", file);
#endif

    /*
     * We don't want to talk to two processes at the same time
     */
    if (adc_Open)
        return -EBUSY;

    adc_Open++;

    try_module_get(THIS_MODULE);
    return SUCCESS;
}

static int adc_close(struct inode *inode, struct file *file)
{
#ifdef DEBUG
    printk(KERN_INFO "adc_close(%p,%p)\n", inode, file);
#endif

    /*
     * We're now ready for our next caller
     */
    adc_Open--;

    module_put(THIS_MODULE);
    return SUCCESS;
}

static ssize_t adc_read(struct file *file,   /* see include/linux/fs.h   */
                           char __user * buffer,        /* buffer to be
                                                         * filled with data */
                           size_t length,       /* length of the buffer     */
                           loff_t * offset)
{
    /*
     * Number of bytes actually written to the buffer
     */
    
    int error_count,size_of_message;
    uint16_t adc_val;

#ifdef DEBUG
    printk(KERN_INFO "adc_read(%p,%p,%d)\n", file, buffer, length);
#endif
     
    switch(sel_ch)
   {
      case 0:
         adc_val=get_adc_val();
         break;
      case 1:
         adc_val=get_adc_val();
         break;
      case 2:
         adc_val=get_adc_val();
         break;
      case 3:
         adc_val=get_adc_val();
         break;
      case 4:
         adc_val=get_adc_val();
         break;
      case 5:
         adc_val=get_adc_val();
         break;
      case 6:
         adc_val=get_adc_val();
         
         break;
      case 7:
         adc_val=get_adc_val();
         break;
      default:

         printk("Invalid channel ID\n");
         return -1;
   }

     /* If Want data in higher 10 bits (sel_al = 0) then need to shift current data by 6 positions as by default it is right aligned.
 
        By default data is right aligned so nothing is done when sel_al = 1, it will be send as it is to user space.
     */

      if(sel_al == 0) 
      {
          //printk(KERN_INFO "When left shift : original value : %d\n",adc_val);
          adc_val = adc_val << 6;
          //printk(KERN_INFO "When left shift : shifted value : %d\n",adc_val);        
         
      }
 
       size_of_message = sizeof(adc_val);
       error_count = copy_to_user(buffer,&adc_val,size_of_message);
         
        if(error_count==0)
	{
           return (size_of_message=0);
   	} 

    return SUCCESS;
}



long adc_ioctl(struct file *file,             
                  unsigned int ioctl_num,        /* number and param for ioctl */
                  unsigned long ioctl_param)
{
     int *temp_channel_no;
     int *temp_align; 

    /*
     * Switch according to the ioctl called
     */
    switch (ioctl_num) {
    case IOCTL_SEL_CHANNEL:
          /*
         * Receive a pointer to a variable (in user space) and set that
         * to be the channel number.
         */

         temp_channel_no = (int *)ioctl_param;
         get_user(sel_ch,temp_channel_no);
         printk(KERN_INFO "Channel - %d is selected\n",sel_ch);

         

         break;

    case IOCTL_SEL_ALIGN:
          /*
         * Receive a pointer to a variable (in user space) and set that
         * to be the alignment of data read.
         */
          temp_align = (int *)ioctl_param;
          get_user(sel_al,temp_align );
          
          if(sel_al == 0)
          {
	    printk(KERN_INFO "Data will alligned in higher 10 bits\n");
	  }
          else if(sel_al == 1)
          {
	    printk(KERN_INFO "Data will alligned in lower 10 bits\n");	
	  }
           
         break;

       }

    return SUCCESS;
}



/* Module Declarations */

struct file_operations Fops = {
        .open = adc_open,
        .read = adc_read,
        .unlocked_ioctl = adc_ioctl,
        .release = adc_close, 
};

/*
 * Initialize the module - Register the character device
 */
int entry_module(void)
{

        // STEP 1 : reserve <major, minor>
	if (alloc_chrdev_region(&char_adc, 0, 1, DEVICE_NAME) < 0)
	{
		return -1;
	}


        	// STEP 2 : dynamically create device node in /dev directory
        if ((cls = class_create(THIS_MODULE, "charadc")) == NULL)
	{
		unregister_chrdev_region(char_adc, 1);
		return -1;
	}

       if (device_create(cls, NULL, char_adc, NULL, DEVICE_NAME) == NULL)
	{
		class_destroy(cls);
		unregister_chrdev_region(char_adc, 1);
		return -1;
	}
	
        // STEP 3 : Link fops and cdev to device node
         cdev_init(&c_dev, &Fops);
        if (cdev_add(&c_dev, char_adc, 1) == -1)
	{
		device_destroy(cls, char_adc);
		class_destroy(cls);
		unregister_chrdev_region(char_adc, 1);
		return -1;
	}

      printk(KERN_INFO "Device %s is registered.\n",DEVICE_NAME);

        
      return 0;
}

/*
 * Cleanup - unregister the appropriate file from /proc
 */
int exit_module(void)
{
    /*
     * Unregister the device
     */
    	cdev_del(&c_dev);
	device_destroy(cls, char_adc);
	class_destroy(cls);
	unregister_chrdev_region(char_adc, 1);

    

    printk(KERN_INFO "Device %s is unregistered.\n",DEVICE_NAME);

    return 0;

}

module_init(entry_module);
module_exit(exit_module);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Desai Aneri Hiren (2019H1400582P)");
MODULE_DESCRIPTION("G547 : Assignment 1 ");
