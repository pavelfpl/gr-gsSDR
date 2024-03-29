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

#ifndef INCLUDED_GSSDR_USRP_GAIN_CONTROL_H
#define INCLUDED_GSSDR_USRP_GAIN_CONTROL_H

#include <gsSDR/api.h>
#include <gnuradio/block.h>

namespace gr {
  namespace gsSDR {

    /*!
     * \brief <+description of block+>
     * \ingroup gsSDR
     *
     */
    class GSSDR_API usrp_gain_control : virtual public gr::block
    {
     public:
      typedef boost::shared_ptr<usrp_gain_control> sptr;

      /*!
       * \brief Return a shared_ptr to a new instance of gsSDR::usrp_gain_control.
       *
       * To avoid accidental use of raw pointers, gsSDR::usrp_gain_control's
       * constructor is in a private implementation
       * class. gsSDR::usrp_gain_control::make is the public interface for
       * creating new instances.
       */
      static sptr make(float gain_on,float gain_off,int timer_wait);
      
      virtual void set_gain_on(int gain_on)=0;
      virtual void set_gain_off(int gain_off)=0;
      virtual void set_timer_wait(int timer_wait)=0;

    };

  } // namespace gsSDR
} // namespace gr

#endif /* INCLUDED_GSSDR_USRP_GAIN_CONTROL_H */

