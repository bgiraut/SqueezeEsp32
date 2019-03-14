#include "stRingBuffer.h"


stRingBuffer::stRingBuffer(uint32_t pBufferSize)
{
vcRBwindex = 0 ;                            // Fill pointer in ringbuffer
vcRBrindex = pBufferSize - 1 ;              // Emptypointer in ringbuffer
  
vcRingBufferSize = pBufferSize;
vcRBcount = 0;
vcRingBuffer = (uint8_t *) malloc ( vcRingBufferSize ) ; // Create ring buffer
}

stRingBuffer::~stRingBuffer()
{
free(vcRingBuffer), vcRingBuffer = 0;
}


uint32_t stRingBuffer::getBufferSize()
{
  return vcRingBufferSize;
}

void stRingBuffer::PrintRingBuffer(uint32_t pSize)
{
Serial.print("Array : ");
if(pSize > dataSize())
 pSize = dataSize();

 for(uint32_t i = 0; i < pSize; i++)
 {
   Serial.print((unsigned char) *(vcRingBuffer + vcRBrindex +i) , HEX);
 }
  
  Serial.println("");
}


void stRingBuffer::clearBuffer()
{
  vcRBwindex = 0 ;                      // Reset ringbuffer administration
  vcRBrindex = vcRingBufferSize - 1 ;
  vcRBcount = 0 ;
}

uint8_t stRingBuffer::readDataAt(uint16_t x)
{
return  *(vcRingBuffer + vcRBrindex  + x);  
}


/**
 * Return true if free space in the buffer for at least one byte
 */
bool stRingBuffer::isFreeSpace()
{
return ( vcRBcount < vcRingBufferSize ) ; 
}

/**
 * Return data size in the buffer
 */
uint32_t stRingBuffer::dataSize()
{
return vcRBcount;
}

/**
 * Push one byte of data to the ringbuffer
 */
void stRingBuffer::putData(uint8_t pData)
{
  // No check on available space.  See ringspace()
  *(vcRingBuffer + vcRBwindex) = pData ;         // Put byte in ringbuffer
  if ( ++vcRBwindex == vcRingBufferSize )      // Increment pointer and
  {
    vcRBwindex = 0 ;                    // wrap at end
  }
  vcRBcount++ ;                          // Count number of bytes in the  
}

/**
 * Retreive one byte of data from the ringbuffer
 */
uint8_t stRingBuffer::getData()
{
 if ( ++vcRBrindex == vcRingBufferSize )      // Increment pointer and
  {
    vcRBrindex = 0 ;                    // wrap at end
  }
  vcRBcount-- ;                          // Count is now one less
  return *(vcRingBuffer + vcRBrindex) ;      // return the oldest byte  
}

  
