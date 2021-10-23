//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "xatmi.h"

#include "common/algorithm.h"
#include "common/domain.h"

#include <locale>

#include <iostream>
#include <sstream>
#include <cstring>

// temp
#include "common/process.h"
#include "common/chronology.h"
// end temp

namespace casual
{
   namespace example::server
   {
      extern "C"
      {

         void casual_example_echo( TPSVCINFO* info)
         {
            tpreturn( TPSUCCESS, 0, info->data, info->len, 0);
         }

         void casual_example_domain_name( TPSVCINFO* info)
         {
            auto buffer = ::tpalloc( X_OCTET, nullptr, common::domain::identity().name.size() + 1);

            common::algorithm::copy( common::domain::identity().name, buffer);
            buffer[ common::domain::identity().name.size()] = '\0';

            tpreturn( TPSUCCESS, 0, buffer, common::domain::identity().name.size() + 1, 0);
         }

         void casual_example_forward_echo( TPSVCINFO* info)
         {
            casual_service_forward( "casual/example/echo", info->data, info->len);
         }

         void casual_example_conversation( TPSVCINFO* info)
         {
            tpreturn( TPSUCCESS, 0, info->data, info->len, 0);
         }

         void casual_example_sink( TPSVCINFO* info)
         {
            tpreturn( TPSUCCESS, 0, nullptr, 0, 0);
         }
         
         void casual_example_uppercase( TPSVCINFO* info)
         {
            auto buffer = common::range::make( info->data, info->len);

            common::algorithm::transform( buffer, buffer, ::toupper);

            tpreturn( TPSUCCESS, 0, info->data, info->len, 0);
         }

         void casual_example_lowercase( TPSVCINFO* info)
         {
            auto buffer = common::range::make( info->data, info->len);

            common::algorithm::transform( buffer, buffer, ::tolower);

            tpreturn( TPSUCCESS, 0, info->data, info->len, 0);
         }

         void casual_example_rollback( TPSVCINFO* info)
         {
            tpreturn( TPFAIL, 0, info->data, info->len, 0);
         }

         void casual_example_error_system( TPSVCINFO* info)
         {
            throw std::runtime_error{ "throwing from service"};
         }

         void casual_example_terminate( TPSVCINFO* info)
         {
            std::terminate();
         }

         //! TODO maintainence:
         //! * we can't have a service named <some-name>2
         //! * has to have some name that give some guidance what it does
         //! * the implementation is way to C:ish
         //! * if we need to pass instructions (which I think we need) we should 
         //!    have a real model (data structures) to represent real meaning
         //!    and then serialize to and from json
         //!
         //! We'll fix this, but I don't have time right now. I did improve on the unittest that
         //! calls this "2"
         //! 
         //! Good comments though.
         void casual_example_conversation2( TPSVCINFO* info)
         {
            /* service  casual/example/conversation2
               Conversational service that takes data in connect 
               and any it gets via tprecv, concatenates them and send
               with tpsend. Then does a tpreply with a 
               string representing the flags (in hex) present on service
               entry.
               
               End of data from connector is indicated by the transfer
               of control of the conversation to the service. In essence
               we collect data until we get control, send the data back and
               terminate the conversation and tell the "connector" what
               flags were set when the service was called.

               The service is useful to check flags passed to service and
               for experiments with a simple conversational "echo style"
               service.
            */

            std::string collected_data;
            // save (copy) any data provided in the call
            if (info->len > 0) { // not really needed, insert probably does nothing if info->len == 0
               collected_data.assign(info->data, info->len);
            }

            auto receiving = ( (info->flags & TPCONV) &&
                                 (info->flags & TPRECVONLY) ) ? true : false;

            while (receiving) {
               auto recv_buffer=tpalloc(X_OCTET, nullptr, 128);
               long event=0;
               long recv_len = tptypes(recv_buffer, nullptr, nullptr);
               auto recv_return=tprecv(info->cd,
                                       &recv_buffer, &recv_len,
                                       TPSIGRSTRT,
                                       &event);
               // Transfer of control of conversation?
               if (recv_return == -1 &&
                  tperrno == TPEEVENT &&
                  event == TPEV_SENDONLY)
               {
                     receiving = false;
               }
               // Data possible?
               if (recv_return == 0 || 
                     (recv_return == -1 && tperrno==TPEEVENT && event == TPEV_SENDONLY) )
               {
                  // nothing appended if recv_len == 0
                  collected_data.append(recv_buffer, recv_len);
               }
               tpfree(recv_buffer);                       
            } // while(receiving)
            // ww have got control of conversation. If any data has
            // been collected send it back.
            if (collected_data.length() > 0) {
               auto send_buffer = tpalloc(X_OCTET, nullptr, collected_data.length());
               auto send_buffer_len=tptypes(send_buffer, nullptr, nullptr);
               
               // tpsend is a C api so use C-style copy to fill in buffer :-)
               memcpy(send_buffer, &collected_data[0], send_buffer_len);
               long event=0;
               auto retval=tpsend(info->cd,
                                    send_buffer, send_buffer_len,
                                    TPSIGRSTRT,
                                    &event);
               if (retval == -1) {
                  // unexpected failure. One possibility is to write some
                  // information and exit/return. We can also 
                  // do a tpreturn with service failure, but if a tpsend
                  // fails when we have just gained control of the
                  // conversation is is unlikely to be useful to the
                  // other end, but it will terminate the service call in
                  // and "orderly" fashion!
                  // Use cerr instead? 
                  std::cout << "conversation2: tpsend failed. tperrno: "
                              << tperrno
                              << " event: " << event
                              << std::endl;
                  tpfree(send_buffer);
                  tpreturn(TPFAIL, 0, nullptr, 0, 0);              
               }
               tpfree(send_buffer);
            }
            // terminate service/conversation
            std::ostringstream oss {"Flags: ", std::ios::ate};
            oss << std::hex << "0x" << std::setfill('0') << std::setw(8) << info->flags;
            //std::cout << "tpreply buffer:" << oss.str() 
            //          << " oss.str().length(): " << oss.str().length() 
            //          << std::endl << std::flush;
            auto tpreturn_buffer = tpalloc(X_OCTET, nullptr, oss.str().length());
            // Copy in "C-style" to C-style buffer (character array with C-string, no terminating \0)
            //memcpy(tpreturn_buffer, oss.str().c_str(), oss.str().length());
            //casual::platform::time::unit sleep_time{60s};
            //common::process::sleep(sleep_time);

            std::string s = oss.str();

            common::algorithm::copy(common::range::make(s.begin(), s.end()), tpreturn_buffer);
            tpreturn( TPSUCCESS, 0, tpreturn_buffer, tptypes(tpreturn_buffer, nullptr, nullptr), 0);
         }
      }

   } // example::server
} // casual

