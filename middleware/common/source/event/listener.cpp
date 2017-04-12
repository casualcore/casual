//!
//! casual 
//!

#include "common/event/listener.h"

#include "common/message/handle.h"

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
               using message_type = common::message::Type;

               void subscribe( const process::Handle& process, std::vector< message_type> types)
               {

                  message::event::subscription::Begin request;

                  request.process = process;
                  request.types = std::move( types);

                  communication::ipc::blocking::send(
                        communication::ipc::domain::manager::device(),
                        request);

               }
            } // <unnamed>
         } // local


         Listener::Listener() = default;
         Listener::~Listener() = default;


         void Listener::listen( handler_type&& handler)
         {
            local::subscribe( process::Handle{ process::id(), m_device.connector().id()}, handler.types());



         }

         namespace detail
         {
            handler_type subscribe( handler_type&& handler)
            {
               Trace trace{ "common::event::detail::subscribe"};

               local::subscribe( process::handle(), handler.types());


               return std::move( handler);
            }
         } // detail

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
