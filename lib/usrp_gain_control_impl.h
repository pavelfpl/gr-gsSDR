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

#ifndef INCLUDED_GSSDR_USRP_GAIN_CONTROL_IMPL_H
#define INCLUDED_GSSDR_USRP_GAIN_CONTROL_IMPL_H

#include <gsSDR/usrp_gain_control.h>
#include <gnuradio/thread/thread.h>

namespace gr {
  namespace gsSDR {

    class usrp_gain_control_impl : public usrp_gain_control
    {
     private:
       pmt::pmt_t out_port_0; 
       static bool m_exit_requested;
       
       bool m_new_timer;
       float m_gain_on; 
       float m_gain_off; 
       int m_timer_wait;
       
       void threadDeInit();
       void send_uhd_usrp_cmd(float gain);
     public:
       usrp_gain_control_impl(float gain_on,float gain_off,int timer_wait);
      ~usrp_gain_control_impl();
      
      gr::thread::thread _thread;
      boost::mutex fp_mutex;
      
      void set_gain_on(int gain_on);
      void set_gain_off(int gain_off);
      void set_timer_wait(int timer_wait);
      
      bool stop();   
      void usrp_gain_control_wait();
      void packet_handler(pmt::pmt_t msg);
      
      // Where all the action really happens ...
      void forecast (int noutput_items, gr_vector_int &ninput_items_required);

      int general_work(int noutput_items,
           gr_vector_int &ninput_items,
           gr_vector_const_void_star &input_items,
           gr_vector_void_star &output_items);

    };

  } // namespace gsSDR
} // namespace gr

#endif /* INCLUDED_GSSDR_USRP_GAIN_CONTROL_IMPL_H */


