//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "common/message/event.h"
#include "common/message/dispatch/handle.h"

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

         void subscribe( const process::Handle& process, std::vector< message::Type> types);
         void unsubscribe( const process::Handle& process, std::vector< message::Type> types);

         namespace detail
         {
            handler_type subscribe( handler_type&& handler);

            namespace unsubscribe 
            {
               template< typename Callback>
               struct Wrapper : Callback
               {
                  explicit Wrapper( Callback callback) : Callback{ std::move( callback)} 
                  {}

                  Wrapper( Wrapper&&) noexcept = default;
                  Wrapper& operator = ( Wrapper&&) noexcept = default;

                  ~Wrapper()
                  {
                     if( ! m_active)
                        return;

                     using traits_type = traits::function< Callback>;
                     using message_type = std::remove_cvref_t< typename traits_type::template argument_t< 0>>;

                     event::unsubscribe( process::handle(), { message_type::type()});
                  }
               private:
                  move::Active m_active;
               };

               template< typename... Callback>
               handler_type wrapper( Callback&&... callbacks)
               {
                  handler_type result;

                  ( result.insert( Wrapper< Callback>( std::forward< Callback>( callbacks))) , ... );

                  return detail::subscribe( std::move( result));
               }
               
            } // unsubscribe 

            

         } // detail

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
                  auto handler = handler_type{ std::forward< Callback>( callbacks)...};
                  auto unsubscription = scope::unsubscribe( handler.types());
                  common::message::dispatch::relaxed::pump( 
                     std::forward< C>( condition), 
                     common::message::dispatch::handle::defaults() + std::move( handler),
                     device);
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
            Trace trace{ "common::event::listen"};
            
            auto& device = communication::ipc::inbound::device();
            auto handler = handler_type{ std::forward< Callback>( callbacks)...};
            auto subscription = scope::subscribe( handler.types());

            common::message::dispatch::relaxed::pump( 
               std::forward< C>( condition),
               common::message::dispatch::handle::defaults() + std::move( handler),
               device);
         }


         template< typename... Callback>
         handler_type listener( Callback&&... callbacks)
         {
            return detail::unsubscribe::wrapper( std::forward< Callback>( callbacks)...);
         }


      } // event
   } // common
} // casual


