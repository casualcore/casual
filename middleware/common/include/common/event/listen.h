//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


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
         using message_type = common::message::Type;

         namespace detail
         {
            handler_type subscribe( handler_type&& handler);

            void listen( device_type& device, handler_type&& handler);
            void listen( device_type& device, std::function< void()> empty, handler_type&& handler);

         } // detail

         void subscribe( const process::Handle& process, std::vector< message_type> types);
         void unsubscribe( const process::Handle& process, std::vector< message_type> types);

         //! Register and start listening on events.
         template< typename... Callback>
         void listen( device_type& device, Callback&&... callbacks)
         {
            detail::listen( device, device.handler( std::forward< Callback>( callbacks)...));
         }

         //! Register and start listening on events on the default inbound queue.
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


