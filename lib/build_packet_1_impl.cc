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

#include "build_packet_1_impl.h"

// ---------------------
// GNURADIO specific ...
// ---------------------
#include <gnuradio/io_signature.h>
#include <gnuradio/blocks/pdu.h>
#include <gnuradio/thread/thread.h>
#include <gnuradio/digital/crc32.h>

// -------------------
// System standard ...
// -------------------
#include <sys/types.h>
#include <sys/stat.h>
#include <stdexcept>
#include <cstdlib>
#include <limits>
#include <time.h> 
#include <stdio.h>

// --------------
// My DEFINES ...
// --------------
#define CONST_DATA_FROM_RANDOM_GENERATOR 0
#define CONST_DATA_FROM_FILE 1
#define CONST_DATA_CMD_LINE 2

#define CONST_DATA_PACKED 0 
#define CONST_DATA_UNPACKED 1

#define CONST_HEADER_PRELOAD_BYTES 2
#define CONST_HEADER_PACKET_SIZE 2
#define CONST_HEADER_CRC32 4
#define CONST_PAYLOAD_CRC32 4

#define CONST_TRANSFER_IN_PROGRESS 0
#define CONST_TRANSFER_READY 1

#define CONST_FILE_READ_OK 0
#define CONST_FILE_READ_FAILED 1

// ------------------------------------------------------------------------------------------------------
// !!!  ADD DIGITAL component to ROOT CMakeList.txt - set(GR_REQUIRED_COMPONENTS RUNTIME FFT DIGITAL) !!!
// ------------------------------------------------------------------------------------------------------

namespace gr {
  namespace gsSDR {
    
    using namespace std;
    
    build_packet_1::sptr
    build_packet_1::make(bool appendHeader, int packetLength, int dataType, int dataFrom, const char *filename, std::vector<unsigned char> packet_bytes_h)
    {
      return gnuradio::get_initial_sptr
        (new build_packet_1_impl(appendHeader, packetLength, dataType, dataFrom, filename, packet_bytes_h));
    }

    /*
     * The private constructor ...
     * ---------------------------
     */
    build_packet_1_impl::build_packet_1_impl(bool appendHeader, int packetLength, int dataType, int dataFrom, const char *filename, std::vector<unsigned char> packet_bytes_h)
      : gr::block("build_packet_1",
              gr::io_signature::make(0, 0, 0), 	 // gr::io_signature::make(<+MIN_IN+>, <+MAX_IN+>, sizeof(<+ITYPE+>)),
              gr::io_signature::make(0, 0, 0)),  // gr::io_signature::make(<+MIN_OUT+>, <+MAX_OUT+>, sizeof(<+OTYPE+>)))
              m_appendHeader(appendHeader),	 // appendHeader - append header to beginning of the packet
              m_packetLength(packetLength),	 // burst packet length / including header
              m_dataType(dataType),		 // output data type - packed or unpacked
              m_dataFrom(dataFrom)		 // dataFrom - fromFile or random / for testing 
              
    {
	  
	  if(packetLength == 0) packetLength = 64; // Set default vector [ packet size ]... 
	    
	  if(m_dataFrom == CONST_DATA_FROM_FILE){
	     fileOpen(filename);
	  }
	  
	  // Set packet bytes uchar ...
	  // --------------------------
	  packet_bytes = packet_bytes_h;
	  
	  // Initialize random seed ...
	  // --------------------------
	  srand(time(NULL));
      
	  out_port_0 = pmt::mp("stat_pdus");
      	  out_port_1 = pmt::mp("out_pdus");
	  
	  message_port_register_in(pmt::mp("pdus"));
	  message_port_register_out(out_port_0); // pdu - status ... 
	  message_port_register_out(out_port_1); // pud - vectors ...
	  
	  set_msg_handler(pmt::mp("pdus"), boost::bind(&build_packet_1_impl::packet_handler, this, _1));
      
    }
    
    // fileOpen - private function ...
    // -------------------------------
    void build_packet_1_impl::fileOpen(const char *filename){
	
        // obtain exclusive access for duration of this function ...
	// ---------------------------------------------------------
	gr::thread::scoped_lock lock(fp_mutex);
	
	struct stat results;
	
	// The size of the file in bytes is in results.st_size ...
	// -------------------------------------------------------
	if(stat(filename, &results) != 0){
	   perror(filename);
	   throw std::runtime_error("File status failed");
	}
	
	// Set file size ...
	// -----------------
	m_fileSize = results.st_size;
        m_fileIncrement = 0;
	
	fileToTransfer.open(filename, ios::in | ios::binary);
    }
    
    // fileClose - private function ...
    // --------------------------------
    void build_packet_1_impl::fileClose(){
	// Close file [if opened] ...
	// --------------------------
	if(fileToTransfer.is_open())
	   fileToTransfer.close();
    }
    
    /*
     * Our virtual destructor.
     * -----------------------
     */
    build_packet_1_impl::~build_packet_1_impl(){
	fileClose();
    }

    void
    build_packet_1_impl::forecast (int noutput_items, gr_vector_int &ninput_items_required)
    {
        /* <+forecast+> e.g. ninput_items_required[0] = noutput_items */
    }

    int
    build_packet_1_impl::general_work (int noutput_items,
                       gr_vector_int &ninput_items,
                       gr_vector_const_void_star &input_items,
                       gr_vector_void_star &output_items)
    {
        /*
	const <+ITYPE*> *in = (const <+ITYPE*> *) input_items[0];
        <+OTYPE*> *out = (<+OTYPE*> *) output_items[0];
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
    void build_packet_1_impl::packet_handler(pmt::pmt_t msg){
	
	int offset = 0;
	int extraOffset = 0; 					// Valid only for file read transfer ...
	int payload_length = m_packetLength;
	
	// Get parameters from the pdu / or make dict ...
	// ----------------------------------------------
	// pmt::pmt_t meta = pmt::car(msg);
	pmt::pmt_t meta = pmt::make_dict();
	
	// Create vector of given length - m_packetLength / packed ...
	// -----------------------------------------------------------
	vector<unsigned char> data_packet(m_packetLength,0x00); // Set default values ...
	
	// Generate header of the packet / PRE  ...
	// ----------------------------------------
	if(m_appendHeader && m_dataFrom != CONST_DATA_CMD_LINE){
	   *(uint16_t*)&data_packet[0] = 0x1337;					// Header == 0x1337 [2 bytes]
	   *(uint16_t*)&data_packet[2] = m_packetLength - CONST_HEADER_PRELOAD_BYTES - CONST_HEADER_PACKET_SIZE
				- CONST_HEADER_CRC32 - CONST_PAYLOAD_CRC32; 		// [2 bytes]
	   *(uint32_t*)&data_packet[4] = gr::digital::crc32(&data_packet[0],4);	    	// [4 bytes]	
	   
	   // Add header offset ...
	   // ---------------------
	   offset+=8; payload_length-= CONST_PAYLOAD_CRC32;
	}
	
	// Append payload bytes / option 0 - RANDOM PAYLOAD ...
	// ----------------------------------------------------
	if(m_dataFrom == CONST_DATA_FROM_RANDOM_GENERATOR){
	  for(int i = offset;i < payload_length;i++){
	      data_packet[i] = rand() % 256; 	// Bytes - 0 ... 255
	  }
	}
	
	// Append payload bytes / option 1 - DATA_FROM_FILE PAYLOAD ...
	// ------------------------------------------------------------
	if(m_dataFrom == CONST_DATA_FROM_FILE){
	  if(m_fileIncrement < m_fileSize){ 
	    // Check for opened file ...
	     // -------------------------
	     if(fileToTransfer.is_open()){
		char read2file[payload_length - offset];
	        // seekg ...
	        // ---------
		fileToTransfer.seekg(m_fileIncrement);
		// read ...
		// --------
		fileToTransfer.read(read2file, (payload_length - offset));
	        // EOF or error states ...
		// -----------------------
		if(!fileToTransfer){
		   extraOffset = (payload_length - offset) - fileToTransfer.gcount(); 
		   fileToTransfer.clear(); 	// Clear errors ...
		}
		
		// m_fileIncrement ...
		// -------------------
		m_fileIncrement+=(payload_length - offset) - extraOffset;
		
		// copy data to data_packet ...
		// ----------------------------
		for(int i = offset;i < (payload_length - extraOffset);i++){
		    data_packet[i] = (unsigned char)read2file[i-offset];
		}
		
		// pmt ...
		// -------
        meta = pmt::dict_add(meta, pmt::string_to_symbol("f_errors"), pmt::from_long(CONST_FILE_READ_OK));
		meta = pmt::dict_add(meta, pmt::string_to_symbol("f_transfer"), pmt::from_long(CONST_TRANSFER_IN_PROGRESS));
		message_port_pub(out_port_0,meta);
	    }else{
	      // Handle errors + pmt ...
	      // -----------------------
	      meta = pmt::dict_add(meta, pmt::string_to_symbol("f_errors"), pmt::from_long(CONST_FILE_READ_OK));
	      meta = pmt::dict_add(meta, pmt::string_to_symbol("f_transfer"), pmt::from_long(CONST_TRANSFER_READY));
	      message_port_pub(out_port_0,meta);
	      return; 
	     }
	  }else{
	    // pmt - transfer ready ...
	    // ------------------------
	    meta = pmt::dict_add(meta, pmt::string_to_symbol("f_errors"), pmt::from_long(CONST_FILE_READ_OK));
	    meta = pmt::dict_add(meta, pmt::string_to_symbol("f_transfer"), pmt::from_long(CONST_TRANSFER_READY));
	    message_port_pub(out_port_0,meta);
	    return;
	  }
	}
	
    // Append payload bytes / option 2 - RANDOM FROM CMD LINE ...
	// ----------------------------------------------------------
	if(m_dataFrom == CONST_DATA_CMD_LINE){
       data_packet.resize(packet_bytes.size());  
       m_packetLength = packet_bytes.size();
        
	   for(int i = 0;i < packet_bytes.size();i++){
	       data_packet[i] = packet_bytes[i]; 	
	   }
	}
	
	// Generate header of the packet / POST  ...
	// -----------------------------------------
	if(m_appendHeader && m_dataFrom != CONST_DATA_CMD_LINE){
	   if(extraOffset!=0){ *(uint16_t*)&data_packet[2]-= extraOffset; *(uint32_t*)&data_packet[4] = gr::digital::crc32(&data_packet[0],4);}
	   *(uint32_t*)&data_packet[payload_length - extraOffset] = gr::digital::crc32(&data_packet[8],(payload_length - offset) - extraOffset);	// extraOffset - valid only for file read ...
	}
	
	// Data packed ...
	// ---------------
	if(m_dataType == CONST_DATA_PACKED){
	   pmt::pmt_t packed_vec = pmt::init_u8vector(m_packetLength, data_packet);
	   message_port_pub(out_port_1, pmt::cons(meta, packed_vec));
	}
	
	// Data unpacked ...
	// -----------------
	/* in 0b11110000 out 0b00000001 0b00000001 0b00000001 0b00000001 0b00000000 0b00000000 0b00000000 0b00000000
	 * https://stackoverflow.com/questions/50977399/what-do-packed-to-unpacked-blocks-do-in-gnu-radio
	 * */
	
	if(m_dataType == CONST_DATA_UNPACKED){
	   int unpackedLength = m_packetLength*8;  
	   vector<unsigned char> data_unpacked(unpackedLength,0x00); // unpacked bytes ...
	   
	   for (int i=0; i<unpackedLength; i++){
		data_unpacked[i] = (unsigned char)((data_packet.at(i/8) & (1 << (7 - (i % 8)))) != 0);
	   }
	   
	   pmt::pmt_t unpacked_vec = pmt::init_u8vector(unpackedLength, data_unpacked);
	   message_port_pub(out_port_1, pmt::cons(meta, unpacked_vec));
	}
     } 
   } /* namespace gsSDR */
} /* namespace gr */

