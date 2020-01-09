//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "common/message/event.h"

#include "common/communication/ipc.h"
#include "common/execute.h"


namespace casual
{
   namespace common
   {
      namespace event
      {

         using device_type = common::communication::ipc::inbound::Device;
         using handler_type = device_type::handler_type;
         using message_type = common::message::Type;
         using error_type = typename device_type::error_type;

         namespace detail
         {
            handler_type subscribe( handler_type&& handler);

            void listen( device_type& device, handler_type&& handler);
            void listen( device_type& device, std::function< void()> empty, handler_type&& handler, const error_type& error = nullptr);

         } // detail

         void subscribe( const process::Handle& process, std::vector< message_type> types);
         void unsubscribe( const process::Handle& process, std::vector< message_type> types);

         namespace scope
         {
            inline auto subscribe( const process::Handle& process, std::vector< message_type> types)
            {
               event::subscribe( process, types);
               return common::execute::scope( [&process, types = std::move( types)](){ event::unsubscribe( process, types);});
            }
         } // scope

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

            template< typename Device, typename... Callback>
            auto listen( Device& device, std::function< void()> empty, Callback&&... callbacks) 
               ->  decltype( detail::listen( device.device(), empty, device.handler( std::forward< Callback>( callbacks)...), device.error_handler()))
            {
               detail::listen( device.device(), empty, device.handler( std::forward< Callback>( callbacks)...), device.error_handler());
            }

            template< typename... Callback>
            void listen( std::function< void()> empty, Callback&&... callbacks)
            {
               listen( communication::ipc::inbound::device(), std::move( empty), std::forward< Callback>( callbacks)...);
            }
         } // idle

         namespace conditional
         {
            template< typename... Callback>
            void listen( std::function< bool()> condition, Callback&&... callbacks)
            {
               detail::listen( communication::ipc::inbound::device(), std::move( condition), std::forward< Callback>( callbacks)...);
            }

         } // conditional

         namespace no
         {
            namespace subscription
            {
               namespace detail
               {
                  void listen( device_type& device, handler_type&& handler);
                  void conditional( device_type& device, std::function< bool()> done, handler_type&& handler);
               } // detail

               template< typename... Callback>
               void listen( device_type& device, Callback&&... callbacks)
               {
                  detail::listen( device, { std::forward< Callback>( callbacks)...});
               }

               //! blocks until `done` is true
               template< typename... Callback>
               void conditional( std::function< bool()> done, Callback&&... callbacks)
               {
                  detail::conditional( communication::ipc::inbound::device(), std::move( done), { std::forward< Callback>( callbacks)...});
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


