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

#ifndef INCLUDED_GSSDR_WEBSOCKET_TRANSFER_SINK_H
#define INCLUDED_GSSDR_WEBSOCKET_TRANSFER_SINK_H

#include <gsSDR/api.h>
#include <gnuradio/block.h>

namespace gr {
  namespace gsSDR {

    /*!
     * \brief <+description of block+>
     * \ingroup gsSDR
     *
     */
    class GSSDR_API websocket_transfer_sink : virtual public gr::block
    {
     public:
      typedef boost::shared_ptr<websocket_transfer_sink> sptr;

      /*!
       * \brief Return a shared_ptr to a new instance of gsSDR::websocket_transfer_sink.
       *
       * To avoid accidental use of raw pointers, gsSDR::websocket_transfer_sink's
       * constructor is in a private implementation
       * class. gsSDR::websocket_transfer_sink::make is the public interface for
       * creating new instances.
       */
      static sptr make(const std::string &ServerName="178.128.160.180", const std::string &ServerPort="8002", int stationId=0, const std::string &UserName="", const std::string &UserPass="");
    };

  } // namespace gsSDR
} // namespace gr

#endif /* INCLUDED_GSSDR_WEBSOCKET_TRANSFER_SINK_H */

