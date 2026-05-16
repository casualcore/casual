//!
//! Copyright (c) 2025, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "casual/manager/service/invoke.h"
#include "casual/manager/service.h"

#include "common/serialize/service/protocol.h"

namespace casual
{
   namespace manager::service::protocol
   {
      struct Concurrent
      {
         common::serialize::service::Protocol protocol;
         invoke::concurrent::callback_function_type callback;
      };

      common::serialize::service::Protocol deduce( invoke::Parameter&& parameter);
      Concurrent deduce( invoke::concurrent::Parameter&& parameter);

      //! a wrapper for common::serialize::service::user to be used in managers
      //! and use the Parameter and Result types
      template< typename... Ts>
      invoke::Result dispatch( invoke::Parameter&& parameter, Ts&&... ts)
      {
         invoke::Result result;
         result.payload = common::serialize::service::user( 
            std::move( parameter.payload),
            std::forward< Ts>( ts)...);

         return result;
      }

      template< typename F, typename... Ts>
      invoke::Result dispatch( common::serialize::service::Protocol&& protocol, F&& function, Ts&&... ts)
      {
         invoke::Result result;
         result.payload = common::serialize::service::user( 
            std::move( protocol), std::forward< F>( function), 
            std::forward< Ts>( ts)...);

         return result;
      }

      namespace concurrent
      {


         struct Protocol : common::serialize::service::Protocol
         {
            using common::serialize::service::Protocol::Protocol;

            invoke::concurrent::callback_function_type callback;
         };

         template< typename R>
         struct Finalize
         {
            using argument_type = R;

            void operator () ( argument_type&& argument)
            {
               invoke::Result result;

               concurrent.protocol << CASUAL_NAMED_VALUE_NAME( argument, "result");
               result.payload = concurrent.protocol.finalize();

               std::invoke( concurrent.callback, std::move( result));
            }

            protocol::Concurrent concurrent;
         };

         template<>
         struct Finalize< void> 
         {
            using argument_type = void;

            void operator () ()
            {
               invoke::Result result;

               result.payload = concurrent.protocol.finalize();

               std::invoke( concurrent.callback, std::move( result));
            }

            protocol::Concurrent concurrent;
         };


         namespace detail
         {
            template< typename F>
            constexpr auto deduce_finalize( protocol::Concurrent&& concurrent, F& function)
            {
               using traits_type = common::traits::function< F>;
               using finalize_type = std::remove_cvref_t< typename traits_type::template argument_t< 0>>; 

               return finalize_type{ std::move( concurrent) };
            }
         } // detail

         template< typename F, typename... Ts>
         void dispatch( protocol::Concurrent&& concurrent, F&& function, Ts&&... ts)
         {
            auto finalize = detail::deduce_finalize( std::move( concurrent), function);

            if( finalize.concurrent.protocol.call())
            {
               std::invoke( function, std::move( finalize), std::forward< Ts>( ts)...);
            }
            else
            {
               // we are called in _non invoke_ context. We don't call, but we need to finalize
               // the protocol

               using argument_type = typename decltype( finalize)::argument_type;

               if constexpr( std::is_void_v< argument_type>)
               {
                  finalize();
               }
               else
               {
                  finalize( argument_type{});
               }
            }
         }

      } // concurrent      
   } // manager::service::protocol
   
} // casual
