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

#ifndef INCLUDED_GSSDR_GS_DOPPLER_CORRECTION_IMPL_H
#define INCLUDED_GSSDR_GS_DOPPLER_CORRECTION_IMPL_H

#include <gsSDR/gs_doppler_correction.h>
#include <gnuradio/thread/thread.h>

#include <boost/bind.hpp>
#include <boost/asio.hpp>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/algorithm/string/trim.hpp>

namespace gr {
  namespace gsSDR {
    
    using namespace std;  
    using boost::asio::ip::tcp; 
    
    class server_session;
    
    // Class server ...
    // ---------------
    class server {
        public:
            server(boost::asio::io_service& io_service, short port);
            void handle_timeout(const boost::system::error_code& error);
                
            void decrement_active_connection();
            void add_active_connection();

            void handle_accept(server_session* new_session, const boost::system::error_code& error);
        private:
            boost::asio::io_service& io_service_;
            tcp::acceptor acceptor_;
            boost::asio::deadline_timer timer_;
    
           int count; 
    };
    
    // Class server_session ...
    // ------------------------
    class server_session{
        public:
            server_session(boost::asio::io_service& io_service, server *p_server_);
            tcp::socket& socket();
            void start();
            void handle_timeout(const boost::system::error_code& error);
            void handle_read(const boost::system::error_code& error, size_t bytes_transferred);
            void handle_write(const boost::system::error_code& error);
        private:
            boost::asio::io_service& io_service_;
            server *p_server;
            tcp::socket socket_;
            boost::asio::deadline_timer timer_;
            boost::asio::streambuf buffer;
            
            bool newFreq;
            int m_freq;
    };
      
    // Class gs_doppler_correction_impl ...
    class gs_doppler_correction_impl : public gs_doppler_correction
    {
     private:
        pmt::pmt_t out_port_0; 
      
        static bool m_exit_requested;
         
        std::string m_ServerName;
        std::string m_ServerPort;
        int m_sourceType;
      
        gr::thread::thread _thread;
        boost::mutex fp_mutex;
        static gs_doppler_correction_impl *sInstance;
     
        void threadTransferDeInit();
     public:
      gs_doppler_correction_impl(const std::string &ServerName, const std::string &ServerPort, int sourceType);
      ~gs_doppler_correction_impl();

      static gs_doppler_correction_impl *mainInstance();
      void gs_doppler_correction_wait();  
      void message_callback(int freq);
      bool stop_callback() {return m_exit_requested;}
      bool stop();       
      
      // Where all the action really happens ...
      void forecast (int noutput_items, gr_vector_int &ninput_items_required);

      int general_work(int noutput_items,
           gr_vector_int &ninput_items,
           gr_vector_const_void_star &input_items,
           gr_vector_void_star &output_items);

    };

  } // namespace gsSDR
} // namespace gr

#endif /* INCLUDED_GSSDR_GS_DOPPLER_CORRECTION_IMPL_H */

