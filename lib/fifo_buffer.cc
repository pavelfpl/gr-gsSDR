#include "fifo_buffer.h"

#define CONST_POWER_OF_TWO_ARRAY_L 12 // Define powersOfTwo length [12]
#define CONST_POWER_OF_TWO_ARRAY_D 1  // Define powersOfTwo default element [67108864]

const unsigned int powersOfTwo[CONST_POWER_OF_TWO_ARRAY_L] = {1048576,2097152,4194304,8388608,
                                     16777216,33554432,67108864,134217728,268435456,536870912,
                                     1073741824,2147483648};

// fifo_buffer constructor [public]
// --------------------------------
fifo_buffer::fifo_buffer(size_t bufferSize_, int buffer_type_){
    m_buffer_type = buffer_type_;

    m_fifoBufferStorage = 0;
    
    switch(m_buffer_type){
      case CONST_BUFFER_GR_WEB_STORAGE:
        m_fifoBufferStorage = new gr_storage_web[bufferSize_];  
        break;
      default:
        m_fifoBufferStorage = new gr_storage_web[bufferSize_]; break;
    }

    fifo_init(m_fifoBufferStorage, bufferSize_);
}

// fifo_buffer destructor [public]
// -------------------------------
fifo_buffer::~fifo_buffer(){

  switch(m_buffer_type){
    case CONST_BUFFER_GR_WEB_STORAGE:
      if(m_fifoBufferStorage != 0){ delete[] m_fifoBufferStorage; m_fifoBufferStorage = 0; } break;
    default:
      if(m_fifoBufferStorage != 0){ delete[] m_fifoBufferStorage; m_fifoBufferStorage = 0; } break;
  }
}

// isPowerOfTwo function [private]
// -------------------------------
unsigned int fifo_buffer::isPowerOfTwo(unsigned int x){

    int j=0;

    for (j=0;j<CONST_POWER_OF_TWO_ARRAY_L;j++){
         if(powersOfTwo[j] == x){
            return x;
         }
    }

    return powersOfTwo[CONST_POWER_OF_TWO_ARRAY_D];
}

// fifo_init function [private]
// ----------------------------
void fifo_buffer::fifo_init(gr_storage_web *buff_storage, size_t bufferSize_){

    m_fifo.fifoBufferStorage = buff_storage;
    m_fifo.fifoHead = 0;
    m_fifo.fifoTail = 0;
    m_fifo.fifoMask = bufferSize_-1;
    m_fifo.fifoSize = bufferSize_;
}

// fifo_changeSize function [public]
// ---------------------------------
void fifo_buffer::fifo_changeSize(size_t bufferSize_){

    size_t bufferSize;

    switch(m_buffer_type){
      case CONST_BUFFER_GR_WEB_STORAGE:
        if(m_fifoBufferStorage != 0){ delete[] m_fifoBufferStorage; m_fifoBufferStorage = 0; } break;
      default:
        if(m_fifoBufferStorage != 0){ delete[] m_fifoBufferStorage; m_fifoBufferStorage = 0; } break;
    }

    // bufferSize = isPowerOfTwo(bufferSize_);

    switch(m_buffer_type){
      case CONST_BUFFER_GR_WEB_STORAGE:
          m_fifoBufferStorage = new gr_storage_web[bufferSize_]; break;
      default:
          m_fifoBufferStorage = new gr_storage_web[bufferSize_]; break;
    }

    fifo_init(m_fifoBufferStorage, bufferSize_);

}


// gr_storage functions ...
// ------------------------
uint fifo_buffer::fifo_write_storage(const gr_storage_web *buff_storage, const uint nStorage){

  int j = 0;
  const gr_storage_web *p_gr = buff_storage;

  gr::thread::scoped_lock lock(fp_mutex);   // shared resources ...
  
  for(j=0;j<nStorage;j++){
      if(m_fifo.fifoHead+1 == m_fifo.fifoTail || ((m_fifo.fifoHead+1 == m_fifo.fifoSize) && (m_fifo.fifoTail==0))){
         return j;                                          // fifo buffer overflow - no more space to write ...
      }else{
         // uint fifoHeadTmp = m_fifo.fifoHead;
         m_fifo.fifoBufferStorage[m_fifo.fifoHead] =*p_gr++;
         m_fifo.fifoHead++;
         // m_fifo.fifoHead = (fifoHeadTmp & m_fifo.fifoMask);  // 
	if(m_fifo.fifoHead == m_fifo.fifoSize) {m_fifo.fifoHead = 0;}
      }
  }

  return nStorage;  // fifo write OK ...
}

uint fifo_buffer::fifo_read_storage(gr_storage_web *buff_storage,const uint nStorage){

  int j = 0;
  
  gr::thread::scoped_lock lock(fp_mutex);   // shared resources ...

  for(j=0;j<nStorage;j++){

      if(m_fifo.fifoHead == m_fifo.fifoTail ){
         return j; // no or more data to read ...
      }else{
         // uint fifoTailTmp = m_fifo.fifoTail;
         *buff_storage++ = m_fifo.fifoBufferStorage[m_fifo.fifoTail];
         m_fifo.fifoTail++;
         //m_fifo.fifoTail = (fifoTailTmp & m_fifo.fifoMask); // 
         if(m_fifo.fifoTail == m_fifo.fifoSize) {m_fifo.fifoTail = 0;}
      }
  }

  return nStorage;  // fifo read OK ..
}
