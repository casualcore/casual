//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "common/message/event.h"
#include "common/message/handle.h"

#include "common/communication/ipc.h"
#include "common/execute.h"
#include "common/functional.h"


namespace casual
{
   namespace common
   {
      namespace event
      {

         using device_type = common::communication::ipc::inbound::Device;
         using handler_type = decltype( message::dispatch::handler( std::declval< device_type&>()));


         namespace detail
         {
            handler_type subscribe( handler_type&& handler);

         } // detail

         void subscribe( const process::Handle& process, std::vector< message::Type> types);
         void unsubscribe( const process::Handle& process, std::vector< message::Type> types);

         namespace scope
         {
            inline auto unsubscribe( std::vector< message::Type> types)
            {
               return common::execute::scope( [types = std::move( types)](){ event::unsubscribe( process::handle(), std::move( types));});
            }

            inline auto subscribe( const process::Handle& process, std::vector< message::Type> types)
            {
               event::subscribe( process, types);
               return unsubscribe( std::move( types));
            }

            inline auto subscribe( std::vector< message::Type> types) 
            { 
               return subscribe( process::handle(), std::move( types));
            }
            
         } // scope

         namespace only
         {
            namespace unsubscribe
            {
               template< typename C, typename... Callback>
               void listen( C&& condition, Callback&&... callbacks)
               {
                  auto& device = communication::ipc::inbound::device();
                  auto handler = handler_type{ std::forward< Callback>( callbacks)...} + common::message::handle::defaults( device);
                  auto unsubscription = scope::unsubscribe( handler.types());
                  common::message::dispatch::relaxed::pump( std::forward< C>( condition), handler, device);
               }
            } // subscription
         } // no

         namespace condition
         {
            using namespace common::message::dispatch::condition;

         } // condition

         template< typename C, typename... Callback>
         void listen( C&& condition, Callback&&... callbacks)
         {
            auto& device = communication::ipc::inbound::device();
            auto handler = handler_type{ std::forward< Callback>( callbacks)...};
            auto subscription = scope::subscribe( handler.types());
            handler += common::message::handle::defaults( device); 
            common::message::dispatch::relaxed::pump( std::forward< C>( condition), handler, device);
         }


         template< typename... Callback>
         handler_type listener( Callback&&... callbacks)
         {
            return detail::subscribe( { std::forward< Callback>( callbacks)...});
         }


      } // event
   } // common
} // casual


