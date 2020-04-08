//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/event/listen.h"

#include "common/execute.h"
#include "common/message/handle.h"
#include "common/message/dispatch.h"

#include "common/communication/instance.h"

namespace casual
{
   namespace common
   {

      namespace event
      {

         namespace local
         {
            namespace
            {

               namespace location
               {
                  template< typename M>
                  void domain( const M& request, const std::vector< message::Type>& types)
                  {
                     signal::thread::scope::Mask block{ signal::set::filled( code::signal::terminate, code::signal::interrupt)};

                     if( algorithm::find_if( types, []( message::Type type){
                        return type >= message::Type::EVENT_DOMAIN_BASE && type < message::Type::EVENT_DOMAIN_BASE_END;}))
                     {
                        communication::ipc::blocking::optional::send(
                              communication::instance::outbound::domain::manager::optional::device(),
                              request);
                     }
                  }

                  template< typename M>
                  void service( const M& request, const std::vector< message::Type>& types)
                  {
                     signal::thread::scope::Mask block{ signal::set::filled( code::signal::terminate, code::signal::interrupt)};

                     if( algorithm::find_if( types, []( message::Type type){
                        return type >= message::Type::EVENT_SERVICE_BASE && type < message::Type::EVENT_SERVICE_BASE_END;}))
                     {
                        communication::ipc::blocking::optional::send(
                              communication::instance::outbound::service::manager::device(),
                              request);
                     }
                  }

               } // location

               void subscribe( const process::Handle& process, std::vector< message::Type> types)
               {
                  log::line( log::debug, "subscribe - process: ", process, ", types: ", types);

                  message::event::subscription::Begin request;

                  request.process = process;
                  request.types = std::move( types);

                  location::domain( request, request.types);
                  location::service( request, request.types);
               }

            } // <unnamed>
         } // local



         namespace detail
         {
            handler_type subscribe( handler_type&& handler)
            {
               Trace trace{ "common::event::detail::subscribe"};

               local::subscribe( process::handle(), handler.types());

               return std::move( handler);
            }
         } // detail

         void subscribe( const process::Handle& process, std::vector< message::Type> types)
         {
            Trace trace{ "common::event::subscribe"};

            local::subscribe( process, std::move( types));
         }

         void unsubscribe( const process::Handle& process, std::vector< message::Type> types)
         {
            Trace trace{ "common::event::unsubscribe"};

            log::line( log::debug, "unsubscribe - process: ", process, ", types: ", types);

            message::event::subscription::End request;
            request.process = process;


            local::location::domain( request, types);
            local::location::service( request, types);
         }

      } // event
   } // common
} // casual
