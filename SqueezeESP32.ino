#include <Arduino.h>

#ifdef ESP32
    #include <WiFi.h>
#else
    #include <ESP8266WiFi.h>
#endif

//#include <WiFiUdp.h>

#define VS1053_MODULE
#define ADAFRUIT_VS1053

#define I2S_DAC_MODULE

#define ADR_LMS "192.168.0.222"

#ifdef VS1053_MODULE
  #include "flac_plugin.h"


  #undef I2S_DAC_MODULE
  #ifdef ESP32
    #define VS1053_CS     5   // D1 // 5
    #define VS1053_DCS    16  // D0 // 16
    #define VS1053_DREQ   4   // D3 // 4
  #else
  // ESP8266
    #define VS1053_CS     D1 // 5
    #define VS1053_DCS    D0 // 16
    #define VS1053_DREQ   D3 // 4
  #endif
  #ifdef ADAFRUIT_VS1053
    #include <Adafruit_VS1053.h>
  #else
    #include <VS1053.h>
  #endif
#endif

#ifdef I2S_DAC_MODULE 
  #include "AudioFileSourceICYStream.h"
  #include "AudioFileSourceBuffer.h"
  #include "AudioGeneratorMP3.h"
  #include "AudioOutputI2SNoDAC.h"
#endif

#include "slimproto.h"


#ifdef VS1053_MODULE
  #ifdef ADAFRUIT_VS1053
    Adafruit_VS1053 viplayer(-1, VS1053_CS, VS1053_DCS, VS1053_DREQ);
  #else
    VS1053 viplayer(VS1053_CS, VS1053_DCS, VS1053_DREQ);
  #endif
#endif



#include <DNSServer.h>            //Local DNS Server used for redirecting all requests to the configuration portal
#ifdef ESP32
#include <WebServer.h> 
#else
#include <ESP8266WebServer.h>     //Local WebServer used to serve the configuration portal
#endif


#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager WiFi Configuration Magic

slimproto * vislimCli = 0;
WiFiClient client;

//WiFiUDP udp;

#ifdef VS1053_MODULE
void LoadPlugin(const uint16_t* plugin, uint16_t plugin_size)
{
int i = 0;
while (i<plugin_size)
  {
  uint16_t addr, n, val;
  addr = plugin[i++];
  n = plugin[i++];
  if (n & 0x8000U)
    { /* RLE run, replicate n samples */
    n &= 0x7FFF;
    val = plugin[i++];
    while (n--)
      {
      viplayer.sciWrite(addr, val);
      }
    }
  else
    { /* Copy run, copy n samples */
    while (n--)
      {
      val = plugin[i++];
      viplayer.sciWrite(addr, val);
      }
    }
  }
}

#endif //VS1053_MODULE
void setup()
{
  Serial.begin(115200);
  delay(1000);
  Serial.println("Connecting to WiFi");

  SPI.begin();
  #ifdef VS1053_MODULE
    viplayer.begin();
    #ifndef ADAFRUIT_VS1053
      viplayer.switchToMp3Mode();
      LoadPlugin(plugin,PLUGIN_SIZE);
      viplayer.setVolume(80);
    #else
      viplayer.begin();
      //viplayer.applyPatch(plugin,PLUGIN_SIZE);
      LoadPlugin(plugin,PLUGIN_SIZE);
      viplayer.setVolume(20,20);
    #endif
  #endif

  
  WiFiManager wifiManager;


  //reset saved settings
  //wifiManager.resetSettings();
  
  wifiManager.autoConnect("SqueezeEsp");

/*
  WiFi.disconnect();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_STA);
  
  WiFi.begin("ssid","wifipass");

  // Try forever
  while (WiFi.status() != WL_CONNECTED) {
    Serial.println("...Connecting to WiFi");
    delay(1000);
  }
  Serial.println("Connected");

*/
}





void loop()
{
//Send udp packet for autodiscovery
   // udp.beginPacket("255.255.255.255",3483);
   // udp.printf("e");
   // udp.endPacket();
    


  
Serial.println("Connecting to server...");

// DEBUG server 3484
// Real server 3483

 if (!client.connect(ADR_LMS, 3483)) {
      Serial.println("connection failed, pause and try connect...");
      delay(2000);
      return;
    }


if(vislimCli) delete vislimCli,vislimCli = 0;

 #ifdef VS1053_MODULE
  vislimCli = new slimproto(ADR_LMS, client, &viplayer);
 #else
  vislimCli = new slimproto(ADR_LMS, client);
 #endif

Serial.println("Connection Ok, send hello to LMS");
delay(2000);
reponseHelo *  HeloRsp = new reponseHelo(client);
HeloRsp->sendResponse();

while(client.connected())
  {
  vislimCli->HandleMessages();
  vislimCli->HandleAudio();
  }
}
