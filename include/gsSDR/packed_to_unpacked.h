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

#ifndef INCLUDED_GSSDR_PACKED_TO_UNPACKED_H
#define INCLUDED_GSSDR_PACKED_TO_UNPACKED_H

#include <gsSDR/api.h>
#include <gnuradio/block.h>

namespace gr {
  namespace gsSDR {

    /*!
     * \brief <+description of block+>
     * \ingroup gsSDR
     *
     */
    class GSSDR_API packed_to_unpacked : virtual public gr::block
    {
     public:
      typedef boost::shared_ptr<packed_to_unpacked> sptr;

      /*!
       * \brief Return a shared_ptr to a new instance of gsSDR::packed_to_unpacked.
       *
       * To avoid accidental use of raw pointers, gsSDR::packed_to_unpacked's
       * constructor is in a private implementation
       * class. gsSDR::packed_to_unpacked::make is the public interface for
       * creating new instances.
       */
      static sptr make();
    };

  } // namespace gsSDR
} // namespace gr

#endif /* INCLUDED_GSSDR_PACKED_TO_UNPACKED_H */

