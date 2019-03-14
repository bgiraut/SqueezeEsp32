#ifndef stRingBuffer_h
#define stRingBuffer_h

#include <Arduino.h>

class stRingBuffer
{
public :

  stRingBuffer(uint32_t pBufferSize);
  ~stRingBuffer();

  
  void PrintRingBuffer(uint32_t pSize);

  bool isFreeSpace();
  uint32_t dataSize();
  
  uint32_t getBufferSize();
  uint8_t readDataAt(uint16_t x);

  void putData(uint8_t);
  uint8_t getData();

  void clearBuffer();

private :

 uint32_t vcRingBufferSize;         // Size of RingBuffer
 uint8_t* vcRingBuffer;             // RingBuffer
 uint32_t vcRBcount;                // Number of bytes in ringbuffer 

uint32_t         vcRBwindex = 0 ;                            // Fill pointer in ringbuffer
uint32_t         vcRBrindex = vcRingBufferSize - 1 ;                // Emptypointer in ringbuffer
  
};

#endif
