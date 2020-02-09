 /*------------------------------------------------------------------------------------------------------------ 
 * Name:    userspp.c :   User space program which will do various operations on created device file dev/adc8
 * Author:  Desai Aneri Hiren (2019H1400582P)
 * Course : EEE G547 Device Drivers
 * Assignment : 1
 * System Detail : UBUNTU 16.04
                   Kernel Version :4.15.0-72-generic                   
 *-------------------------------------------------------------------------------------------------------------
*/

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>             
#include <unistd.h>             
#include <sys/ioctl.h> 
#include <stdint.h>         

#include "charadc.h"

/*
 * Functions for the ioctl calls
 */

int  ioctl_sel_channel(int file_desc, char *channel)
{
    int ret_val;

    ret_val = ioctl(file_desc, IOCTL_SEL_CHANNEL, channel);

    if (ret_val < 0) {
        printf("ioctl_sel_channel failed:%d\n", ret_val);
        exit(-1);
    }
    return 0;
}

int  ioctl_sel_align(int file_desc, char *align)
{
    int ret_val;

    ret_val = ioctl(file_desc, IOCTL_SEL_ALIGN, align);

    if (ret_val < 0) {
        printf("ioctl_sel_align failed:%d\n", ret_val);
        exit(-1);
    }
    return 0;
}



/*
 * Main - Call the ioctl functions
 */
int main()
{
    int file_desc,adc_ch_no,data_align,ret_val;
    uint16_t buffer;
    

    file_desc = open(DEVICE_FILE_NAME, 0);
    if (file_desc < 0) {
        printf("Can't open device file: %s\n", DEVICE_FILE_NAME);
        exit(-1);
    }
 
    
    printf("Enter ADC channel no. (0 to 7) to read data : ");
    adc_ch_no = getc(stdin);

    while (adc_ch_no == 10) //ASCII value of enter is 10. So when user only presses enter without any value keepon asking value
    {

       printf("Enter ADC channel no. (0 to 7) to read data : ");
       adc_ch_no = getc(stdin);
    }

    adc_ch_no -= 48;
   

    if ((adc_ch_no >7) || (adc_ch_no < 0 ))
    {
        printf("Invalid channel number.\n");

        printf("Reexecute program with valid channel number\n");

        return 0;
    }
    
    ioctl_sel_channel(file_desc,&adc_ch_no);
    printf("\n");

    int junk = getc(stdin); //It will capture ENTER key pressed after entering channel value 

    printf("Enter 0 to get data aligned in higher 10 bits or Enter 1 to align data in lower 10 bits : ");

    data_align = getc(stdin);
    
   while (data_align == 10) //ASCII value of enter is 10. So when user only presses enter without any value keepon asking value
    {

       printf("Enter 0 to get data aligned in higher 10 bits or Enter 1 to align data in lower 10 bits : ");
       data_align = getc(stdin);
    }

    data_align -= 48;


     if ((data_align >1) || (data_align < 0 ))
    {
        printf("Invalid alignment selection.\n");

        printf("Reexecute program with valid selection\n");

        return 0;
    }


      ioctl_sel_align(file_desc,&data_align);

      ret_val = read(file_desc,&buffer,sizeof(buffer));

     
      if(ret_val >= 0)
      {
          if(data_align == 0) // will get left aligned data. So need to shift 6 positions to right to get correct value
         {
	    buffer = buffer >> 6;
         }

         printf("\nADC data from channel %d : %d\n",adc_ch_no,buffer);

      }
      else
      {
          printf("Device Read Failed\n");

      }
 
    close(file_desc);  


    return 0;
}
