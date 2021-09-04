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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include "packed_to_unpacked_impl.h"
#include <gnuradio/blocks/pdu.h>

// -------------------
// System standard ...
// -------------------
#include <sys/types.h>
#include <sys/stat.h>
#include <stdexcept>
#include <cstdlib>
#include <limits>
#include <vector>
#include <time.h> 
#include <stdio.h>

namespace gr {
  namespace gsSDR {
      
    using namespace std;

    packed_to_unpacked::sptr
    packed_to_unpacked::make()
    {
      return gnuradio::get_initial_sptr
        (new packed_to_unpacked_impl());
    }


    /*
     * The private constructor ...
     * ---------------------------
     */
    packed_to_unpacked_impl::packed_to_unpacked_impl()
      : gr::block("packed_to_unpacked",
              gr::io_signature::make(0, 0, 0), 	 // gr::io_signature::make(<+MIN_IN+>, <+MAX_IN+>, sizeof(<+ITYPE+>)),
              gr::io_signature::make(0, 0, 0))   // gr::io_signature::make(<+MIN_OUT+>, <+MAX_OUT+>, sizeof(<+OTYPE+>)))
    {
        
       out_port = pmt::mp("pdus_out");
	  
       message_port_register_in(pmt::mp("pdus"));
       message_port_register_out(out_port); 	// pdu - data - complex float ... 
	  
       set_msg_handler(pmt::mp("pdus"), boost::bind(&packed_to_unpacked_impl::packet_handler, this, _1));  
    }

    /*
     * Our virtual destructor ...
     * --------------------------
     */
    packed_to_unpacked_impl::~packed_to_unpacked_impl(){
        
    }

    void packed_to_unpacked_impl::forecast (int noutput_items, gr_vector_int &ninput_items_required)
    {
      /* <+forecast+> e.g. ninput_items_required[0] = noutput_items */
    }

    int packed_to_unpacked_impl::general_work (int noutput_items,
                       gr_vector_int &ninput_items,
                       gr_vector_const_void_star &input_items,
                       gr_vector_void_star &output_items)
    {
      /*
      const <+ITYPE+> *in = (const <+ITYPE+> *) input_items[0];
      <+OTYPE+> *out = (<+OTYPE+> *) output_items[0];
      */
      // Do <+signal processing+>
      // Tell runtime system how many input items we consumed on
      // each input stream.
      consume_each (noutput_items);

      // Tell runtime system how many output items we produced.
      return noutput_items;
    }
    
    // packet_handler - public function / callback ...
    // -----------------------------------------------
    void packed_to_unpacked_impl::packet_handler(pmt::pmt_t msg){
      
        // Do <+signal processing+>
        // Get parameters from the pdu / unpacked bits - unsigned char ...
        // ---------------------------------------------------------------
        vector<unsigned char> data_packet(pmt::u8vector_elements(pmt::cdr(msg)));
        int m_packetLength = data_packet.size();
    
        pmt::pmt_t meta = pmt::make_dict();
    
        // Data unpacked ...
        // -----------------
        /* in 0b11110000 out 0b00000001 0b00000001 0b00000001 0b00000001 0b00000000 0b00000000 0b00000000 0b00000000
        * https://stackoverflow.com/questions/50977399/what-do-packed-to-unpacked-blocks-do-in-gnu-radio
        * */
	
        int unpackedLength = m_packetLength*8;  
        vector<unsigned char> data_unpacked(unpackedLength,0x00); // unpacked bytes ...
	   
        for (int i=0; i<unpackedLength; i++){
            data_unpacked[i] = (unsigned char)((data_packet.at(i/8) & (1 << (7 - (i % 8)))) != 0);
        }
	   
        pmt::pmt_t unpacked_vec = pmt::init_u8vector(unpackedLength, data_unpacked);
	    message_port_pub(out_port, pmt::cons(meta, unpacked_vec));
	}
    
  } /* namespace gsSDR */
} /* namespace gr */

