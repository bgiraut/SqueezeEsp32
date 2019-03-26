#ifndef slimproto_h
#define slimproto_h

//#define ADAFRUIT_VS1053

#ifdef ESP32
  #define RINGBFSIZ 100000
#else
  #define RINGBFSIZ 20000
#endif

#include "config.h"
#include <Arduino.h>
#include "stRingBuffer.h"
    #include <math.h>
    
#ifdef ESP32
    #include <WiFi.h>
#else
    #include <ESP8266WiFi.h>	
#endif

#ifdef ADAFRUIT_VS1053
    #include <Adafruit_VS1053.h>
  #else
    #include <VS1053.h>
  #endif


struct __attribute__((packed)) StrmStructDef
{
   byte command;
   byte autostart;
   byte formatbyte;
   byte pcmsamplesize;
   byte pcmsamplerate;
   byte pcmchannels;
   byte pcmendian;
   byte threshold;
   byte spdif_enable;
   byte trans_period;
   byte trans_type;
   byte flags;
   byte output_threshold;
   byte RESERVED;
   u32_t replay_gain;
   byte server_port[2];
   byte server_ip[4];
   
};

struct __attribute__((packed)) audg_packet {
  char  opcode[4];
  u32_t old_gainL;     // unused
  u32_t old_gainR;     // unused
  u8_t  adjust;
  u8_t  preamp;        // unused
  u32_t gainL;
  u32_t gainR;
  // squence ids - unused
};


class responseBase
{
public : 
  responseBase(WiFiClient * pClient);
  ~responseBase();
  virtual void sendResponse() = 0;

protected :

struct stResponse{
                byte command[4];
                long sizeResponse  = 1;
                };

  WiFiClient * vcClient;
  stResponse  vcResponse;
};


class reponseHelo : public responseBase {
  
public : 
reponseHelo(WiFiClient * pClient);
void sendResponse();

private :

//                               H    E    L    0     {   SIZE          } {ID} {REV}  {  MAC ADDRESS              } {                   UUID                                                      }{   WLAN CHANNEL} { BYTE RECEIVED ???                   }{LANGUAGE}
// const unsigned char helo[] = {0x48,0x45,0x4c,0x4f ,0x00,0x00,0x00,0x24,0x08, 0x0b ,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00       ,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x45,0x4e};

/*
struct HELO_packet {
  char  opcode[4] = {0x48,0x45,0x4c,0x4f};
  u32_t length;
  u8_t  deviceid  = 0x08;
  u8_t  revision  = 0x0b;
  u8_t  mac[6] = {0x00,0x00,0x00,0x00,0x00,0x01};
  u8_t  uuid[16] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01};
  u16_t wlan_channellist  = 0;
  u32_t bytes_received_H, bytes_received_L = 0;
  char  lang[2] = {0x45,0x4e};
  //  u8_t capabilities[];
};

*/

struct __attribute__((packed)) stResponse
{
byte command[4] = {0x48,0x45,0x4c,0x4f};   // HELO
long sizeResponse ;
char diviceID = 0x08;
char firmwareRevision = 0x0b;
byte macAddr[6] = {0x00,0x00,0x00,0x00,0x00,0x01};
byte uuid[16] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01};
byte wlanChannel[2] = {0x00,0x00};
byte receivedData[8] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
byte language[2] = {0x45,0x4e};
char capabilites[92] = "Model=squeezeesp,ModelName=SqueezeEsp,Firmware=7,ogg,flc,pcm,mp3,SampleRate=44100,HasPreAmp";
};

stResponse  vcResponse;
};


class reponseSTAT : public responseBase {
  
public : 
reponseSTAT(WiFiClient * pClient);
void sendResponse();

private :

  
struct __attribute__((packed)) STAT_packet {
  char  opcode[4] = {0x53,0x54,0x41,0x54};   // STAT
  u32_t sizeResponse;
  //u32_t event;
  char  event[4];
  u8_t  num_crlf;
  u8_t  mas_initialized;
  u8_t  mas_mode;
  u32_t stream_buffer_size;
  u32_t stream_buffer_fullness;
  u32_t bytes_received_H;
  u32_t bytes_received_L;
  u16_t signal_strength;
  u32_t jiffies;
  u32_t output_buffer_size;
  u32_t output_buffer_fullness;
  u32_t elapsed_seconds;
  u16_t voltage;
  u32_t elapsed_milliseconds;
  u32_t server_timestamp;
  u16_t error_code;
};

public : 

STAT_packet  vcResponse;

};

typedef struct StrmStructDef StrmStruct;
typedef struct audg_packet AudgStruct;

  
class slimproto
{
public:
      #ifdef ADAFRUIT_VS1053
        slimproto(String pAdrLMS, WiFiClient pClient, Adafruit_VS1053 * pPlayer);
      #else
        slimproto(String pAdrLMS,WiFiClient * pClient, VS1053 * pPlayer);
      #endif

			
      slimproto(WiFiClient * pClient);
      ~slimproto();
			
			/**
			* Read message from socket and push them in a local buffer
			**/
			int HandleMessages();
     
      int HandleAudio();
			
private:


     stRingBuffer * vcRingBuffer;
     //stRingBuffer * vcCommandRingBuf;


      String vcAdrLMS; 

			int _pin;
			String vcBufferInput;

      int vcCommandSize;

       uint8_t*         ringbuf ;                                 // Ringbuffer for VS1053

      unsigned long StartTimeCurrentSong = 0;
      unsigned long EndTimeCurrentSong = 0;
      uint32_t      ByteReceivedCurrentSong = 0;

      unsigned long TimeCounter = 0;

      unsigned long LastStatMsg = 0;
      
			
			void HandleCommand(byte pCommand [], int pSize);
			void HandleStrmQCmd(byte pCommand [], int pSize);
			void HandleStrmTCmd(byte pCommand [], int pSize);
			void HandleStrmSCmd(byte pCommand [], int pSize);
			void HandleStrmPCmd(byte pCommand [], int pSize);
			void HandleStrmUCmd(byte pCommand [], int pSize);

      void HandleAudgCmd(byte pCommand [], int pSize);
      
			void ExtractCommand(byte * pBuf, int pSize);
			void ByteArrayCpy(byte * pDst, byte * pSrv, int pSize);

     void PrintByteArray(byte * psrc, int pSize);
     void PrintByteArray(String psrc, int pSize);
     void PrintHex8(uint8_t *data, uint8_t length);

     u32_t unpackN(u32_t *src);
     u16_t unpackn(u16_t *src);
      
		 WiFiClient * vcClient;            // Client to handle control messages
     WiFiClient vcStreamClient;      // Client to handle audio stream
     
     /*  
      #define VS1053_CS     D1 (1)
      #define VS1053_DCS    D0 (3)
      #define VS1053_DREQ   D3 (5) 
      */


      #ifdef ADAFRUIT_VS1053
        Adafruit_VS1053 * vcplayer;
      #else
         VS1053 * vcplayer;
      #endif

      enum player_status {
          StopStatus,
          PlayStatus,
          PauseStatus
        };


      player_status vcPlayerStat = StopStatus ;    /* 0 = stop , 1 = play , 2 = pause */

      

};
    
#endif
