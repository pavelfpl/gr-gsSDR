/*
 * File:   fifo_buffer.h
 * Author: Pavel Fiala 2015 - 2018, v 2.0 version adds gr_complex (complex) type ...
 * Designed for GnuRadio FIFO/FTDI FT232h or Altera MSGDMA sink / source
 */

#ifndef FIFO_BUFFER_H
#define FIFO_BUFFER_H

#include <sys/types.h>
#include <iostream>

#include <gnuradio/thread/thread.h>
#include <gsSDR/gr_storage_web.h>         // gr_storage_web type ...

#define CONST_BUFFER_GR_WEB_STORAGE 1     // gr_storage_web ...

struct fifo_t{
    // Using only one of these three buffers ...
    // -----------------------------------------
    gr_storage_web *fifoBufferStorage;

    uint fifoHead;
    uint fifoTail;
    uint fifoMask;
    size_t fifoSize;
};

class fifo_buffer{

public:
    fifo_buffer(size_t bufferSize_ = 16,int buffer_type_ = CONST_BUFFER_GR_WEB_STORAGE);
    ~fifo_buffer();
    void fifo_changeSize(size_t bufferSize_);
    uint fifo_getFifoHead() const {return m_fifo.fifoHead;}
    uint fifo_getFifoTail() const {return m_fifo.fifoTail;}
    uint fifo_getFifoSize() const {return m_fifo.fifoSize;}

    // ------------------------
    // gr_storage_web functions ...
    // ------------------------
    uint fifo_write_storage(const gr_storage_web *buff_storage, const uint nStorage);
    uint fifo_read_storage(gr_storage_web *buff_storage, const uint nStorage);
private:
    boost::mutex fp_mutex;
    
    int m_buffer_type;
    struct fifo_t m_fifo;
    
    gr_storage_web *m_fifoBufferStorage;

    unsigned isPowerOfTwo(unsigned int x);
    void fifo_init(gr_storage_web *buff_storage, size_t bufferSize_);
};

#endif /* FIFO_BUFFER_H */
