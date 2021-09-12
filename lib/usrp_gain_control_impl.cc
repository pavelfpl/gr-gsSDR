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
#include <gnuradio/blocks/pdu.h>
#include <gnuradio/logger.h>

#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include "usrp_gain_control_impl.h"

#include <stdlib.h>  
#include <cstdlib>
#include <iostream>

#define CONST_SLEEP_INTERVAL_MILI_SECONDS 1

namespace gr {
  namespace gsSDR {
      
    using namespace std;    
    
    // Define static variables ...
    // ---------------------------
    bool usrp_gain_control_impl::m_exit_requested;    

    usrp_gain_control::sptr
    usrp_gain_control::make(float gain_on,float gain_off,int timer_wait)
    {
      return gnuradio::get_initial_sptr
        (new usrp_gain_control_impl(gain_on, gain_off, timer_wait));
    }

    /*
     * The private constructor ...
     * ---------------------------
     */
    usrp_gain_control_impl::usrp_gain_control_impl(float gain_on,float gain_off,int timer_wait)
      : gr::block("usrp_gain_control",
              gr::io_signature::make(0,0,0),
              gr::io_signature::make(0,0,0)),
              m_gain_on(gain_on), m_gain_off(gain_off), m_timer_wait(timer_wait)
    {
        
       m_new_timer = false; 
       m_exit_requested = false; 

       // Register PDU output port / usrp control ...
       // -------------------------------------------
       out_port_0 = pmt::mp("usrp_pdus");
       message_port_register_out(out_port_0); 
       
       message_port_register_in(pmt::mp("pdus"));	  
       set_msg_handler(pmt::mp("pdus"), boost::bind(&usrp_gain_control_impl::packet_handler, this, _1));
       
      
       /* -- Create read stream THREAD -- 
       [http://antonym.org/2009/05/threading-with-boost---part-i-creating-threads.html]
       and 
       [http://antonym.org/2010/01/threading-with-boost---part-ii-threading-challenges.html]
       */

      _thread = gr::thread::thread(&usrp_gain_control_impl::usrp_gain_control_wait,this);
        
    }

    /*
     * Our virtual destructor ...
     * --------------------------
     */
    usrp_gain_control_impl::~usrp_gain_control_impl(){
       
        // Only for debug  ...
       // -------------------
       cout<< "Calling STOP - usrp_gain_control_impl ... "<< endl;
        
       threadDeInit(); // De-init  ...
        
    }

    void usrp_gain_control_impl::forecast (int noutput_items, gr_vector_int &ninput_items_required)
    {
      /* <+forecast+> e.g. ninput_items_required[0] = noutput_items */
    }
    
    // stop - function ... 
    bool usrp_gain_control_impl::stop(){
        
        // Only for debug  ...
        // -------------------
        cout<< "Calling STOP - usrp_gain_control_impl ... "<< endl;
        
        threadDeInit(); // De-init  ...
      
        return true;
    }
    
    // threadTransferDeInit function [private] ...
    void usrp_gain_control_impl::threadDeInit(){
        
        gr::thread::scoped_lock lock(fp_mutex);   // shared resources ...
        m_exit_requested = true;
        lock.unlock();

        if(_thread.joinable()) _thread.join();
    } 
    
    // setGainOn - callback ...
    void usrp_gain_control_impl::set_gain_on(int gain_on){
          gr::thread::scoped_lock lock(fp_mutex);  
          m_gain_on = gain_on;
    }
    
    // setGainOff - callback ...
    void usrp_gain_control_impl::set_gain_off(int gain_off){
          gr::thread::scoped_lock lock(fp_mutex);  
          m_gain_off = gain_off;
    }
    
    // setTimerWait - callback ...
    void usrp_gain_control_impl::set_timer_wait(int timer_wait){
          gr::thread::scoped_lock lock(fp_mutex);  
          m_timer_wait = timer_wait;
    }
    
    // usrp_gain_control_wait - public function ...
    void usrp_gain_control_impl::usrp_gain_control_wait(){
        
        // Send m_gain_off at the start ...
        boost::this_thread::sleep(boost::posix_time::milliseconds(CONST_SLEEP_INTERVAL_MILI_SECONDS));
        send_uhd_usrp_cmd(m_gain_off);
        
        while(true){
            // m_exit_requested - request ...
            // ------------------------------
            if(m_exit_requested) return;   

            // m_new_timer - request ...
            // -------------------------  
            if(m_new_timer){
               gr::thread::scoped_lock lock(fp_mutex);     
               m_new_timer = false; 
               lock.unlock(); 
               boost::asio::io_context io;
               // boost::asio::deadline_timer t(io, boost::posix_time::seconds(1));
               boost::asio::deadline_timer t(io, boost::posix_time::milliseconds(m_timer_wait));
               t.wait();
               send_uhd_usrp_cmd(m_gain_off);
            }else{
               boost::this_thread::sleep(boost::posix_time::milliseconds(CONST_SLEEP_INTERVAL_MILI_SECONDS));
            }
        } 
#ifdef CONST_DEBUG            
        cout<< "usrp_gain_control_wait processing THREAD ends - clearing resources ..."<< endl;   
#endif
    }    

    // send_uhd_usrp_cmd - private function ...
    void usrp_gain_control_impl::send_uhd_usrp_cmd(float gain){
        
       pmt::pmt_t command = pmt::make_dict();
       
       command = pmt::dict_add(command, pmt::mp("gain"), pmt::mp(gain));
       // Alternative ?
       // command = pmt::dict_add(command, pmt::string_to_symbol("gain"), pmt::from_double(gain));
        
       message_port_pub(out_port_0, command);  //  pmt::cons(meta) 
    }

    // usrp_gain_control_impl - public function / callback ...
    void usrp_gain_control_impl::packet_handler(pmt::pmt_t msg){
	  
	   //  Get parameters from the pdu ...
	   //  pmt::pmt_t meta_in = pmt::car(msg);
       cout<<"Get trigger: "<<msg<<endl;
        
       send_uhd_usrp_cmd(m_gain_on);
            
       gr::thread::scoped_lock lock(fp_mutex);   
       m_new_timer = true; 
    }

    int usrp_gain_control_impl::general_work (int noutput_items,
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

  } /* namespace gsSDR */
} /* namespace gr */

