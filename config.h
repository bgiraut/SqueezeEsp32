#ifndef config_h
#define config_h

#include <Ethernet.h>
#include <EthernetUdp.h>


#define VS1053_MODULE
//#define ADAFRUIT_VS1053

//#define I2S_DAC_MODULE

#define DEFAULT_LMS_ADDR "192.168.0.222"

#define UPD_PORT 3483

static IPAddress LMS_addr(0,0,0,0);

#endif
