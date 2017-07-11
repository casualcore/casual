//!
//! casual 
//!

#ifndef CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_EVENT_LISTENER_H_
#define CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_EVENT_LISTENER_H_

#include "common/message/event.h"

#include "common/communication/ipc.h"


namespace casual
{
   namespace common
   {
      namespace event
      {

         using device_type = common::communication::ipc::inbound::Device;
         using handler_type = device_type::handler_type;

         namespace detail
         {
            handler_type subscribe( handler_type&& handler);

            void listen( device_type& device, handler_type&& handler);
            void listen( device_type& device, std::function< void()> empty, handler_type&& handler);

         } // detail

         //!
         //! Register and start listening on events.
         //!
         template< typename... Callback>
         void listen( device_type& device, Callback&&... callbacks)
         {
            detail::listen( device, device.handler( std::forward< Callback>( callbacks)...));
         }

         //!
         //! Register and start listening on events on the default inbond queue.
         //!
         template< typename... Callback>
         void listen( Callback&&... callbacks)
         {
            listen( communication::ipc::inbound::device(), std::forward< Callback>( callbacks)...);
         }


         namespace idle
         {
            template< typename... Callback>
            void listen( device_type& device, std::function< void()> empty, Callback&&... callbacks)
            {
               detail::listen( device, empty, device.handler( std::forward< Callback>( callbacks)...));
            }

            template< typename... Callback>
            void listen( std::function< void()> empty, Callback&&... callbacks)
            {
               listen( communication::ipc::inbound::device(), std::move( empty), std::forward< Callback>( callbacks)...);
            }
         } // idle

         namespace no
         {
            namespace subscription
            {
               namespace detail
               {
                  void listen( device_type& device, handler_type&& handler);
               } // detail

               template< typename... Callback>
               void listen( device_type& device, Callback&&... callbacks)
               {
                  detail::listen( device, { std::forward< Callback>( callbacks)...});
               }
            } // subscription
         } // no


         template< typename... Callback>
         handler_type listener( Callback&&... callbacks)
         {
            return detail::subscribe( { std::forward< Callback>( callbacks)...});
         }


      } // event
   } // common
} // casual

#endif // CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_EVENT_LISTENER_H_
