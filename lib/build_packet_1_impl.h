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

#ifndef INCLUDED_PACKETDECODERSDR_BUILD_PACKET_1_IMPL_H
#define INCLUDED_PACKETDECODERSDR_BUILD_PACKET_1_IMPL_H

#include <gsSDR/build_packet_1.h>
#include <fstream>

namespace gr {
  namespace gsSDR {

    class build_packet_1_impl : public build_packet_1
    {
     private:
       boost::mutex fp_mutex;
       std::ifstream fileToTransfer;
       
       pmt::pmt_t out_port_0;
       pmt::pmt_t out_port_1;
       
       bool m_appendHeader;	 // appendHeader - append header to beginning of the packet ...
       int m_packetLength;	 // burst packet length / including header ...
       int m_dataType;		 // output data type - packed or unpacked ...
       int m_dataFrom;		 // dataFrom - fromFile or random / for testing ...
       unsigned long m_fileSize;
       unsigned long m_fileIncrement;
       
       void fileOpen(const char *filename);
       void fileClose();
       
       std::vector<unsigned char> packet_bytes; 
     public:
      build_packet_1_impl(bool appendHeader, int packetLength, int dataType, int dataFrom, const char *filename, std::vector<unsigned char> packet_bytes_h);
      ~build_packet_1_impl();

      // Where all the action really happens
      void forecast (int noutput_items, gr_vector_int &ninput_items_required);

      int general_work(int noutput_items,
		       gr_vector_int &ninput_items,
		       gr_vector_const_void_star &input_items,
		       gr_vector_void_star &output_items);
      
      void packet_handler(pmt::pmt_t msg);
    };

  } // namespace gsSDR
} // namespace gr

#endif /* INCLUDED_PACKETDECODERSDR_BUILD_PACKET_1_IMPL_H */
