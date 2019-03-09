//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/event/listen.h"

#include "common/execute.h"
#include "common/message/handle.h"

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
                  void domain( const M& request, const std::vector< message_type>& types)
                  {
                     signal::thread::scope::Mask block{ signal::set::filled( signal::Type::terminate, signal::Type::interrupt)};

                     if( algorithm::find_if( types, []( message_type type){
                        return type >= message_type::EVENT_DOMAIN_BASE && type < message_type::EVENT_DOMAIN_BASE_END;}))
                     {
                        communication::ipc::blocking::send(
                              communication::instance::outbound::domain::manager::optional::device(),
                              request);
                     }
                  }

                  template< typename M>
                  void service( const M& request, const std::vector< message_type>& types)
                  {
                     signal::thread::scope::Mask block{ signal::set::filled( signal::Type::terminate, signal::Type::interrupt)};

                     if( algorithm::find_if( types, []( message_type type){
                        return type >= message_type::EVENT_SERVICE_BASE && type < message_type::EVENT_SERVICE_BASE_END;}))
                     {
                        communication::ipc::blocking::send(
                              communication::instance::outbound::service::manager::device(),
                              request);
                     }
                  }

               } // location

               void subscribe( const process::Handle& process, std::vector< message_type> types)
               {
                  log::line( log::debug, "subscribe - process: ", process, ", types: ", types);

                  message::event::subscription::Begin request;

                  request.process = process;
                  request.types = std::move( types);

                  location::domain( request, request.types);
                  location::service( request, request.types);
               }

               auto subscription( strong::ipc::id ipc, std::vector< message_type> types)
               {
                  process::Handle process{ process::id(), ipc};

                  local::subscribe( process, types);

                  return execute::scope( [&](){
                     event::unsubscribe( process, types);
                  });
               }

               namespace standard
               {
                  handler_type handler( handler_type&& handler)
                  {
                     handler.insert(
                           common::message::handle::Shutdown{},
                           common::message::handle::Ping{});

                     return std::move( handler);
                  }
               } // standard


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

            void listen( device_type& device, handler_type&& h)
            {
               Trace trace{ "common::event::detail::listen"};

               auto handler = local::standard::handler( std::move( h));

               auto subscription = local::subscription( device.connector().handle().ipc(), handler.types());

               while( true)
               {
                  handler( device.next( typename device_type::blocking_policy{}));
               }

            }

            void listen( device_type& device, std::function< void()> empty, handler_type&& h)
            {
               Trace trace{ "common::event::detail::listen"};

               auto handler = local::standard::handler( std::move( h));

               auto subscription = local::subscription( device.connector().handle().ipc(), handler.types());

               while( true)
               {
                  while( handler( device.next( typename device_type::non_blocking_policy{})))
                  {
                     ;
                  }

                  // queue is empty, notify user
                  empty();

                  handler( device.next( typename device_type::blocking_policy{}));
               }

            }
         } // detail

         void subscribe( const process::Handle& process, std::vector< message_type> types)
         {
            Trace trace{ "common::event::subscribe"};

            local::subscribe( process, std::move( types));
         }

         void unsubscribe( const process::Handle& process, std::vector< message_type> types)
         {
            Trace trace{ "common::event::unsubscribe"};

            log::line( log::debug, "unsubscribe - process: ", process, ", types: ", types);

            message::event::subscription::End request;
            request.process = process;

            

            local::location::domain( request, types);
            local::location::service( request, types);
         }

         namespace no
         {
            namespace subscription
            {
               namespace detail
               {
                  void listen( device_type& device, handler_type&& handler)
                  {
                     handler.insert(
                           common::message::handle::Shutdown{},
                           common::message::handle::Ping{});

                     message::dispatch::blocking::pump( handler, device);
                  }
               } // detail
            } // subscription
         } // no

      } // event
   } // common
} // casual
