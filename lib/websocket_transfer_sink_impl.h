/* -*- c++ -*- */
/*
 * Copyright 2020 Pavel Fiala, UwB, 2020.
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifndef INCLUDED_GSSDR_WEBSOCKET_TRANSFER_SINK_IMPL_H
#define INCLUDED_GSSDR_WEBSOCKET_TRANSFER_SINK_IMPL_H

#include <gsSDR/websocket_transfer_sink.h>
#include <gnuradio/thread/thread.h>
#include "fifo_buffer.h"

namespace gr {
  namespace gsSDR {

    class websocket_transfer_sink_impl : public websocket_transfer_sink
    {
     private:
     // Declare private variables ...
     static fifo_buffer m_fifo; 		        // Define FIFO buffer
     static bool m_exit_requested;
         
     std::string m_ServerName;
     std::string m_ServerPort;
     int m_stationId;
     std::string m_UserName;
     std::string m_UserPass;
     
     int m_packetCounter;
     
     gr::thread::thread _thread;
     boost::mutex fp_mutex;
     
     void threadTransferDeInit();

     public:
      websocket_transfer_sink_impl(const std::string &ServerName, const std::string &ServerPort, int stationId, const std::string &UserName, const std::string &UserPass);
      ~websocket_transfer_sink_impl();

      // Where all the action really happens ...
      void forecast (int noutput_items, gr_vector_int &ninput_items_required);
      
      void websocket_transfer_sink_wait();  // virtual reimplement ...
      // bool stop();
      
      int general_work(int noutput_items,
           gr_vector_int &ninput_items,
           gr_vector_const_void_star &input_items,
           gr_vector_void_star &output_items);
      
      void packet_handler(pmt::pmt_t msg);

    };

  } // namespace gsSDR
} // namespace gr

#endif /* INCLUDED_GSSDR_WEBSOCKET_TRANSFER_SINK_IMPL_H */

