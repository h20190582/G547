/*----------------------------------------------------------------------------
 * Name:    usb_driver.c : Read Capacity of USB Pendrive
 * Author:  Desai Aneri Hiren (2019H1400582P)
 * Course : EEE G547 Device Drivers
 * Assignment : 2
 * System Detail : UBUNTU 16.04
                   Kernel Version :4.15.0-72-generic  
*----------------------------------------------------------------------------
*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/usb.h>
#include <linux/slab.h>

//Device list
#define SANDISK_PEN_VID  0x0781
#define SANDISK_PEN_PID  0x5567

#define HP_PEN_VID  0x03f0 
#define HP_PEN_PID  0xdb07

#define TOSHIBA_PEN_VID  0x0930 
#define TOSHIBA_PEN_PID  0x6544

#define SANDISK1_PEN_VID  0x0781
#define SANDISK1_PEN_PID  0x558a


#define be_to_int32(buf) (((buf)[0]<<24)|((buf)[1]<<16)|((buf)[2]<<8)|(buf)[3])


#define USB_ENDPOINT_IN 0x80
#define USB_REQUEST_TYPE_CLASS (0x01 << 5)
#define USB_RECIPIENT_INTERFACE 0x01

#define RETRY_MAX 5
#define READ_CAPACITY_LENGTH          0x08
#define INQUIRY_LENGTH                0x24
#define REQUEST_SENSE_LENGTH          0x12

// Mass Storage Requests values.
#define BOMS_GET_MAX_LUN              0xFE

 
static struct usb_device *device;

//Command Block Wrapper (CBW)
struct command_block_wrapper {
	uint8_t dCBWSignature[4];
	uint32_t dCBWTag;
	uint32_t dCBWDataTransferLength;
	uint8_t bmCBWFlags;
	uint8_t bCBWLUN;
	uint8_t bCBWCBLength;
	uint8_t CBWCB[16];
};

//Command Status Wrapper (CSW)
struct command_status_wrapper {
	uint8_t dCSWSignature[4];
	uint32_t dCSWTag;
	uint32_t dCSWDataResidue;
	uint8_t bCSWStatus;
};


static uint8_t cdb_length[256] = {
//	 0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
	06,06,06,06,06,06,06,06,06,06,06,06,06,06,06,06,  //  0
	06,06,06,06,06,06,06,06,06,06,06,06,06,06,06,06,  //  1
	10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,  //  2
	10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,  //  3
	10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,  //  4
	10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,  //  5
	00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,  //  6
	00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,  //  7
	16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,  //  8
	16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,  //  9
	12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,  //  A
	12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,  //  B
	00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,  //  C
	00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,  //  D
	00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,  //  E
	00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,  //  F
};

static int send_mass_storage_command(struct usb_device *usb_dev, unsigned int endpoint,uint8_t lun,uint8_t *cdb, uint8_t direction, int data_length, uint32_t *ret_tag)
{

     static uint32_t tag = 1;
     struct command_block_wrapper *cbwptr;  
     int retval,size;
     uint8_t cdb_len; 
      int j;


    cbwptr = kmalloc(sizeof(struct command_block_wrapper),GFP_KERNEL);
    if(cbwptr == NULL)
    {
       printk("Error! memory not allocated for CBW\n");
       return -1;
    }
    


     if (cdb == NULL)
     {
       return -1;
     }

     if (endpoint & USB_ENDPOINT_IN) 
      {
		printk("send_mass_storage_command: cannot send command on IN endpoint\n");
		return -1;
      }

      memcpy(&cdb_len, &cdb_length[cdb[0]],sizeof(cdb_len));
          

     if ((cdb_len == 0) || (cdb_len > sizeof(cbwptr->CBWCB))) 
     {
       printk("send_mass_storage_command: don't know how to handle this command (%02X, length %d)\n",cdb[0], cdb_len);
                		return -1;
     }

     
         for(j = 0; j < 3;j++) 
         {    
     	      memset(&(cbwptr->dCBWSignature[j]),0,sizeof(cbwptr->dCBWSignature[j]));
             
         }

	 memset(&(cbwptr->dCBWTag),0,sizeof(cbwptr->dCBWTag));
	 memset(&(cbwptr->dCBWDataTransferLength),0,sizeof(cbwptr->dCBWDataTransferLength));
	 memset(&(cbwptr->bmCBWFlags),0,sizeof(cbwptr->bmCBWFlags));
	 memset(&(cbwptr->bCBWLUN),0,sizeof(cbwptr->bCBWLUN));
	 memset(&(cbwptr->bCBWCBLength),0,sizeof(cbwptr->bCBWCBLength));

        for(j = 0; j < 16;j++)
       {
	 memset(&(cbwptr->CBWCB[j]), 0,sizeof(cbwptr->CBWCB[j]));
       }
      

      memset(&(cbwptr->dCBWSignature[0]),85,sizeof(cbwptr->dCBWSignature[0]));
      memset(&(cbwptr->dCBWSignature[1]),83,sizeof(cbwptr->dCBWSignature[1]));
      memset(&(cbwptr->dCBWSignature[2]),66,sizeof(cbwptr->dCBWSignature[2]));
      memset(&(cbwptr->dCBWSignature[3]),67,sizeof(cbwptr->dCBWSignature[3]));
     *ret_tag = tag;
      memset(&(cbwptr->dCBWTag),tag++,sizeof(cbwptr->dCBWTag));
      memset(&(cbwptr->dCBWDataTransferLength),data_length,sizeof(cbwptr->dCBWDataTransferLength));
      memset(&(cbwptr->bmCBWFlags),direction,sizeof(cbwptr->bmCBWFlags));
      memset(&(cbwptr->bCBWLUN),lun,sizeof(cbwptr->bCBWLUN));     
      memset(&(cbwptr->bCBWCBLength),cdb_len,sizeof(cbwptr->bCBWCBLength));
      memcpy(cbwptr->CBWCB,cdb,cdb_len);

     
        retval = usb_bulk_msg(usb_dev,usb_sndbulkpipe(usb_dev,endpoint),cbwptr,31,&size,0);

        if (retval < 0) 
	{
		printk(KERN_ERR "Bulk message returned %d\n", retval); 
		return retval;
	}
         

	return 0;
}

static int get_mass_storage_status(struct usb_device *usb_dev, unsigned int endpoint, uint32_t e_tag)
{
	int r, size;
        int *sizeptr;
        struct command_status_wrapper *cswptr;

        cswptr = kmalloc(sizeof(struct command_status_wrapper),GFP_KERNEL);
        sizeptr = kmalloc(sizeof(size),GFP_KERNEL);


          if(cswptr == NULL)
          {
                 printk("Error! memory not allocated for CBW\n");
                 return -1;
          }

          if(sizeptr == NULL)
          {
                 printk("Error! memory not allocated for CBW\n");
                 return -1;
          }

        memset(cswptr,0,sizeof(struct command_status_wrapper)); 


         r = usb_bulk_msg(usb_dev, usb_rcvbulkpipe(usb_dev,endpoint),cswptr, 13, sizeptr, 0);
                
         if(r < 0)
         {
		printk(KERN_ERR "Bulk message returned %d\n", r);
	        return r;
         }
		

	if (sizeptr[0] != 13) {
		printk("   get_mass_storage_status: received %d bytes ( expected 13)\n", sizeptr[0]);
		return -1;
	}
	if (cswptr->dCSWTag != e_tag) {
		printk("   get_mass_storage_status: mismatched tags (expected %08X, received %08X)\n",e_tag, cswptr->dCSWTag);
		return -1;
	}
	printk("   Mass Storage Status: %02X (%s)\n", cswptr->bCSWStatus, cswptr->bCSWStatus?"FAILED":"Success");

	if (cswptr->bCSWStatus) 
        {
			
	    if (cswptr->bCSWStatus == 1)
		return -2;	
	    else
	        return -1;
	}
	return 0;
}

static int test_mass_storage(struct usb_device *usb_dev, unsigned int endpoint_in,  unsigned int endpoint_out)
{
        int r, size,j;
	uint8_t size8,requestType;
        uint32_t expected_tag,max_lba, block_size;
	unsigned long int device_size;    
        uint8_t* cdbptr; 
        uint8_t* bufferptr;
        uint8_t *lunptr;
        uint32_t *expected_tagptr;

        //char *vid;
        //char *pid;
        //char *rev;
        //unsigned char sizechar;



        cdbptr = kmalloc(16 * sizeof(size8),GFP_KERNEL);
        bufferptr = kmalloc(64 * sizeof(size8),GFP_KERNEL);
        lunptr =  kmalloc(sizeof(size8),GFP_KERNEL);
        expected_tagptr = kmalloc(sizeof(expected_tag),GFP_KERNEL); 

        //vid = kmalloc(9 * sizeof(sizechar),GFP_KERNEL);
        //pid = kmalloc(9 * sizeof(sizechar),GFP_KERNEL);
        //rev = kmalloc(5 * sizeof(sizechar),GFP_KERNEL);


        if(cdbptr == NULL)
        {
          printk("Error! memory not allocated for CDB\n");
          return -1;
        }
        
         if(bufferptr == NULL)
         {
            printk("Error! memory not allocated for Buffer.\n");
            return -1;
         }
         if(lunptr == NULL)
         {
            printk("Error! memory not allocated for LUN.\n");
            return -1;
         }



         /* if(vid == NULL)
          {
            printk("Error! memory not allocated for vid\n");
            return -1;
          }
          if(pid == NULL)
          {
            printk("Error! memory not allocated for pid\n");
            return -1;
          }
        if(rev == NULL)
          {
            printk("Error! memory not allocated for rev\n");
            return -1;
          }*/


        for(j = 0; j < 16 ; j++)
        {
          cdbptr[j] = 0 ;  
        }

         for(j = 0; j < 64 ; j++)
        {
           bufferptr[j] = 0 ;  
        } 
        lunptr[0] = 0; 
       
        requestType = USB_ENDPOINT_IN |USB_REQUEST_TYPE_CLASS | USB_RECIPIENT_INTERFACE;

       	r = usb_control_msg(usb_dev,usb_rcvctrlpipe(usb_dev,0), BOMS_GET_MAX_LUN,requestType,0,0,lunptr,1,0);

        if (r < 0) 
        {
	    	printk("  Failed: %d usb_control_msg for Max LUN\n",r);

	}
         
        // Read capacity
   	printk("Reading Capacity:\n");
        memset(&cdbptr[0],0x25,sizeof(cdbptr[0]));
        
        send_mass_storage_command(usb_dev,endpoint_out,lunptr[0],cdbptr,USB_ENDPOINT_IN,READ_CAPACITY_LENGTH, expected_tagptr);   
        usb_bulk_msg(usb_dev, usb_rcvbulkpipe(usb_dev,endpoint_in), bufferptr, READ_CAPACITY_LENGTH, &size, 0);//Check return value n throw error

         if (r < 0)  
        {
              printk("Error in usb_bulk_msg for Read Capacity %d \n",r);
              return -1;

	} 



	max_lba = be_to_int32(bufferptr);
	block_size = be_to_int32(&bufferptr[4]);
	device_size =  (((unsigned long int)(max_lba+1))*block_size)/(1024*1024);
        printk("Max LBA: %08X, Block Size: %08X \n", max_lba, block_size);
        printk("Device Capacity  : %lu MB\n",device_size); 


	/*if (get_mass_storage_status(usb_dev, endpoint_in, expected_tagptr[0]) < 0) {
              printk("Error in Mass Storage Status\n");
	}*/


        //Send Inquiry


	/*printk("Sending Inquiry:\n");
        memset(&(cdbptr[0]),0,16*sizeof(cdbptr[0]));
        memset(&(bufferptr[0]),0,64*sizeof(cdbptr[0]));

        memset(&(cdbptr[0]),0x12,sizeof(cdbptr[0]));
        memset(&(cdbptr[4]),0x24,sizeof(cdbptr[4]));
          
        send_mass_storage_command(usb_dev, endpoint_out, lunptr[0], cdbptr, USB_ENDPOINT_IN, INQUIRY_LENGTH, expected_tagptr);

        r = usb_bulk_msg(usb_dev, usb_rcvbulkpipe(usb_dev,endpoint_in), bufferptr, INQUIRY_LENGTH, &size, 0);

        if (r < 0)  
        {
              printk("Error in usb_bulk_msg  %d \n",r);

	}

        for (i=0; i<8; i++) {
	      memcpy(&(vid[i]), &(bufferptr[8+i]),sizeof(vid[i])); 
              memcpy(&(pid[i]), &(bufferptr[16+i]),sizeof(pid[i]));
              memcpy(&(rev[i/2]), &(bufferptr[32 + i/2]),sizeof(rev[i/2]));
	}

        memset(&(vid[8]),0,sizeof(vid[8]));
        memset(&(pid[8]),0,sizeof(pid[8]));
        memset(&(rev[4]),0,sizeof(rev[4]));

        printk("   VID:PID:REV \"%8s\":\"%8s\":\"%4s\"\n", vid, pid, rev);

        if (get_mass_storage_status(usb_dev, endpoint_in, expected_tagptr[0]) < 0) {
              printk("Error in Mass Storage Status\n");
	}*/

    
   return 0;
}


static int pen_probe(struct usb_interface *interface, const struct usb_device_id *id)
{
        int i;
	unsigned char epAddr, epAttr;
	struct usb_endpoint_descriptor *ep_desc;
        unsigned int endpoint_in = 0, endpoint_out = 0;
        struct usb_host_interface *iface_desc;

        iface_desc = interface->cur_altsetting;
        device = interface_to_usbdev(interface);
 
        printk("UAS READ Capacity Driver Inserted\n");

        printk("In Kernel Module: %s\n",THIS_MODULE->name);


         // Check if the device is USB attaced SCSI type Mass storage class
         /*
             bInterfaceClass - 0x08 (Mass Storage Class)
             bInterfaceSubClass - 0x06 (SCSI Primary Command-2 (SPC-2))
             bInterfaceProtocol - 0x50 (Bulk Only Transport)
         */


      if(!((iface_desc->desc.bInterfaceClass == 0x08) && (iface_desc->desc.bInterfaceSubClass == 0x06 ) && (iface_desc->desc.bInterfaceProtocol == 0x50 )))
      {

        printk(" Device is not USB attaced SCSI type Mass storage class\n");
        return -1;

      }     


	if(((id->idProduct == SANDISK_PEN_PID) && (id->idVendor == SANDISK_PEN_VID )) ||((id->idProduct == SANDISK1_PEN_VID ) && (id->idVendor == SANDISK1_PEN_VID)) ||((id->idProduct == TOSHIBA_PEN_PID) && (id->idVendor == TOSHIBA_PEN_VID)) ||((id->idProduct == HP_PEN_PID) && (id->idVendor == HP_PEN_VID)))
	{
		printk(KERN_INFO "Known USB drive detected\n");
	}
        
        printk("VID:PID : %X : %X\n",device->descriptor.idVendor,device->descriptor.idProduct);  
        printk("Interface Class : %X\n",iface_desc->desc.bInterfaceClass);
        printk("Interface Subclass : %X \n",iface_desc->desc.bInterfaceSubClass);
        printk("Interface Protocol : %X\n",iface_desc->desc.bInterfaceProtocol);

        printk("No. of Endpoints : %d \n",interface->cur_altsetting->desc.bNumEndpoints);
 

    for(i=0;i<interface->cur_altsetting->desc.bNumEndpoints;i++)
	{
		ep_desc = &interface->cur_altsetting->endpoint[i].desc;
		epAddr = ep_desc->bEndpointAddress;
		epAttr = ep_desc->bmAttributes;
	
		if((epAttr & USB_ENDPOINT_XFERTYPE_MASK)==USB_ENDPOINT_XFER_BULK) //Check if endpoint is Bulk Endpoint or not
		{
			if(epAddr & USB_ENDPOINT_IN)
                        {
                                endpoint_in = 	epAddr;
                                printk(KERN_INFO " endpoint[%d] is Bulk Endpoint IN\n", i);

                        }
			else
                        {
                                endpoint_out = epAddr;
                                printk(KERN_INFO " endpoint[%d] is Bulk Endpoint OUT\n", i);
			
                        }	
		}
	}

      test_mass_storage(device,endpoint_in,endpoint_out);

        
      return 0;
}
 
static void pen_disconnect(struct usb_interface *interface)
{
    	printk(KERN_INFO "pen_driver Device Removed\n");

}
 
static struct usb_device_id pen_table[] =
{
    {USB_DEVICE(SANDISK_PEN_VID, SANDISK_PEN_PID) },
    {USB_DEVICE(HP_PEN_VID, HP_PEN_PID)},
    {USB_DEVICE(TOSHIBA_PEN_VID, TOSHIBA_PEN_PID)},
    {USB_DEVICE(SANDISK1_PEN_VID, SANDISK1_PEN_PID) },
    {} /* Terminating entry */
};
MODULE_DEVICE_TABLE (usb, pen_table);
 
static struct usb_driver pen_driver =
{
    .name = "pen_driver",  //name of the device
    .probe = pen_probe, // Whenever Device is plugged in
    .disconnect = pen_disconnect, // When device is removed
    .id_table = pen_table,  //  List of devices served by this driver
};
 
static int __init pen_init(void)
{

    return usb_register(&pen_driver);
}
 
static void __exit pen_exit(void)
{
    usb_deregister(&pen_driver);
}
 
module_init(pen_init);
module_exit(pen_exit);
 
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Aneri Desai");
MODULE_DESCRIPTION("USB Pen Info Driver");

