//!
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "domain/discovery/api.h"
#include "domain/discovery/common.h"
#include "domain/discovery/instance.h"
#include "domain/message/discovery.h"

#include "common/communication/instance.h"
#include "common/communication/ipc/flush/send.h"

namespace casual
{
   using namespace common;
   namespace domain::discovery
   {
      namespace local
      {
         namespace
         {
            namespace instance
            {
               auto& device()
               {
                  static communication::instance::outbound::detail::optional::Device device{ discovery::instance::identity};
                  return device;
               }

            } // instance
         } // <unnamed>
      } // local

      namespace internal
      {
         void registration( const common::process::Handle& process)
         {
            Trace trace{ "domain::discovery::internal::registration"};
            message::discovery::internal::Registration message{ process};
            communication::ipc::flush::optional::send( local::instance::device(), message);
         }

         void registration()
         {
            registration( process::handle());
         }


      } // internal
      
      correlation_type request( const Request& request)
      {
         Trace trace{ "domain::discovery::request"};
         return communication::ipc::flush::optional::send( local::instance::device(), request);
      }

      namespace external
      {
         void registration( const common::process::Handle& process, Directive directive)
         {
            Trace trace{ "domain::discovery::external::registration"};
            message::discovery::external::Registration message{ process};
            message.directive = directive;
            communication::ipc::flush::optional::send( local::instance::device(), message);
         } 

         void registration( Directive directive)
         {
            registration( process::handle(), directive);
         }

         correlation_type request( const Request& request)
         {
            Trace trace{ "domain::discovery::external::request"};
            return communication::ipc::flush::optional::send( local::instance::device(), request);
         }

      } // external

      namespace rediscovery
      {

         correlation_type request()
         {
            Trace trace{ "domain::discovery::rediscovery::request"};
            message::discovery::rediscovery::Request message{ process::handle()};
            return communication::ipc::flush::optional::send( local::instance::device(), message);
         }

         namespace blocking
         {
            void request()
            {
               Trace trace{ "domain::discovery::rediscovery::blocking::request"};
               if( auto correlation = rediscovery::request())
               {
                  Reply reply;
                  communication::device::blocking::receive( communication::ipc::inbound::device(), reply, correlation);
               }

            }

         } // blocking
      } // rediscovery

   } // domain::discovery  
} // casual

