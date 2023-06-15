/****                                     ****/
/** Definitions for USB device management   **/
/****                                     ****/

////
// Constants
////

#define MAX_DEVICES		128
#define MAX_ENDPOINTS		16

#define INTERRUPT_TIMEOUT	1000
#define CONTROL_TIMEOUT		1000

////
// Structures
////

typedef struct{
  int vendor;
  int product;
  int type;
  } DeviceID;

typedef struct{
  unsigned char type;
  unsigned char address;
  } Endpoint;

typedef struct{
  DeviceID *id;
  libusb_device_handle *handle;  
  Endpoint in[MAX_ENDPOINTS];
  Endpoint out[MAX_ENDPOINTS];
  int nb_in;
  int nb_out;
  } Device;

////
// Global variables
////

extern Device devices[];
extern int nb_devices;

////
// Prototypes
////

void scan_for_devices(DeviceID *devids);
void free_devices(void);
void configure_devices(void);
void close_devices(void);
unsigned char *get_interrupts_from_device(Device *current,int size);
void get_interrupts_from_devices(int size);
