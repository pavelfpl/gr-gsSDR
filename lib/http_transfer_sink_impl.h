/* -*- c++ -*- */
/* MIT License
 *
 * Copyright (c) 2021 Pavel Fiala
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef INCLUDED_GSSDR_HTTP_TRANSFER_SINK_IMPL_H
#define INCLUDED_GSSDR_HTTP_TRANSFER_SINK_IMPL_H

#include <gsSDR/http_transfer_sink.h>
#include <gnuradio/thread/thread.h>
#include "fifo_buffer.h"

namespace gr {
  namespace gsSDR {

    class http_transfer_sink_impl : public http_transfer_sink
    {
     private:
     // Declare private variables ...
     static fifo_buffer m_fifo; 		        // Define FIFO buffer
     static bool m_exit_requested;
         
     std::string m_ServerName;
     std::string m_ServerPort;
     std::string m_ServerTarget;
     int m_stationId;
     int m_spacecraftId;
     std::string m_UserName;
     std::string m_UserPass;
     
     int m_packetCounter;
     
     gr::thread::thread _thread;
     boost::mutex fp_mutex;
     
     void threadTransferDeInit();

     public:
      http_transfer_sink_impl(const std::string &ServerName, const std::string &ServerPort,const std::string &ServerTarget, int stationId, int spacecraftId, const std::string &UserName, const std::string &UserPass);
      ~http_transfer_sink_impl();

      // Where all the action really happens ...
      void forecast (int noutput_items, gr_vector_int &ninput_items_required);

      void http_transfer_sink_wait();  
      bool stop();                    
    
      int general_work(int noutput_items,
           gr_vector_int &ninput_items,
           gr_vector_const_void_star &input_items,
           gr_vector_void_star &output_items);
        
      void packet_handler(pmt::pmt_t msg);
       
    };

  } // namespace gsSDR
} // namespace gr

#endif /* INCLUDED_GSSDR_HTTP_TRANSFER_SINK_IMPL_H */

