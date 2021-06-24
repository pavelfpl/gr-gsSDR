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
  
#include <stdlib.h>  
#include <cstdlib>
#include <iostream>

// Gnuradio modules include ...
// ----------------------------
#include "gs_doppler_correction_impl.h"

#define CONST_DEBUG
#define CONST_CONNECTION_TIMEOUT 60
#define CONST_READ_TIMEOUT 15

#define CONST_PAYLOAD_LENGTH 1

namespace gr {
  namespace gsSDR {
      

    // Define static variables ...
    // ---------------------------
    bool gs_doppler_correction_impl::m_exit_requested;    
    
    // gs_doppler_correction_impl - sInstance ...
    // ------------------------------------------
    gs_doppler_correction_impl *gs_doppler_correction_impl::sInstance = 0;
    
    // string to short ...
    static int16_t string_to_short(std::string str){

        int16_t tmp=0;
        std::stringstream ss;
        ss << str;
        ss >> tmp;
        
        return tmp;
    }
    
    // ------------------------
    // server_session class ...
    // ------------------------
    
    // server_session constructor ...
    server_session::server_session(boost::asio::io_service& io_service, server *p_server_) :io_service_(io_service),p_server(p_server_), socket_(io_service),timer_(io_service){}
            
    tcp::socket& server_session::socket(){ 
         return socket_;
    }

    // start ...
    void server_session::start(){
            
            std::cout<<"Accepting new request ..."<<std::endl;
                
            timer_.expires_from_now( boost::posix_time::seconds(CONST_READ_TIMEOUT));
            timer_.async_wait(boost::bind(&server_session::handle_timeout, this, boost::asio::placeholders::error));
                
            boost::asio::async_read_until(socket_, buffer, '\n',
                            boost::bind(&server_session::handle_read, this,
                            boost::asio::placeholders::error,
                            boost::asio::placeholders::bytes_transferred));
    }
       
    // handle_timeout ...   
    void server_session::handle_timeout(const boost::system::error_code& error){
                
            // Check for closing event ....
            if(gs_doppler_correction_impl::mainInstance()){
               if(gs_doppler_correction_impl::mainInstance()->stop_callback()){
                  io_service_.stop(); 
                  return;   
            }
        }
    }

    // handle_read ...
    void server_session::handle_read(const boost::system::error_code& error, size_t bytes_transferred){

        int freq=0; 
        std::string response; 
        
        newFreq = false;
        
        if(gs_doppler_correction_impl::mainInstance()){
           if(gs_doppler_correction_impl::mainInstance()->stop_callback()){
              io_service_.stop(); 
              return;   
            }
        }
                
        if(!error){
           std::ostringstream ss;
           ss<<&buffer;                
           std::string s = ss.str();
           
           //Check if s starts with given prefix ...
           if (s.rfind("F", 0) == 0) {
               s.erase(0, 1);
               s.erase(std::remove(s.begin(), s.end(), '\n'),s.end());
               boost::algorithm::trim(s);
            
               try{
                   freq = boost::lexical_cast<int>(s);
                   if(freq!=m_freq){ m_freq = freq; newFreq = true;}
               }catch(boost::bad_lexical_cast const& e){
                    std::cout << "Error: " << e.what() << "\n";
               }
               
               response = "RPRT 0\n";
               
           } else if (s.rfind("f", 0) == 0) {
               response=string("f:")+std::to_string(m_freq)+string("\n");
           } else{
                return;
           }
           
           boost::asio::async_write(socket_,
                        boost::asio::buffer(response),
                        boost::bind(&server_session::handle_write, this,
                        boost::asio::placeholders::error));
        
        }else{
            cout<<"Socket READ - error/timeout ... "<<endl;
            p_server->decrement_active_connection();
            delete this; 
        }
    }
    
    // handle_write ...
    void server_session::handle_write(const boost::system::error_code& error){
                
        if(gs_doppler_correction_impl::mainInstance()){
           if(gs_doppler_correction_impl::mainInstance()->stop_callback()){
              io_service_.stop(); 
              return;   
            }
                   
            // Set new frequency ...
            if(newFreq) gs_doppler_correction_impl::mainInstance()->message_callback(m_freq);
        }
                
        if(!error){
            start(); 
        }else{
            cout<<"Socket WRITE - error/timeout ... "<<endl;   
            p_server->decrement_active_connection();
            delete this;
                    
        }
    }
    
    // ----------------
    // server class ...
    //-----------------
    
    // server constructor ...
    server::server(boost::asio::io_service& io_service, short port)
            :io_service_(io_service), acceptor_(io_service, tcp::endpoint(tcp::v4(), port)),timer_(io_service){

            cout<<"Waiting for connection ..."<<endl;    
                    
            count = 0;    
                    
            timer_.expires_from_now( boost::posix_time::seconds(CONST_CONNECTION_TIMEOUT));
            timer_.async_wait(boost::bind(&server::handle_timeout, this, boost::asio::placeholders::error));    
                    
            server_session* new_session = new server_session(io_service_,this);
            acceptor_.async_accept(new_session->socket(),
                            boost::bind(&server::handle_accept, this, new_session,
                            boost::asio::placeholders::error));
    }
    
    // handle_timeout ... 
    void server::handle_timeout(const boost::system::error_code& error){
            
        if(count > 0) return;
                
        cout<<"No active connection - waiting for connection ..."<<endl;
                
        // Check for closing event ....
        if(gs_doppler_correction_impl::mainInstance()){
           if(gs_doppler_correction_impl::mainInstance()->stop_callback()){
              io_service_.stop(); 
               return;   
            }
        }
                
        timer_.expires_from_now( boost::posix_time::seconds(30));
        timer_.async_wait(boost::bind(&server::handle_timeout, this, boost::asio::placeholders::error));  
    }
    
    // decrement_active_connection ...
    void server::decrement_active_connection(){
         if(count > 0) count--;
    }
    
    // add_active_connection ...
    void server::add_active_connection(){
         count++;
    }

    // handle_accept ... 
    void server::handle_accept(server_session* new_session, const boost::system::error_code& error){
        if (!error){
            add_active_connection();
            new_session->start();
            new_session = new server_session(io_service_,this);
            acceptor_.async_accept(new_session->socket(),
            boost::bind(&server::handle_accept, this, new_session,
            boost::asio::placeholders::error));
        }else{
            delete new_session;
        }
    }
    
    gs_doppler_correction::sptr
    gs_doppler_correction::make(const std::string &ServerName, const std::string &ServerPort, int sourceType)
    {
      return gnuradio::get_initial_sptr
        (new gs_doppler_correction_impl(ServerName, ServerPort, sourceType));
    }

    // mainInstance - public !!! static !!! function - OK ...
    // ------------------------------------------------------
    gs_doppler_correction_impl *gs_doppler_correction_impl::mainInstance(){

        // Possible memory leak --> should not happen ...
        // ----------------------------------------------
        if (sInstance == 0) return 0;
    
        return sInstance;
    }
    
    /*
     * The private constructor ...
     * ---------------------------
     */
    gs_doppler_correction_impl::gs_doppler_correction_impl(const std::string &ServerName, const std::string &ServerPort, int sourceType)
      : gr::block("http_transfer_source",
              gr::io_signature::make(0,0,0),    // <+MIN_IN+>, <+MAX_IN+>, sizeof(<+ITYPE+>)
              gr::io_signature::make(0,0,0)),   // <+MIN_OUT+>, <+MAX_OUT+>, sizeof(<+OTYPE+>)
              m_ServerName(ServerName), m_ServerPort(ServerPort), m_sourceType(sourceType)
    {
       
       sInstance = this;  
        
       m_exit_requested = false; 

       // Register PDU output port ...
       // ---------------------------- 
       out_port_0 = pmt::mp("out");
       message_port_register_out(out_port_0);  
      
       /* -- Create read stream THREAD -- 
       [http://antonym.org/2009/05/threading-with-boost---part-i-creating-threads.html]
       and 
       [http://antonym.org/2010/01/threading-with-boost---part-ii-threading-challenges.html]
       */
#ifdef CONST_DEBUG       
      cout << "Threading - using up to CPUs/Cores: "<< boost::thread::hardware_concurrency() <<endl;
#endif
      _thread = gr::thread::thread(&gs_doppler_correction_impl::gs_doppler_correction_wait,this);
    }
    
    /*
     * Our virtual destructor ...
     * --------------------------
     */
    gs_doppler_correction_impl::~gs_doppler_correction_impl(){
        
#ifdef CONST_DEBUG          
       cout<< "Calling destructor - http_transfer_source_impl ... "<< endl;
#endif
       threadTransferDeInit(); // De-init  ...

#ifdef CONST_DEBUG
       cout<< "Calling destructor - http_transfer_source_impl - EnDL... "<< 
endl;
#endif     
    }
    
    // stop - function ... 
    bool gs_doppler_correction_impl::stop(){
        
        // Only for debug  ...
        // -------------------
        cout<< "Calling STOP - gs_doppler_correction_impl ... "<< endl;
        
        threadTransferDeInit(); // De-init  ...
      
        return true;
    }
    
    // threadTransferDeInit function [private] ...
    void gs_doppler_correction_impl::threadTransferDeInit(){
        
        gr::thread::scoped_lock lock(fp_mutex);   // shared resources ...
        m_exit_requested = true;
        lock.unlock();

        if(_thread.joinable()) _thread.join();
    }
    
    // message_callback [public] ...
    void gs_doppler_correction_impl::message_callback(int freq){
        
        pmt::pmt_t meta = pmt::make_dict();
        meta = pmt::dict_add(meta, pmt::string_to_symbol("freq"), pmt::from_long(freq));

        vector<uint32_t> pmt_payload(CONST_PAYLOAD_LENGTH);
        pmt_payload[0] = freq; 
        
        pmt::pmt_t data_vector = pmt::init_u32vector(pmt_payload.size(),pmt_payload); 
        message_port_pub(out_port_0, pmt::cons(meta, data_vector));
    }
    
    // gs_doppler_correction_wait - new thread ...
    void gs_doppler_correction_impl::gs_doppler_correction_wait(){
        
#ifdef CONST_DEBUG
            cout << "gs_doppler_correction_impl SDR processing THREAD was started" << endl;
#endif
            if(m_exit_requested){
#ifdef CONST_DEBUG            
               cout<< "gs_doppler_correction_impl processing THREAD ends - clearing resources ... "<< endl;   
#endif
               return; 
            }
            
            boost::asio::io_service io_service;
    
            server s(io_service, string_to_short(m_ServerPort));     // atoi(argv[1])

            io_service.run();
                
#ifdef CONST_DEBUG            
            cout<< "gs_doppler_correction_impl processing THREAD ends - clearing resources ... "<< endl;   
#endif
    }
    
    void gs_doppler_correction_impl::forecast (int noutput_items, gr_vector_int &ninput_items_required){
        /* <+forecast+> e.g. ninput_items_required[0] = noutput_items */
    }

    int gs_doppler_correction_impl::general_work (int noutput_items,
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

