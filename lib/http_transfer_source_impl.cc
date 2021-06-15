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
 * The above copyright notice and this permission notice shall be included in 
all
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

// Gnuradio includes ...
// ---------------------
#include <gnuradio/io_signature.h>
#include <gnuradio/blocks/pdu.h>
#include <gnuradio/thread/thread.h>
#include <gnuradio/digital/crc32.h>
#include <gnuradio/logger.h>

// Boost includes ...
// ------------------
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include <boost/date_time/posix_time/posix_time.hpp>

// System includes ...
// -------------------
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <regex>
#include <vector>
#include <iomanip>
#include <sys/time.h>

// Gnuradio modules include ...
// ----------------------------
#include "http_transfer_source_impl.h"

// Define constants ...
// --------------------
#define CONST_SLEEP_INTERVAL_MILI_SECONDS 100
 
// #define CONST_DEBUG
#define CONST_DEBUG_HTTP

/*
 https://github.com/boostorg/boost_install/issues/12
//Stops looking for BoostConfig.cmake e.g. in 
/opt/boost_1_76_0/lib/cmake/Boost-1.76.0/BoostConfig.cmake 
cmake -DBoost_NO_BOOST_CMAKE=ON ../

export TARGET_BOOST=/opt/boost_1_76_0
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/opt/boost_1_76_0/lib
cmake -DBoost_NO_BOOST_CMAKE=TRUE -DBoost_NO_SYSTEM_PATHS=TRUE 
-DBOOST_ROOT:PATHNAME=$TARGET_BOOST 
-DBoost_LIBRARY_DIRS:FILEPATH=${TARGET_BOOST}/lib ../

ldd /home/pavelf/gr3.8/lib/libgnuradio-gsSDR.so
*/

// #define CONST_BOOST_1_7

namespace gr {
  namespace gsSDR {
    // Short alias for this namespace ...
    // ----------------------------------
    namespace beast = boost::beast;     // from <boost/beast.hpp>
    namespace http = beast::http;       // from <boost/beast/http.hpp>
    namespace net = boost::asio;        // from <boost/asio.hpp>
#ifdef CONST_BOOST_1_7    
    using tcp = net::ip::tcp;           // from <boost/asio/ip/tcp.hpp>
#else
    using tcp = boost::asio::ip::tcp; 
#endif
    
    namespace pt = boost::property_tree;
    using namespace boost::posix_time;
     
    using namespace std;   
    
    // Define static variables ...
    // ---------------------------
    bool http_transfer_source_impl::m_exit_requested;  
    
    // std long to string conversion ...
    static std::string long_to_string(long value) {
        std::ostringstream os;
        os << value;
        return os.str();
    }
    
    // HEX string to uint8 ...
    static uint8_t string_to_uint8(std::string str) {

        uint16_t tmp=0x0000;
        uint8_t x=0x00;
        std::stringstream ss;
        ss << std::hex<< str;
        ss >> tmp;
        x = static_cast<uint8_t>(tmp);
        return x;
    }
    // create_json_string function ...
    static std::string create_json_string(pt::ptree oroot_){
        
        std::ostringstream buf;
        write_json (buf, oroot_, false);
        std::string json = buf.str();

        std::regex reg("\\\"((?:0[xX])?[0-9a-fA-F]+)\\\"");                      
       // std::regex reg("\\\"([0-9]+\\.{0,1}[0-9]*)\\\"");
        std::string json_corection_1 = std::regex_replace(json, reg, "$1");
        std::regex reg_2("\\\"(true|false)\\\"");
        std::string json_corection_2 = std::regex_replace(json_corection_1, reg_2, "$1");

#ifdef CONST_DEBUG  
        std::cout<<json_corection_2<<std::endl;
#endif
        return json_corection_2;
    }
    
    // create_json_response - static function ...
    static std::string create_json_response(int64_t timestamp, int spacecraftId){

        pt::ptree oroot_response;
        oroot_response.put("timestamp",timestamp); 
        oroot_response.put("spacecraftId",spacecraftId);
        oroot_response.put("state","OK");

        return create_json_string(oroot_response);
    }   
    
    http_transfer_source::sptr
    http_transfer_source::make(const std::string &ServerName, const std::string &ServerPort,const std::string &ServerTarget, int stationId, int spacecraftId, 
        const std::string &UserName, const std::string &UserPass){
            return gnuradio::get_initial_sptr
                (new http_transfer_source_impl(ServerName, ServerPort, ServerTarget, stationId, spacecraftId, UserName, UserPass));
    }

    /*
     * The private constructor ...
     * ---------------------------
     */
    http_transfer_source_impl::http_transfer_source_impl(const std::string &ServerName, const std::string &ServerPort,const std::string &ServerTarget, int stationId, int spacecraftId, const std::string &UserName, const std::string &UserPass)
      : gr::block("http_transfer_source",
              gr::io_signature::make(0,0,0),    // <+MIN_IN+>, <+MAX_IN+>, sizeof(<+ITYPE+>)
              gr::io_signature::make(0,0,0)),   // <+MIN_OUT+>, <+MAX_OUT+>, sizeof(<+OTYPE+>)
              m_ServerName(ServerName), 
              m_ServerPort(ServerPort),m_ServerTarget(ServerTarget), m_stationId(stationId), 
              m_spacecraftId(spacecraftId), m_UserName(UserName), m_UserPass(UserPass)
    {
        
       m_exit_requested = false; 

       // Register PDU output port ...
       // ---------------------------- 
       out_port_0 = pmt::mp("out");
       message_port_register_out(out_port_0);  
      
       /* Create read stream THREAD 
        [http://antonym.org/2009/05/threading-with-boost---part-i-creating-threads.html]
        and 
        [http://antonym.org/2010/01/threading-with-boost---part-ii-threading-challenges.html]
       */
#ifdef CONST_DEBUG       
      cout << "Threading - using up to CPUs/Cores: "<< boost::thread::hardware_concurrency() <<endl;
#endif
      _thread = gr::thread::thread(&http_transfer_source_impl::http_transfer_source_wait,this);
    }

    /*
     * Our virtual destructor ...
     * --------------------------
     */
    http_transfer_source_impl::~http_transfer_source_impl(){
    
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
    bool http_transfer_source_impl::stop(){
        
        // Only for debug  ...
        // -------------------
        cout<< "Calling STOP - http_transfer_source_impl ... "<< endl;
        
        threadTransferDeInit(); // De-init  ...
      
        return true;
    }
    
    // threadTransferDeInit function [private] ...
    void http_transfer_source_impl::threadTransferDeInit(){
        
        gr::thread::scoped_lock lock(fp_mutex);   // shared resources ...
        m_exit_requested = true;
        lock.unlock();

        if(_thread.joinable()) _thread.join();
    }
    
    // http_transfer_source_wait function [provides new THREAD] - [private /* static */] ...
    void http_transfer_source_impl::http_transfer_source_wait(){
        
             bool server_connected = true;
             
             int spacecraftId = 0;
             int radioId = 0;
             int64_t timestamp = 0;
                        
             string packet_type = "";
             string packet_payload="";
        
#ifdef CONST_DEBUG
            cout << "http_transfer_sink_wait SDR processing THREAD was started" << endl;
#endif
            if(m_exit_requested){
#ifdef CONST_DEBUG            
               cout<< "http_transfer_sink_wait processing THREAD ends - clearing resources ... "<< endl;   
#endif
               return; 
            }
            
            // Set up an HTTP GET request message ...
            // --------------------------------------
            // http::request<http::string_body> req{http::verb::get, target, version};
            // req.set(http::field::host, host);
            // req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
            
            http::request<http::string_body> req_get;
            req_get.version(11);
            req_get.method(http::verb::get);
            req_get.target(m_ServerTarget);
            req_get.set(http::field::host, m_ServerName);
            req_get.set(http::field::content_type, "application/json");

            http::request<http::string_body> req_post_resp;
            req_post_resp.version(11);
            req_post_resp.method(http::verb::post);
            req_post_resp.target(m_ServerTarget);
            req_post_resp.set(http::field::host, m_ServerName);
            req_post_resp.set(http::field::content_type, "application/json");
            
            // The io_context is required for all I/O ...
            // ------------------------------------------
            net::io_context ioc;

            // These objects perform our I/O ...
            // ---------------------------------
#ifdef CONST_BOOST_1_7
            tcp::resolver resolver(ioc);
            beast::tcp_stream stream(ioc);
#else     
            tcp::resolver resolver{ioc};
            tcp::socket stream{ioc};
#endif       
            // This buffer is used for reading and must be persisted ...
            beast::flat_buffer buffer;
            
            // Declare a container to hold the response ...
            http::response<http::dynamic_body> res;
            
            auto results = resolver.resolve(m_ServerName, m_ServerPort);  // Look up the domain name ...
            
            try{
                // Make the connection on the IP address we get from a lookup ...
#ifdef CONST_BOOST_1_7
                stream.connect(results);
#else
                boost::asio::connect(stream, results.begin(), results.end());
#endif               
            }
                
            catch (boost::system::system_error const& e){
                std::cout << "Warning: could not connect : " << e.what() << std::endl;
                server_connected = false;
            }

            catch(std::exception const& e){
                std::cerr << "Error: " << e.what() << std::endl; // return EXIT_FAILURE;
                server_connected = false;
            }
            
            // While TRUE loop ...
            // -------------------
            while(true){
                try{ // START of TRY - CATCH ...
                    
                    // Test if server is connected ...
                    if(!server_connected){   
#ifdef CONST_BOOST_1_7
                        stream.connect(results);
#else
                        boost::asio::connect(stream, results.begin(), results.end());
#endif               
                        
                        server_connected = true;
                    }
                    
                    http::write(stream, req_get);           // Send (periodic) the HTTP request to the remote host ...
                    beast::flat_buffer buffer;              // This buffer is used for reading and must be persisted ...
                    http::response<http::dynamic_body> res; // Declare a container to hold the response
                    http::read(stream, buffer, res);        // Receive the HTTP response

                    // std::cout << res << std::endl; 

                    std::string s = boost::beast::buffers_to_string(res.body().data());

                    // std::cout <<"Data response: "<< s << std::endl;

                    if (s.find("[]") != std::string::npos || s.size()<=2) {
#ifdef CONST_DEBUG_HTTP                     
                      //std::cout<<"No valid packet found ..."<<std::endl;
#endif
                    }else{
#ifdef CONST_DEBUG_HTTP                     
                        std::cout<<"New packet  ..."<<std::endl;
#endif
                        pt::ptree pt_read;
                        std::istringstream is(s);
                        read_json (is, pt_read);

                        for (auto& array_element : pt_read) {
                          for (auto& property : array_element.second){
                            std::cout << property.first << " = " << property.second.get_value<std::string>() << "\n";
                            
                            //-- 1] test property - type  ...
                            if(property.first.compare("type")==0){ // TC ...
                               packet_type = property.second.get_value<std::string>();
                            }       
                            
                            //-- 2] test property - timestamp  ...
                            if(property.first.compare("timestamp")==0){
                               timestamp = property.second.get_value<int64_t>();
                            }
                            
                            //-- 3] test property - payload  ...
                            if(property.first.compare("payload")==0){
                               packet_payload = property.second.get_value<std::string>();
                            }       

                            //-- 4] test property - spacecraftId  ...
                            if(property.first.compare("spacecraftId")==0){
                               spacecraftId = property.second.get_value<int>();
                            }
                            
                            //-- 5] test property - radioId  ...
                            if(property.first.compare("radioId")==0){
                               radioId = property.second.get_value<int>();
                            }
                        }
                    }
                    
                    if(packet_type.compare("TC")==0 && timestamp!=0 && spacecraftId != 0){
                       req_post_resp.body() = create_json_response(timestamp,spacecraftId);
                       req_post_resp.prepare_payload();

                       http::write(stream, req_post_resp); 
                       beast::flat_buffer buffer;          
                       http::response<http::dynamic_body> resStatus;
                       http::read(stream, buffer, resStatus);
                       // std::cout << res << std::endl;

                       if(resStatus.result_int() == 200){
#ifdef CONST_DEBUG_HTTP                         
                          std::cout<<"Packet ACK was sent OK ..."<<resStatus.result()<<std::endl;
#endif
                       }
                    }
                 }
              } // End of TRY - CATCH ...
                
              catch (boost::system::system_error const& e){
                    std::cout << "Warning: could not connect : " << e.what() << std::endl;
                    server_connected = false;
              }

              catch(std::exception const& e){
                    std::cerr << "Error: " << e.what() << std::endl; // return EXIT_FAILURE;
                    server_connected = false;
              }
              
              // Create pdu and send payload data to SDR radio ...
              // -------------------------------------------------
              if(packet_type.compare("TC")==0 && timestamp!=0 && spacecraftId !=0){
                
                int c = 0;   
                bool ok = true;
                vector<uint8_t> pmt_payload((packet_payload.length()/2)-2); 
                 
                for(int i=0;i<packet_payload.length(); i+=2){
                     if(i==0){
                       if(packet_payload.substr(0,2).compare("0x")!=0){ok = false; break;}   
                     }else{
                       pmt_payload[c++]=(string_to_uint8(packet_payload.substr(i,2)));   
                     }
                }
                
                if(ok){
                   // Create PDU (DATA payload + meta data) ... 
                   // ------------------------------------------ 
                   pmt::pmt_t meta = pmt::make_dict();
                   meta = pmt::dict_add(meta, pmt::string_to_symbol("timestamp"), pmt::from_long(timestamp));
                   meta = pmt::dict_add(meta, pmt::string_to_symbol("spacecraftId"), pmt::from_long(spacecraftId));  
                   meta = pmt::dict_add(meta, pmt::string_to_symbol("radioId"), pmt::from_long(radioId));
                   
                   pmt::pmt_t data_vector = pmt::init_u8vector(pmt_payload.size(),pmt_payload); 
                   message_port_pub(out_port_0, pmt::cons(meta, data_vector));
                }
                
                // Reset all parameters ...  
                packet_type = "";  packet_payload = "";  timestamp = 0;  spacecraftId = 0; radioId = 0;
              }
              
              gr::thread::scoped_lock lock(fp_mutex);   // shared resources ...
              if(m_exit_requested){                     // shared global variable - exit requested if m_exit_requested == TRUE --> break ... 
                if(!server_connected) break; 
                  
                try{ 
                   // Gracefully close the socket ...
                   beast::error_code ec;
                    
#ifdef CONST_BOOST_1_7                    
                   stream.socket().shutdown(tcp::socket::shutdown_both, ec);
#else
                   stream.shutdown(tcp::socket::shutdown_both, ec);
#endif
                   // Not_connected happens sometimes, so don't bother reporting it ...
                   if(ec && ec != beast::errc::not_connected) throw beast::system_error{ec};
                }
                
                catch (boost::system::system_error const& e){
                    std::cout << "Warning: could not disconnect : " << e.what() << std::endl;
                }

                catch(std::exception const& e){
                    std::cerr << "Error: " << e.what() << std::endl; // return EXIT_FAILURE;
                } 
                  
                // Break finally ... 
                break;                
                    
              } 
                
              // Waiting for data ...
              // --------------------
              boost::this_thread::sleep(boost::posix_time::milliseconds(CONST_SLEEP_INTERVAL_MILI_SECONDS)); 
        
        }
    }
    
    void http_transfer_source_impl::forecast (int noutput_items, gr_vector_int &ninput_items_required){
        /* <+forecast+> e.g. ninput_items_required[0] = noutput_items */
    }

    int http_transfer_source_impl::general_work (int noutput_items,
                       gr_vector_int &ninput_items,
                       gr_vector_const_void_star &input_items,
                       gr_vector_void_star &output_items)
    {
        
      /*  
      const <+ITYPE+> *in = (const <+ITYPE+> *) input_items[0];
      <+OTYPE+> *out = (<+OTYPE+> *) output_items[0];
      */
      
      // Do <+signal processing+>
      // ------------------------
      // Tell runtime system how many input items we consumed on
      // each input stream.
      consume_each (noutput_items);

      // Tell runtime system how many output items we produced.
      return noutput_items;
    }

  } /* namespace gsSDR */
} /* namespace gr */

