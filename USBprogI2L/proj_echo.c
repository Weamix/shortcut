/****                            ****/
/** Utility to handle echo device  **/
/****                            ****/

////
// Include files
////

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <termios.h>
#include <libusb-1.0/libusb.h>

#include "libusb.h"

////
// Constants
////

#define		TYPE_ECHO	0
#define		EP_SIZE		10
#define		DEFAULT_TIMEOUT	1000

////
// Structures
////

////
// Global variables
////

DeviceID ID_echo[]={
  {0x4242,0x0002,TYPE_ECHO},
  {-1,-1}
  };

////
// Fonctions
////

// Tests du périphèrique USB

int test_echo(void){
unsigned char token[EP_SIZE]={0xab};
unsigned char bytes[EP_SIZE];
if(nb_devices>0){
  if(devices[0].in[0].type!=LIBUSB_TRANSFER_TYPE_INTERRUPT) return -2;
  int in=devices[0].in[0].address;
  if(devices[0].out[0].type!=LIBUSB_TRANSFER_TYPE_INTERRUPT) return -3;
  int out=devices[0].out[0].address;
  int size=0,ressnd,resrcv,i;
  ressnd=libusb_interrupt_transfer(devices[0].handle,out,token,EP_SIZE,&size,DEFAULT_TIMEOUT);
  printf("Résultat envoi=%d taille envoyée=%d\n",ressnd,size);
  for(i=0;i<EP_SIZE;i++) printf("%02x ",token[i]);
  printf("\n");
  sleep(5);
  while(1){
    resrcv=libusb_interrupt_transfer(devices[0].handle,in|LIBUSB_ENDPOINT_IN,bytes,EP_SIZE,&size,DEFAULT_TIMEOUT);
    printf("Résultat réception=%d taille réception=%d\n",resrcv,size);
    for(i=0;i<EP_SIZE;i++) printf("%02x ",bytes[i]);
    printf("\n");
    sleep(1);
    }
  return 0;
  }
return -1;
}

int test_echo_v2(void){
unsigned char token[EP_SIZE]={0x01};
unsigned char bytes[EP_SIZE];
if(nb_devices>0){
  if(devices[0].in[0].type!=LIBUSB_TRANSFER_TYPE_INTERRUPT) return -2;
  int in=devices[0].in[0].address;
  if(devices[0].out[0].type!=LIBUSB_TRANSFER_TYPE_INTERRUPT) return -3;
  int out=devices[0].out[0].address;
  int size=0,ressnd,resrcv,i;
  while(1){
    ressnd=libusb_interrupt_transfer(devices[0].handle,out,token,EP_SIZE,&size,DEFAULT_TIMEOUT);
    printf("Résultat envoi=%d taille envoyée=%d\n",ressnd,size);
    for(i=0;i<EP_SIZE;i++) printf("%02x ",token[i]);
    printf("\n");
    sleep(1);
    resrcv=libusb_interrupt_transfer(devices[0].handle,in|LIBUSB_ENDPOINT_IN,bytes,EP_SIZE,&size,DEFAULT_TIMEOUT);
    printf("Résultat réception=%d taille réception=%d\n",resrcv,size);
    for(i=0;i<EP_SIZE;i++) printf("%02x ",bytes[i]);
    printf("\n");
    sleep(1);
    token[0]++;
    }
  return 0;
  }
return -1;
}

// Main procedure

int main(void){
scan_for_devices(ID_echo); 
configure_devices();
int result=test_echo_v2();
printf("Résultat du test : %d\n",result);
close_devices();
exit(0);
}
