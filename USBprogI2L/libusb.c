/****                                 ****/
/** Library for USB device management   **/
/****                                 ****/

////
// Include files
////

#include <stdio.h>
#include <stdlib.h>
#include <libusb-1.0/libusb.h>

#include "libusb.h"

////
// Constants
////

#define MAX_DEVICES		128
#define MAX_ENDPOINTS		16

#define INTERRUPT_DATA_SIZE	64
#define INTERRUPT_TIMEOUT	1000

////
// Structures
////

////
// Global variables
////

static libusb_context *context;
Device devices[MAX_DEVICES];
int nb_devices=0;

////
// Functions
////

// Scan for devices

void scan_for_devices(DeviceID *devids){
libusb_device **list;
int status=libusb_init(&context);
if(status<0) {perror("scan_for_devices.libusb_init"); exit(-1);}
ssize_t count=libusb_get_device_list(context,&list);
if(count<0) {perror("scan_for_devices.libusb_get_device_list"); exit(-1);}
ssize_t i=0;
for(i=0;i<count;i++){
  libusb_device *device=list[i];
  struct libusb_device_descriptor descriptor;
  status=libusb_get_device_descriptor(device,&descriptor);
  if(status!=0) continue;
  unsigned char found=0;
  DeviceID *id=devids;
  while(id->vendor>=0){
    if(descriptor.idVendor==id->vendor &&
       descriptor.idProduct==id->product)
      { found=1; break; }
    id++;
    }
  if(found){
#ifdef VERBOSE
    uint8_t address=libusb_get_device_address(device);
    printf("Device Found @ Address %d\n",address);
#endif
    if(nb_devices<MAX_DEVICES-1){
      Device *current=&(devices[nb_devices]);
      status=libusb_open(device,&(current->handle));
      if(status!=0){ perror("scan_for_launchers.libusb_open"); exit(-1); }
      current->id=id;
      current->nb_in=0;
      current->nb_out=0;
      nb_devices++;
#ifdef VERBOSE
      printf("We have now %d device(s).\n",nb_devices);
#endif
      }
    }
  }
libusb_free_device_list(list,1);
}

// Free devices from kernel

void free_devices(void){
int status;
int i,j;
for(i=0;i<nb_devices;i++){ 
  Device *current=&(devices[i]);
  libusb_device_handle *handle=current->handle;
#ifdef VERBOSE
  printf("Free device #%d\n",i);
#endif

  /* Get the configuration value */
  libusb_device *device=libusb_get_device(handle);
  struct libusb_config_descriptor *cfg_descriptor;
  status=libusb_get_active_config_descriptor (device,&cfg_descriptor);
  if(status!=0)
    { perror("free_devices.libusb_get_active_config_descriptor"); exit(-1); }
  int cfg_value=cfg_descriptor->bConfigurationValue;
#ifdef VERBOSE
  printf("  current configuration value is %d\n",cfg_value);
#endif

  /* Detach interfaces if needed */
  for(j=0;j<cfg_descriptor->bNumInterfaces;j++){
    const struct libusb_interface *altif_descriptor=cfg_descriptor->interface+j;
    const struct libusb_interface_descriptor *if_descriptor=
      altif_descriptor->altsetting;
    int if_number=if_descriptor->bInterfaceNumber;
    if(libusb_kernel_driver_active(handle,if_number)){
#ifdef VERBOSE
    printf("  detaching interface #%d from kernel\n",if_number);
#endif
      status=libusb_detach_kernel_driver(handle,if_number);
      if(status!=0)
        { perror("free_devices.libusb_detach_kernel_driver"); exit(-1); }
      }
    }

  libusb_free_config_descriptor(cfg_descriptor);
  }
}

// Configure devices
// (we use the first configuration and first alternate setting)

void configure_devices(void){
int status;
int i,j,k;
for(i=0;i<nb_devices;i++){ 
  Device *current=&(devices[i]);
  libusb_device_handle *handle=current->handle;
#ifdef VERBOSE
  printf("Configure device #%d\n",i);
#endif

  /* Get the configuration value */
  libusb_device *device=libusb_get_device(handle);
  struct libusb_config_descriptor *cfg_descriptor;
  status=libusb_get_config_descriptor(device,0,&cfg_descriptor);
  if(status!=0)
    { perror("configure_devices.libusb_get_config_descriptor"); exit(-1); }
  int cfg_value=cfg_descriptor->bConfigurationValue;
#ifdef VERBOSE
  printf("  using configuration value %d\n",cfg_value);
#endif

  /* Set the configuration */
  status=libusb_set_configuration(handle,cfg_value);
  if(status!=0)
    { perror("configure_devices.libusb_set_configuration"); exit(-1); }
#ifdef VERBOSE
  printf("  configuration of value %d selected\n",cfg_value);
#endif

  /* Claiming interfaces */
  for(j=0;j<cfg_descriptor->bNumInterfaces;j++){
    const struct libusb_interface *altif_descriptor=cfg_descriptor->interface+j;
    const struct libusb_interface_descriptor *if_descriptor=
      altif_descriptor->altsetting;
    int if_number=if_descriptor->bInterfaceNumber;
    status=libusb_claim_interface(handle,if_number);
    if(status!=0)
      { perror("configure_devices.libusb_claim_interface"); exit(-1); }
#ifdef VERBOSE
    printf("  interface #%d claimed\n",if_number);
#endif

    /* Find communication endpoints */
    for(k=0;k<if_descriptor->bNumEndpoints;k++){
      const struct libusb_endpoint_descriptor *ep_descriptor=
        if_descriptor->endpoint+k;
      int type=(ep_descriptor->bmAttributes & 0x3);
      int address=ep_descriptor->bEndpointAddress;
#ifdef VERBOSE
    printf("    endpoint found; type=%02x, address=%02x\n",type,address);
#endif
      unsigned char direction=(address & LIBUSB_ENDPOINT_IN);
      if(direction!=0){
        int nb=current->nb_in;
        current->in[nb].type=type;
        current->in[nb].address=address&(~LIBUSB_ENDPOINT_IN);
        current->nb_in++;
        }
      else{
        int nb=current->nb_out;
        current->out[nb].type=type;
        current->out[nb].address=address;
        current->nb_out++;
        }
      }
    }

  libusb_free_config_descriptor(cfg_descriptor);
  }
}

// Close all ressources

void close_devices(void){
int status;
int i,j;
for(i=0;i<nb_devices;i++){
  libusb_device_handle *handle=devices[i].handle;
#ifdef VERBOSE
  printf("Free ressources for devices #%d\n",i);
#endif

  /* Get the configuration value */
  libusb_device *device=libusb_get_device(handle);
  struct libusb_config_descriptor *cfg_descriptor;
  status=libusb_get_config_descriptor(device,0,&cfg_descriptor);
  if(status!=0)
    { perror("close_devices.libusb_get_config_descriptor"); exit(-1); }
  int cfg_value=cfg_descriptor->bConfigurationValue;
#ifdef VERBOSE
  printf("  using configuration value %d\n",cfg_value);
#endif

  /* Free interfaces */
  for(j=0;j<cfg_descriptor->bNumInterfaces;j++){
    const struct libusb_interface *altif_descriptor=cfg_descriptor->interface+j;
    const struct libusb_interface_descriptor *if_descriptor=
      altif_descriptor->altsetting;
    int if_number=if_descriptor->bInterfaceNumber;
    status=libusb_release_interface(handle,if_number);
    if(status!=0)
      { perror("close_devices.libusb_release_interface"); exit(-1); }
#ifdef VERBOSE
    printf("  interface #%d unclaimed\n",if_number);
#endif
    }

  /* Close device handle */
  libusb_close(handle);
  }
libusb_exit(context);
}

// Read interrupts from devices

unsigned char *get_interrupts_from_device(Device *current,int size){
int j,k,status;
libusb_device_handle *handle=current->handle;
int max=size<0?INTERRUPT_DATA_SIZE:size;
static unsigned char data[INTERRUPT_DATA_SIZE];
for(j=0;j<current->nb_in;j++){
  int type=current->in[j].type;
  if(type!=LIBUSB_TRANSFER_TYPE_INTERRUPT) continue;
  int endp=(current->in[j].address | LIBUSB_ENDPOINT_IN);
  int bytes_in;
#ifdef VERBOSE
  printf("  polling interrupt from ep #%02x\n",endp);
#endif
  status=libusb_interrupt_transfer(
           handle,endp,data,max,&bytes_in,INTERRUPT_TIMEOUT);
  if(status==0 && bytes_in>0){
#ifdef VERBOSE
    printf("  data from ep #%02x => ",endp);
    for(k=0;k<bytes_in;k++) printf("%02x ",data[k]);
    printf("\n");
#endif
    return data;
    }
  }
return NULL;
}

void get_interrupts_from_devices(int size){
int i;
for(i=0;i<nb_devices;i++){
  Device *current=&(devices[i]);
  printf("Interrupt(s) from device #%d\n",i);
  get_interrupts_from_device(current,size);
  }
}
