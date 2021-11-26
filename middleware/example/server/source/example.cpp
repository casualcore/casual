//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "xatmi.h"

#include "common/algorithm.h"
#include "common/domain.h"
#include "common/log.h"

#include <locale>

#include <iostream>
#include <sstream>
#include <cstring>

#include "common/process.h"
#include "common/chronology.h"

namespace casual
{
   using namespace common;
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
         //! * the implementation is way to C:ish
         //! * if we need to pass instructions (which I think we need) we should 
         //!    have a real model (data structures) to represent real meaning
         //!    and then serialize to and from json
         //!
         //! We'll fix this, but I don't have time right now. I did improve on the unittest that
         //! calls this service.
         //! 
         //! Good comments though.
         void casual_example_conversation_recv_send( TPSVCINFO* info)
         {
            // service  casual/example/conversation_recv_send
            // Conversational service that takes data in connect 
            // and any it gets via tprecv, concatenates them and send
            // with tpsend (if any data received). Then does a tpreply with a 
            // string representing the flags (in hex) present on service
            // entry.
            // 
            // End of data from connector is indicated by the transfer
            // of control of the conversation to the service. In essence
            // we collect data until we get control, send the data back and
            // terminate the conversation and tell the "connector" what
            // flags were set when the service was called.
            // 
            // The service is useful to check flags passed to service and
            // for experiments with a simple conversational "echo style"
            // service.
            
            std::string collected_data;
            // save (copy) any data provided in the call
            if( info->len > 0)  // not really needed, insert probably does nothing if info->len == 0
               collected_data.assign(info->data, info->len);

            auto receiving = ( ( info->flags & TPCONV) && ( info->flags & TPRECVONLY)) ? true : false;

            while( receiving) 
            {
               if( collected_data.find( "sleep before tprecv") != std::string::npos)
               {  
                  // This is a "hack" used by unit tests to delay the tprecv
                  // so that data or a disconnect arrives before the 
                  // tprecv() is called. This is to "force" a timing case.
                  // There is obviously not an absolute guarantee that
                  // the desired effect is achived, but in practice it should work.
                  common::process::sleep( 1s);
               }

               auto recv_buffer = tpalloc( X_OCTET, nullptr, 128);
               long event=0;
               long recv_len = tptypes( recv_buffer, nullptr, nullptr);
               auto recv_return = tprecv( info->cd, &recv_buffer, &recv_len, TPSIGRSTRT, &event);

               // Data possible?
               if( recv_return == 0 || (recv_return == -1 && tperrno == TPEEVENT && event == TPEV_SENDONLY))
               {
                  // nothing appended if recv_len == 0
                  collected_data.append( recv_buffer, recv_len);
               }

               // finished with this receive buffer
               // (could allocate outside of loop and reuse) 
               tpfree( recv_buffer); 

               if( recv_return == -1 && tperrno == TPEEVENT)
               {
                  // handle events...
                  if( event == TPEV_SENDONLY)
                  {
                     // Transfer of comntrol of conversation
                     receiving = false;
                  }
                  else if( event == TPEV_DISCONIMM)
                  {
                     // This is "kind of normal" in the sense that it can
                     // occur but is exceptional. Possible reasons are:
                     // * tpdiscon() by initiator (no well designed appl should
                     //   use tpdiscon())
                     // * initiator "terminates" (eg tpreturn()) with active
                     //   conversation
                     // * communication failures or other system level failures
                     //   that cause the party at the other end to "disappear".
                     // In any case the only reasonable thing to do is a
                     // tpreturn() with service failure. The
                     // conversation "descriptor" is already "invalid" and any
                     // active transaction will be rolled back.
                     //
                     // There is no simple way for a unit test to "check" that
                     // a called service does something reasonable when it
                     // gets a TPEV_DISCONIMM, but we log something to allow
                     // for manual checking
                     log::line( log::debug, "casual_example_conversation_recv_send got TPEV_DISCONIMM");
                     tpreturn( TPFAIL, 0, nullptr, 0, 0);
                  } 
                  else
                  {
                     // all other events (TPEV_SVCERR, _SVCSUCC or _SVCFAIL)
                     // should only be possible in "originator" of a
                     // conversation.
                     // What to do if it happens? for now we return to casual
                     // with a "return" instead of tpreturn(). This is
                     // "abnormal".
                     log::line( log::category::error, "casual_example_conversation_recv_send got unexpected event from tprecv - tperrno: ",
                        tperrnostring( tperrno), " event: ", event);

                     return; // abnormal(!) service return
                  }
               }
               else if( recv_return == -1)
               {
                  // Got an error but not TPEEVENT. This is "unexpected"
                  // we log and tpreturn with failure.
                  log::line( log::category::error, "casual_example_conversation_recv_send got unexpected return status from tprecv - tperrno: ",
                        tperrnostring( tperrno), " event: ", event);
                  tpreturn( TPFAIL, 0, nullptr, 0, 0);
               }
            } // while(receiving) <-- if this is needed the control flow is way to big! (which it is...)

            // ww have got control of conversation. If any data has
            // been collected send it back.
            if( ! collected_data.empty()) 
            {
               auto send_buffer = tpalloc( X_OCTET, nullptr, collected_data.length());
               auto send_buffer_len = tptypes( send_buffer, nullptr, nullptr);
               
               common::algorithm::copy( collected_data, send_buffer);

               long event = 0;
               auto retval = tpsend( info->cd, send_buffer, send_buffer_len, TPSIGRSTRT, &event);
               if( retval == -1) 
               {
                  // unexpected failure. One possibility is to write some
                  // information and exit/return. We can also 
                  // do a tpreturn with service failure, but if a tpsend
                  // fails when we have just gained control of the
                  // conversation is is unlikely to be useful to the
                  // other end, but it will terminate the service call in
                  // and "orderly" fashion!
                  log::line( log::debug, "conversation_recv_send: tpsend failed. tperrno: ", tperrno, " event: ", event);
                  tpfree( send_buffer);
                  tpreturn( TPFAIL, 0, nullptr, 0, 0);              
               }
               tpfree( send_buffer);
            }
            // terminate service/conversation
            std::ostringstream oss { "Flags: ", std::ios::ate};
            oss << std::hex << "0x" << std::setfill('0') << std::setw(8) << info->flags;
            auto tpreturn_buffer = tpalloc( X_OCTET, nullptr, oss.str().length());

            std::string string = std::move( oss).str();

            common::algorithm::copy( string, tpreturn_buffer);
            tpreturn( TPSUCCESS, 0, tpreturn_buffer, tptypes( tpreturn_buffer, nullptr, nullptr), 0);
         }
      }

   } // example::server
} // casual

