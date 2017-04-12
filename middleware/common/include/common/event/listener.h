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
         struct Listener
         {
            Listener();
            ~Listener();

            using device_type = common::communication::ipc::inbound::Device;
            using handler_type = device_type::handler_type;


            template< typename... Callback>
            void listen( Callback&&... callbacks)
            {
               listen( m_device.handler( std::forward< Callback>( callbacks)...));
            }

         private:

            void listen( handler_type&& handler);

            device_type m_device;
         };



         using device_type = common::communication::ipc::inbound::Device;
         using handler_type = device_type::handler_type;

         namespace detail
         {
            handler_type subscribe( handler_type&& handler);

            void listen( handler_type&& handler);


         } // detail

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
