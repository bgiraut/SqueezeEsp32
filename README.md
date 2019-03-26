# SqueezeEsp32
Squeezebox player for ESP32 board and ESP8266

Use a vs1053 module or I2S DAC as audio output. 

* With the vs1053 module audio decoding is perform by the module. 
* With the I2S DAC the player use the library available here : https://github.com/earlephilhower/ESP8266Audio 

The configuration is define in the file config.h via some #define 

Wifi configuration is done via a AP started on the first boot. It use the library https://github.com/bbx10/WiFiManager

Once connected to the network, the player send multicast packet to discovery LMS. 


