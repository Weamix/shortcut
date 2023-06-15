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

#define TYPE_ECHO 0
#define EP_OUT_SIZE 16
#define EP_IN_SIZE 1
#define DEFAULT_TIMEOUT 1000

////
// Structures
////

////
// Global variables
////

DeviceID ID_echo[] = {
    {0x4242, 0x0002, TYPE_ECHO},
    {-1, -1}};

////
// Fonctions
////

// Tests du périphèrique USB

unsigned char prompt_user_for_key(void)
{
  int c;
  printf("Press the key to configure...\n");
  scanf("%d", &c);
  return c;
}

unsigned char *prompt_user_for_string(void)
{
  char *s = malloc(15 * sizeof(char));
  printf("Enter a shortcut to bind to the key...\n");
  scanf("%s", s);
  for (int i = strlen(s); i < 15; i++)
  {
    s[i] = 0;
  }
  return s;
}

int test_echo_v2(void) {
    char key = prompt_user_for_key();
    char *s = prompt_user_for_string();
    if (strlen(s) > 15) {
        printf("Error: string too long\n");
        return -1;
    }
    unsigned char token[EP_OUT_SIZE] = {key, s[0], s[1], s[2], s[3], s[4], s[5], s[6], s[7], s[8], s[9], s[10], s[11],
                                        s[12], s[13], s[14]};
    unsigned char bytes[EP_IN_SIZE];
    printf("nb %d \n", nb_devices);
    if (nb_devices > 0) {
        if (devices[0].in[0].type != LIBUSB_TRANSFER_TYPE_INTERRUPT)
            return -2;
        int in = devices[0].in[0].address;
        if (devices[0].out[0].type != LIBUSB_TRANSFER_TYPE_INTERRUPT)
            return -3;
        int out = devices[0].out[0].address;
        int size = 0, ressnd, resrcv, i;
        ressnd = libusb_interrupt_transfer(devices[0].handle, out, token, EP_OUT_SIZE, &size, DEFAULT_TIMEOUT);
        printf("Résultat envoi=%d taille envoyée=%d\n", ressnd, size);
        for (i = 0; i < EP_OUT_SIZE; i++)
            printf("%02x ", token[i]);
        printf("\n");
        sleep(1);
        resrcv = libusb_interrupt_transfer(devices[0].handle, in | LIBUSB_ENDPOINT_IN, bytes, EP_IN_SIZE, &size,
                                           DEFAULT_TIMEOUT);
        printf("Résultat réception=%d taille réception=%d\n", resrcv, size);
        for (i = 0; i < EP_IN_SIZE; i++)
            printf("%02x ", bytes[i]);
        printf("\n");
        sleep(1);
        return 0;
    }
    return -1;
}

// Main procedure

int main(void)
{
  scan_for_devices(ID_echo);
  configure_devices();
  int result = test_echo_v2();
  printf("Résultat du test : %d\n", result);
  close_devices();
  exit(0);
}
