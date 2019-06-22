#include "slimproto.h"

//#define ADAFRUIT_VS1053

// Default volume
#define VOLUME  80


#ifdef I2S_DAC_MODULE
// Called when a metadata event occurs (i.e. an ID3 tag, an ICY block, etc.
void MDCallback(void *cbData, const char *type, bool isUnicode, const char *string)
{
  const char *ptr = reinterpret_cast<const char *>(cbData);
  (void) isUnicode; // Punt this ball for now
  // Note that the type and string may be in PROGMEM, so copy them to RAM for printf
  char s1[32], s2[64];
  strncpy_P(s1, type, sizeof(s1));
  s1[sizeof(s1)-1]=0;
  strncpy_P(s2, string, sizeof(s2));
  s2[sizeof(s2)-1]=0;
  Serial.printf("METADATA(%s) '%s' = '%s'\n", ptr, s1, s2);
  Serial.flush();
}

// Called when there's a warning or error (like a buffer underflow or decode hiccup)
void StatusCallback(void *cbData, int code, const char *string)
{
  const char *ptr = reinterpret_cast<const char *>(cbData);
  // Note that the string may be in PROGMEM, so copy it to RAM for printf
  char s1[64];
  strncpy_P(s1, string, sizeof(s1));
  s1[sizeof(s1)-1]=0;
  Serial.printf("STATUS(%s) '%d' = '%s'\n", ptr, code, s1);
  Serial.flush();
}

#endif //I2S_DAC_MODULE



responseBase::responseBase(WiFiClient * pClient)
  {
  vcClient = pClient;  
  }
  
responseBase::~responseBase()
  {
    
  }


/*
void responseBase::sendResponse()
{
  vcResponse.sizeResponse = sizeof(vcResponse);
  vcClient->write((char*) &vcResponse, sizeof(vcResponse));
}
*/


reponseHelo::reponseHelo(WiFiClient * pClient) : responseBase(pClient)
{ 
}

void reponseHelo::sendResponse()
{
  vcResponse.sizeResponse = __builtin_bswap32(sizeof(vcResponse) - 8); // N'inclus pas la commande ni la taille. 
  vcClient->write((char*) &vcResponse, sizeof(vcResponse));
}



reponseSTAT::reponseSTAT(WiFiClient * pClient) : responseBase(pClient)
{
// Clear the reponse struct
memset((void *) &vcResponse,'\0', sizeof(vcResponse));
}

void reponseSTAT::sendResponse()
{
  // Type of response -> STAT
  memcpy((void *) vcResponse.opcode, "STAT", 4);
  
  vcResponse.sizeResponse = __builtin_bswap32(sizeof(vcResponse) - 8); // N'inclus pas la commande ni la taille. 
  vcClient->write((char*) &vcResponse, sizeof(vcResponse)); 
}

/**
* Constructor for VS1053 version
**/

#ifdef VS1053_MODULE
#ifdef ADAFRUIT_VS1053
  slimproto::slimproto(String pAdrLMS, WiFiClient pClient, Adafruit_VS1053 * pPlayer)
#else
  slimproto::slimproto(String pAdrLMS, WiFiClient * pClient, VS1053 * pPlayer)
#endif
{
vcCommandSize = 0;

vcPlayerStat = StopStatus;    /* 0 = stop , 1 = play , 2 = pause */


u32_t viFreeMem = system_get_free_heap_size();

Serial.print("Free Memory for RingBuffer  : "), Serial.print(viFreeMem), Serial.println(" bytes");  

// Initialize the ringbuffer for audio
//vcRingBuffer = new stRingBuffer(RINGBFSIZ > viFreeMem ? viFreeMem-5000 : RINGBFSIZ); // if not enought memory for the ringbuffer use free memory minus 5ko
vcRingBuffer = new stRingBuffer(RINGBFSIZ); // if not enought memory for the ringbuffer use free memory minus 5ko
vcAdrLMS = pAdrLMS;
 
vcClient = pClient;	
vcplayer = pPlayer;

LastStatMsg = millis();
TimeCounter = millis();

EndTimeCurrentSong = StartTimeCurrentSong = 0;

}

#else

/**
* Constructor for DAC version
**/
slimproto::slimproto(String pAdrLMS, WiFiClient * pClient)
{
vcDacAudioGen = 0;
vcDacFile = 0;
vcDacBuff = 0;
vcDacOut = 0;
vcAdrLMS = pAdrLMS;
  
vcCommandSize = 0;

vcPlayerStat = StopStatus;    /* 0 = stop , 1 = play , 2 = pause */

vcClient = pClient;  

LastStatMsg = millis();
TimeCounter = millis();
EndTimeCurrentSong = StartTimeCurrentSong = 0;  
}

#endif //VS1053_MODULE

slimproto::~slimproto()
{
if(vcRingBuffer) delete vcRingBuffer, vcRingBuffer = 0;

#ifdef I2S_DAC_MODULE 
  if(vcDacAudioGen) delete vcDacAudioGen, vcDacAudioGen = 0;
  if(vcDacFile) delete vcDacFile, vcDacFile = 0;
  if(vcDacBuff) delete vcDacBuff, vcDacBuff = 0;  
  if(vcDacOut) delete vcDacOut,vcDacOut = 0;
#endif
}

int slimproto::HandleMessages()
{
uint8_t viBuffer;
int    viSizeRead;

 if(vcClient->connected())
  {
  if(vcCommandSize == 0 && vcClient->available() >= 2)
    {
    uint8_t  viExtractSize[2];

    vcClient->read(viExtractSize,2);

    // Convert string size into integer
    vcCommandSize = (viExtractSize[0] << 8) | viExtractSize[1];

    if(vcCommandSize != 172)
      Serial.print("Expected command size : "), Serial.println(vcCommandSize);  
    }


  if(vcCommandSize > 250)
    {
    uint8_t availableSize = vcClient->available();
      
    Serial.println("Expected command size to big ??!!??");
    Serial.print("Available size : "),Serial.println(availableSize);
    uint8_t  viExtractCommand[availableSize];
    viSizeRead =  vcClient->read(viExtractCommand,availableSize);
    PrintByteArray(viExtractCommand,viSizeRead );
    vcCommandSize = 0;
    }


  if(vcCommandSize && vcClient->available() >= vcCommandSize)
    {
    uint8_t  viExtractCommand[vcCommandSize]; 

    viSizeRead =  vcClient->read(viExtractCommand,vcCommandSize); 

    if(viSizeRead != vcCommandSize)
      {
      Serial.println("Not enought data as expected !!!"); 
      }

    if(vcCommandSize != 172)
      PrintByteArray(viExtractCommand,viSizeRead);

    HandleCommand(viExtractCommand,viSizeRead); 
    vcCommandSize = 0; 
    }
  }

  // Send Stat Message if last one is more than 60 seconds
if(millis() - LastStatMsg >= ( 60 * 1000))
   {
   Serial.println("No Stat request from 60 seconds, Is there any probem ?");
    
   //reponseSTAT viResponse(vcClient);
   //viResponse.vcResponse.elapsed_seconds = 0;
   //viResponse.sendResponse();
   //LastStatMsg = millis();
   return false;
   } 

return true;
}



#ifdef VS1053_MODULE

int slimproto::HandleAudio() // Handle audio in vs1053 mode
{
// Lire des données du stream et les envoyer dans le vs1053      
uint32_t viRead;
__attribute__((aligned(4))) uint8_t buf[32] ; // Buffer for chunk
 
if(vcStreamClient.connected())
  {
  viRead =  vcStreamClient.available() ;

   while (vcRingBuffer->isFreeSpace() && viRead-- )
      {
      vcRingBuffer->putData(vcStreamClient.read()) ;                // Yes, store one byte in ringbuffer
      yield() ;
      }
        
  if(millis() - TimeCounter >= ( 5 * 1000))
   {
   TimeCounter = millis();
   Serial.print("Audio : RingBuffer Size : ");
   Serial.print(vcRingBuffer->dataSize());
   Serial.print(" / ");
   Serial.println(vcRingBuffer->getBufferSize());
   }
  }
 else
  {
  // End of stream
  
  if(vcPlayerStat == PlayStatus)
    {
    vcPlayerStat = PauseStatus;    /* 0 = stop , 1 = play , 2 = pause */

    EndTimeCurrentSong = millis();

    // Stream buffer empty
    reponseSTAT viResponse(vcClient);
    memcpy((void *) viResponse.vcResponse.event, "STMd", 4);
    viResponse.vcResponse.bytes_received_L = ByteReceivedCurrentSong;

    viResponse.vcResponse.elapsed_seconds =  (EndTimeCurrentSong - StartTimeCurrentSong)/ 1000;
    viResponse.sendResponse();

    reponseSTAT viResponseU(vcClient);
    memcpy((void *) viResponseU.vcResponse.event, "STMu", 4);
    viResponseU.vcResponse.bytes_received_L = ByteReceivedCurrentSong;

    viResponseU.vcResponse.elapsed_seconds =  (EndTimeCurrentSong - StartTimeCurrentSong)/ 1000;
    viResponseU.sendResponse();
    }

  if(millis() - TimeCounter >= ( 5 * 1000))
   {
   TimeCounter = millis();
   Serial.println("Audio : Streaming not connected");
   }  
  }

/*
if(vcStreamClient.connected())
  {
   if(vcplayer->readyForData())
     Serial.println("VS1053 ready for data");
  }
*/
  // Send data to the vs1053 if available
  #ifdef ADAFRUIT_VS1053
    while(vcplayer->readyForData() && vcRingBuffer->dataSize()> 0) // Try to keep VS1053 filled
  #else
    while(vcplayer->data_request() && vcRingBuffer->dataSize()> 0) // Try to keep VS1053 filled
  #endif
      {
      int viByteRetrieve = 0;
      
      // Get Max 32 byte of data from the RingBuffer
      while(vcRingBuffer->dataSize() && viByteRetrieve < 32 )
        {
        buf[viByteRetrieve++] =  vcRingBuffer->getData();
        }

      ByteReceivedCurrentSong += viByteRetrieve;
     
      #ifdef ADAFRUIT_VS1053
        vcplayer->playData (buf, viByteRetrieve) ;
      #else
        vcplayer->playChunk (buf, viByteRetrieve) ;
      #endif

      yield() ;
      } 
}
#else // Handle Audio in DAC mode
int slimproto::HandleAudio()
{
if(vcDacAudioGen && vcPlayerStat != PauseStatus)
  if(vcDacAudioGen->isRunning()){
  if(!vcDacAudioGen->loop())  vcDacAudioGen->stop();
  }   
}

#endif // VS1053_MODULE
/**
 * Stop command
 */

void slimproto::HandleStrmQCmd(byte pCommand [], int pSize)
{
#ifdef VS1053_MODULE 
  #ifndef ADAFRUIT_VS1053
    vcplayer->stopSong();
  #endif
#else
  if(vcDacAudioGen)
    vcDacAudioGen->stop();
    
  if(vcDacAudioGen) delete vcDacAudioGen, vcDacAudioGen = 0;
  if(vcDacOut) delete vcDacOut, vcDacOut = 0;
  if(vcDacBuff) delete vcDacBuff, vcDacBuff = 0;
  if(vcDacFile) delete vcDacFile, vcDacFile = 0;

#endif //VS1053_MODULE
  
reponseSTAT * viResponse = 0;
  
  vcPlayerStat = StopStatus;
  EndTimeCurrentSong = millis();
 
  viResponse = new reponseSTAT(vcClient);
  memcpy((void *) viResponse->vcResponse.event, "STMf", 4);

  viResponse->vcResponse.elapsed_seconds =  0;
  
  // Send stop confirm
  viResponse->sendResponse(); 
}

/**
 * Status command
 * 
 */
void slimproto::HandleStrmTCmd(byte pCommand [], int pSize)
{
StrmStruct strmInfo;
memcpy(&strmInfo, pCommand+4, sizeof(strmInfo));

  
reponseSTAT viResponse(vcClient);
memcpy((void *) viResponse.vcResponse.event, "STMt", 4);
viResponse.vcResponse.bytes_received_L = ByteReceivedCurrentSong;

viResponse.vcResponse.server_timestamp = strmInfo.replay_gain;



// If current stat is 'stop' send 0 as elapsed time
if(vcPlayerStat == StopStatus)
  viResponse.vcResponse.elapsed_seconds = 0;

// else elapsed time of the current song
else
  viResponse.vcResponse.elapsed_seconds =  (millis() - StartTimeCurrentSong)/ 1000;


viResponse.sendResponse();
LastStatMsg = millis();

//debug 
PrintByteArray((byte *)&viResponse.vcResponse, sizeof(viResponse.vcResponse));

//vcClient->write(staT, sizeof(staT));	
}


/**
* Handle start command
*/

void slimproto::HandleStrmSCmd(byte pCommand [], int pSize)
{
char viUrl[pSize +100];
char viUrl2[pSize +100];
byte viTmpUrl[pSize +100];


memset(viUrl,0,sizeof(viUrl));
memset(viUrl2,0,sizeof(viUrl2));
memset(viTmpUrl,0,sizeof(viTmpUrl));
  
  
StrmStruct strmInfo;
memcpy(&strmInfo, pCommand+4, sizeof(strmInfo));

// Format MP3
if(strmInfo.formatbyte == 'm')
{
Serial.println("Format MP3"); 

#ifdef I2S_DAC_MODULE
vcDacAudioGen = new AudioGeneratorMP3(); 
vcDacAudioGen->RegisterStatusCB(StatusCallback, (void*)"mp3");
#endif //I2S_DAC_MODULE

}
// Format FLAC
else if(strmInfo.formatbyte == 'f')
{
Serial.println("Format flac"); 

#ifdef I2S_DAC_MODULE
vcDacAudioGen = new AudioGeneratorFLAC(); 
vcDacAudioGen->RegisterStatusCB(StatusCallback, (void*)"flac");
#endif //I2S_DAC_MODULE

}
else if(strmInfo.formatbyte == '0')
{
Serial.println("Format ogg"); 
#ifdef I2S_DAC_MODULE
  Serial.println("Ogg format not supported in DAC mode"); 
  return;
#endif //I2S_DAC_MODULE
}


ByteArrayCpy(viTmpUrl,pCommand+sizeof(strmInfo)+4 , pSize-sizeof(strmInfo)-4);
if(strmInfo.server_ip[0] == 0)
  {
  #ifdef VS1053_MODULE
    vcAdrLMS.toCharArray(viUrl, pSize);
  #else
    vcAdrLMS.toCharArray(viUrl2, pSize);
    sprintf(viUrl,"http://%s:9000/%s",viUrl2,"stream.mp3"); 
  #endif //VS1053_MODULE
  }
 
else
  {
  #ifdef VS1053_MODULE
  sprintf(viUrl, "%d.%d.%d.%d",strmInfo.server_ip[0],strmInfo.server_ip[1],strmInfo.server_ip[2],strmInfo.server_ip[3]); 
  #else
    sprintf(viUrl, "http://%d.%d.%d.%d:9000%s",strmInfo.server_ip[0],strmInfo.server_ip[1],strmInfo.server_ip[2],strmInfo.server_ip[3],(char*) viTmpUrl); 
  #endif //VS1053_MODULE
  }

Serial.print("Url : "); 
Serial.println(viUrl); 


Serial.println("Let's play music");

#ifdef VS1053_MODULE
 #ifndef ADAFRUIT_VS1053
  vcplayer->startSong();
 #else
  //vcplayer-> begin();
  //vcplayer->softReset();
 #endif //ADAFRUIT_VS1053

#else // Mode DAC
  vcDacFile = new AudioFileSourceICYStream((char *) viUrl); 
  vcDacFile->RegisterMetadataCB(MDCallback, (void*)"ICY");

  // Use 8k of audio buffer
  vcDacBuff = new AudioFileSourceBuffer(vcDacFile, 4096);
  vcDacBuff->RegisterStatusCB(StatusCallback, (void*)"buffer");
  
  vcDacOut = new AudioOutputI2S();
  vcDacAudioGen->begin(vcDacBuff, vcDacOut); 
  vcDacAudioGen->loop();
#endif //VS1053_MODULE

 

  // on flag l'état à 'lecture'
  vcPlayerStat  = PlayStatus;

  // Starting time
  StartTimeCurrentSong = millis();

  ByteReceivedCurrentSong = 0;

#ifdef VS1053_MODULE
  Serial.println("connecting to stream... ");
      
    if (!vcStreamClient.connect(viUrl, 9000)) {
      Serial.println("Connection failed");
      return;
    }
  Serial.println("Connexion ok"); 
  Serial.print("Ask for : "),Serial.println(String((char*) viTmpUrl));
  vcStreamClient.print(String("") + String((char*) viTmpUrl) + "\r\n" +
                  "Host: " + viUrl + "\r\n" + 
                  "Connection: close\r\n\r\n");
#endif VS1053_MODULE

                  
// Send connect

reponseSTAT viResponseSTMc(vcClient);
memcpy((void *) viResponseSTMc.vcResponse.event, "STMc", 4);
viResponseSTMc.sendResponse();
//vcClient->write(staC, sizeof(staC));	
// Send connected 	

reponseSTAT viResponseSTMe(vcClient);
memcpy((void *) viResponseSTMe.vcResponse.event, "STMe", 4);
viResponseSTMe.sendResponse();
//vcClient->write(staE, sizeof(staE));		

// Send HTTP headers received from stream connection
reponseSTAT viResponseSTMh(vcClient);
memcpy((void *) viResponseSTMh.vcResponse.event, "STMh", 4);
viResponseSTMh.sendResponse();
//vcClient->write(staH, sizeof(staH));	

// Send Track Started
reponseSTAT viResponseSTMs(vcClient);
memcpy((void *) viResponseSTMs.vcResponse.event, "STMs", 4);
viResponseSTMs.sendResponse();
}

/**
* Handle pause command
*/ 
void slimproto::HandleStrmPCmd(byte pCommand [], int pSize)
{ 
// Send Pause confirm
reponseSTAT viResponseSTMp(vcClient);
memcpy((void *) viResponseSTMp.vcResponse.event, "STMp", 4);
viResponseSTMp.sendResponse();

vcPlayerStat = PauseStatus;
}

/**
* Handle unpause command
*/

void slimproto::HandleStrmUCmd(byte pCommand [], int pSize)
{
//vcplayer->pausePlaying(false);

// Send UnPause confirm
reponseSTAT viResponseSTMp(vcClient);
memcpy((void *) viResponseSTMp.vcResponse.event, "STMr", 4);
viResponseSTMp.sendResponse();

vcPlayerStat = PlayStatus;
}

/**
 * Handle volum control
 */
void slimproto::HandleAudgCmd(byte pCommand [], int pSize)
{
#ifdef VS1053_MODULE
  audg_packet viVolDef;
  memcpy(&viVolDef, pCommand, sizeof(audg_packet));
  u32_t viVol = unpackN((u32_t*) (pCommand+14)) ;
  
  Serial.print("Volume : ");  
  Serial.println(viVol); 
  u32_t  newvolume = ((100 * log10((viVol*100)/65))/5);
  Serial.print("new volume  : ");  
  Serial.println(newvolume);

  #ifdef ADAFRUIT_VS1053
     vcplayer->setVolume((viVol  *100) / 65536,(viVol  *100) / 65536);
  #else
    vcplayer->setVolume(newvolume);
  #endif //ADAFRUIT_VS1053
#else
  Serial.println("Sorry, no volume control in DAC mode");
#endif // VS1053_MODULE
}


void slimproto::HandleCommand(byte pCommand[], int pSize)
{
byte viCommand[5] = {0};

ByteArrayCpy(viCommand, pCommand,4);

//Serial.print("Handle command : ");
//Serial.println((char *) viCommand);	

if(strcmp((const char *)viCommand,"strm") == 0)
	{
	//Serial.println("strm command");
	unsigned char viSubCmd;
	
	viSubCmd = (unsigned char) pCommand[4];
	
	Serial.print("Sub command : ");
	Serial.println(viSubCmd);
	switch (viSubCmd) {
         case 'q':
             Serial.println("Sub command q (stop)");
			 HandleStrmQCmd(pCommand,pSize );
             break;			
		 case 't':
             Serial.println("Sub command t (status)");
			 HandleStrmTCmd(pCommand,pSize );
             break;
		case 'p':
             Serial.println("Sub command p (pause)");
			 HandleStrmPCmd(pCommand,pSize );
             break;	 
		case 'u':
             Serial.println("Sub command u (unpause)");
			 HandleStrmUCmd(pCommand,pSize );
             break;				
		case 's':
             Serial.println("Sub command s (start)");
			 HandleStrmSCmd(pCommand,pSize );
             break;	 
         default:
             Serial.println("default SubCommand");
      }
	}
else if (strcmp((const char *)viCommand,"vfdc") == 0)
	{
	//Serial.println("vfdc command");
	}
else if (strcmp((const char *)viCommand,"audg") == 0)
	{
	Serial.println("audg command");
  HandleAudgCmd(pCommand,pSize);
 
	}
else	
	{
	Serial.print("Autre command : [");
	Serial.print((char*)viCommand);
  Serial.println("]");
  Serial.print("Size : ");
  Serial.println(pSize);  
	} 
}

void slimproto::ExtractCommand(byte * pBuf, int pSize)
{
for(int i = 0; i < pSize; i++)
	{
	pBuf[i]= vcBufferInput[i];
	}
	
vcBufferInput.remove(0,pSize);	
}

void slimproto::ByteArrayCpy(byte * pDst, byte * pSrv, int pSize)
{
for(int i = 0; i < pSize; i++)
	{
	pDst[i]= pSrv[i];
	}
}


u32_t slimproto::unpackN(u32_t *src) {
  u8_t *ptr = (u8_t *)src;
  return *(ptr) << 24 | *(ptr+1) << 16 | *(ptr+2) << 8 | *(ptr+3);
} 

u16_t slimproto::unpackn(u16_t *src) {
  u8_t *ptr = (u8_t *)src;
  return *(ptr) << 8 | *(ptr+1);
} 


void slimproto::PrintByteArray(byte * psrc, int pSize)
{
  
 Serial.print("Array(");
 Serial.print(pSize);
 Serial.print(") : ");

     char tmp[16];
       for (int i=0; i<pSize; i++) {
         sprintf(tmp, "%.2X",psrc[i]);
         Serial.print(tmp); Serial.print(" ");
       }
  
  Serial.println("");
}

void slimproto::PrintHex8(uint8_t *data, uint8_t length) // prints 8-bit data in hex with leading zeroes
{
 Serial.print("Array(");
 Serial.print(length);
 Serial.print(") : ");
 
  
     char tmp[16];
       for (int i=0; i<length; i++) {
         sprintf(tmp, "%.2X",data[i]);
         Serial.print(tmp); Serial.print(" ");
       }
 Serial.println("");
}

void slimproto::PrintByteArray(String psrc, int pSize)
{
  
 Serial.print("Array : ");

 for(int i = 0; i < pSize; i++)
 {
   Serial.print(psrc[i], HEX);
 }
  
  Serial.println("");
}
