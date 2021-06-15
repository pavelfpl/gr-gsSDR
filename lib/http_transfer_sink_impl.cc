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
#include "http_transfer_sink_impl.h"

// Define constants ...
// --------------------
#define CONST_WRITE_BUFFER_SIZE 10
#define CONST_SLEEP_INTERVAL_MILI_SECONDS 1000
 
//#define CONST_DEBUG
#define CONST_DEBUG_HTTP

/*
 https://github.com/boostorg/boost_install/issues/12
//Stops looking for BoostConfig.cmake e.g. in /opt/boost_1_76_0/lib/cmake/Boost-1.76.0/BoostConfig.cmake 
cmake -DBoost_NO_BOOST_CMAKE=ON ../

export TARGET_BOOST=/opt/boost_1_76_0
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/opt/boost_1_76_0/lib
cmake -DBoost_NO_BOOST_CMAKE=TRUE -DBoost_NO_SYSTEM_PATHS=TRUE -DBOOST_ROOT:PATHNAME=$TARGET_BOOST -DBoost_LIBRARY_DIRS:FILEPATH=${TARGET_BOOST}/lib ../

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
      
    // std long to string conversion ...
    static std::string long_to_string(long value) {
        std::ostringstream os;
        os << value;
        return os.str();
    }
    
    // create_json_string function ...
    static std::string create_json_string(pt::ptree oroot_){
        
        std::ostringstream buf;
        write_json (buf, oroot_, false);
        std::string json = buf.str();

        std::regex reg("\\\"((?:0[xX])?[0-9a-fA-F]+)\\\"");                             // std::regex reg("\\\"([0-9]+\\.{0,1}[0-9]*)\\\"");
        std::string json_corection_1 = std::regex_replace(json, reg, "$1");
        std::regex reg_2("\\\"(true|false)\\\"");
        std::string json_corection_2 = std::regex_replace(json_corection_1, reg_2, "$1");

#ifdef CONST_DEBUG  
        std::cout<<json_corection_2<<std::endl;
#endif
        return json_corection_2;
    }
    
    // Define static variables ...
    // ---------------------------
    fifo_buffer http_transfer_sink_impl::m_fifo;
    bool http_transfer_sink_impl::m_exit_requested;  
      
    http_transfer_sink::sptr
    http_transfer_sink::make(const std::string &ServerName, const std::string &ServerPort,const std::string &ServerTarget, int stationId, int spacecraftId, const std::string &UserName, const std::string &UserPass)
    {
      return gnuradio::get_initial_sptr
        (new http_transfer_sink_impl(ServerName, ServerPort, ServerTarget, stationId, spacecraftId, UserName, UserPass));
    }

    /*
     * The private constructor ...
     * ---------------------------
     */
    http_transfer_sink_impl::http_transfer_sink_impl(const std::string &ServerName, const std::string &ServerPort,const std::string &ServerTarget, int stationId, int spacecraftId, const std::string &UserName, const std::string &UserPass)
      : gr::block("http_transfer_sink",
              gr::io_signature::make(0,0,0),    // <+MIN_IN+>, <+MAX_IN+>, sizeof(<+ITYPE+>)
              gr::io_signature::make(0,0,0)),   // <+MIN_OUT+>, <+MAX_OUT+>, sizeof(<+OTYPE+>)
              m_ServerName(ServerName), m_ServerPort(ServerPort),m_ServerTarget(ServerTarget), m_stationId(stationId), m_spacecraftId(spacecraftId), m_UserName(UserName), m_UserPass(UserPass)
    {
        
       m_packetCounter = 0; m_exit_requested = false; 
       m_fifo.fifo_changeSize(32);

       // Register PDU input port ...
       // --------------------------- 
       message_port_register_in(pmt::mp("in"));
       set_msg_handler(pmt::mp("in"), boost::bind(&http_transfer_sink_impl::packet_handler, this, boost::placeholders::_1)); // _1
       
       /* Create read stream THREAD [http://antonym.org/2009/05/threading-with-boost---part-i-creating-threads.html]
          and [http://antonym.org/2010/01/threading-with-boost---part-ii-threading-challenges.html]
       */
#ifdef CONST_DEBUG       
     cout << "Threading - using up to CPUs/Cores: "<< boost::thread::hardware_concurrency() <<endl;
#endif
     _thread = gr::thread::thread(&http_transfer_sink_impl::http_transfer_sink_wait,this);
        
    }

    /*
     * Our virtual destructor ...
     * --------------------------
     */
    http_transfer_sink_impl::~http_transfer_sink_impl(){
    
#ifdef CONST_DEBUG          
      cout<< "Calling destructor - http_transfer_sink_impl ... "<< endl;
#endif
      threadTransferDeInit(); // De-init  ...

#ifdef CONST_DEBUG
      cout<< "Calling destructor - http_transfer_sink_impl - EnDL... "<< endl;
#endif        
        
    }
    
    
    bool http_transfer_sink_impl::stop(){
        
        // Only for debug  ...
        // -------------------
        cout<< "Calling STOP - http_transfer_sink_impl ... "<< endl;
        
        threadTransferDeInit(); // De-init  ...
      
        return true;
    }
    
    
    // threadTransferDeInit function [private] ...
    void http_transfer_sink_impl::threadTransferDeInit(){
        
        gr::thread::scoped_lock lock(fp_mutex);   // shared resources ...
        m_exit_requested = true;
        lock.unlock();

        if(_thread.joinable()) _thread.join();
    }
    
    void http_transfer_sink_impl::forecast (int noutput_items, gr_vector_int &ninput_items_required){
         /* <+forecast+> e.g. ninput_items_required[0] = noutput_items */
    }

    int http_transfer_sink_impl::general_work (int noutput_items,
                       gr_vector_int &ninput_items,
                       gr_vector_const_void_star &input_items,
                       gr_vector_void_star &output_items){
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
    
    // http_transfer_sink_impl function [provides new THREAD] - [private /* static */] ...
    void http_transfer_sink_impl::http_transfer_sink_wait(){
        
            int niutput_items_real = 0;
            bool data_sent_success = true;
            gr_storage_web buffer_write_storage[CONST_WRITE_BUFFER_SIZE];

#ifdef CONST_DEBUG
            cout << "http_transfer_sink_wait SDR processing THREAD was started ..." << endl;
#endif
            if(m_exit_requested){
#ifdef CONST_DEBUG            
            cout<< "http_transfer_sink_wait processing THREAD ends - clearing resources ... "<< endl;   
#endif
            return; 
            }
            
            // ---------------------
            // HTTP post request ...
            // ---------------------
            http::request<http::string_body> req;
            req.version(11);    // Default 11 ..
            req.method(http::verb::post);
            req.target(m_ServerTarget);
            req.set(http::field::host, m_ServerName);
            req.set(http::field::content_type, "application/json");
            
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

            
            // While TRUE loop ...
            // -------------------
            while(true){
             if(data_sent_success) niutput_items_real = m_fifo.fifo_read_storage(buffer_write_storage, CONST_WRITE_BUFFER_SIZE); 

             if(niutput_items_real > 0){
                try{ 
                    // Look up the domain name ...
                    auto const results = resolver.resolve(m_ServerName, m_ServerPort);

                    // Make the connection on the IP address we get from a lookup ...
#ifdef CONST_BOOST_1_7
                    stream.connect(results);
#else
                    boost::asio::connect(stream, results.begin(), results.end());
#endif               
                    
                    // Send JSON data ...
                    // ------------------ 
                    for(int i=0;i<niutput_items_real;i++){
                        // Set HTTP body ...
                        req.body() = buffer_write_storage[i].getString();
                        req.prepare_payload();
                        
                        // Send the HTTP request to the remote host
                        http::write(stream, req);

                        // Receive the HTTP response
                        http::read(stream, buffer, res);
                        
                        if(res.result_int() == 200){
                           data_sent_success = true;
#ifdef CONST_DEBUG_HTTP                          
                           std::cout<<"Packet was sent succesfully: "<<res.result()<<std::endl;
#endif
                        }else{
                          data_sent_success = false;
#ifdef CONST_DEBUG_HTTP                          
                           std::cout<<"Packet was not sent succesfully - error: "<<res.result()<<std::endl;
#endif                          
                          
                        }
                    }
                    
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
                    std::cout << "Warning: could not connect : " << e.what() << std::endl;
                }

                catch(std::exception const& e){
                    std::cerr << "Error: " << e.what() << std::endl; // return EXIT_FAILURE;
                }
            }
            
            // Waiting for data ...
            // --------------------
            boost::this_thread::sleep(boost::posix_time::milliseconds(CONST_SLEEP_INTERVAL_MILI_SECONDS));
                
            gr::thread::scoped_lock lock(fp_mutex);   // shared resources ...
            if(m_exit_requested) break;               // shared global variable - exit requested if m_exit_requested == TRUE --> break ...  
        }
    }
    
    // packet_handler - public function / callback ...
    // -----------------------------------------------
    void http_transfer_sink_impl::packet_handler(pmt::pmt_t msg){
        
        int packetSize = 0;
        
        // Extract payload ...
        // -------------------
        vector<unsigned char> data_uint8(pmt::u8vector_elements(pmt::cdr(msg)));
        packetSize = data_uint8.size();
        
#ifdef CONST_DEBUG           
        cout<<"Packet size: "<<packetSize<<endl;
#endif             
        
        // Extract meta information / verbose mode ...
        // -------------------------------------------
        pmt::pmt_t meta = pmt::car(msg);
        
        if(pmt::dict_has_key(meta, pmt::string_to_symbol("syncword"))) {
           pmt::pmt_t r = pmt::dict_ref(meta, pmt::string_to_symbol("syncword"), pmt::PMT_NIL); 
#ifdef CONST_DEBUG           
           cout<<"Syncword: "<<pmt::to_long(r)<<endl;
#endif           
        }
        
        // Create a ptree (json) root ...
        // ------------------------------
        pt::ptree oroot;
        
        // Create JSON ...
        // ---------------
        oroot.put("groundStation", 0); struct timeval tv; gettimeofday(&tv, NULL);       // ptime t = microsec_clock::universal_time();
        oroot.put("timestamp", long_to_string(tv.tv_sec * 1000 + tv.tv_usec / 1000));    // to_iso_extended_string(t)+"Z";
        oroot.put("crcOK",true);
        oroot.put("packetNumber",1);
        oroot.put("packetLength", packetSize);

        std::ostringstream ss;
        ss << std::hex << std::uppercase << std::setfill('0');
        std::for_each(data_uint8.cbegin(), data_uint8.cend(), [&](int c) {ss << std::setw(2) << c; });
        std::string dataResult; dataResult.append("\""); dataResult.append("0x"); dataResult.append(ss.str()); dataResult.append("\"");

        oroot.put("payload", dataResult);
        oroot.put("spacecraftId", 1);

        // Add to circular buffer ...
        // --------------------------
        gr_storage_web storage = gr_storage_web(0,create_json_string(oroot));
        m_fifo.fifo_write_storage(&storage, 1);
        
    }
    
  } /* namespace gsSDR */
} /* namespace gr */

