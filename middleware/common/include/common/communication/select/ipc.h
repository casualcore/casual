//! 
//! Copyright (c) 2022, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/communication/ipc.h"

namespace casual
{
   namespace common::communication::select::ipc
   {
      namespace dispatch
      {         
         namespace detail
         {
            template< typename H, typename Policy>
            struct basic_dispatcher
            {
               basic_dispatcher( H handler) : m_handler{ std::move( handler)} {}
               
               bool operator () ( tag::consume)
               {
                  // we try to consume the cache.
                  if( ! m_handler( communication::ipc::inbound::device().cached()))
                     return false;

                  while( m_handler( communication::ipc::inbound::device().cached()))
                     ;
                  
                  return true;
               }

               bool operator() ( strong::file::descriptor::id descriptor, tag::read)
               {
                  if( communication::ipc::inbound::device().descriptor() != descriptor)
                     return false;

                  auto count = Policy::next::ipc;
                  while( count-- > 0 && m_handler( device::non::blocking::next( communication::ipc::inbound::device())))
                     ; // no-op

                  return true;
               }

            private:
               H m_handler;
            };

            namespace policy
            {
               struct Default
               {
                  struct next
                  {
                     static constexpr platform::size::type ipc = platform::batch::message::pump::next;
                  };
               };
            } // policy

            template< typename Policy, typename State, typename HC>
            auto create( State& state, HC&& handler_creator)
            {
               using handler_type = decltype( handler_creator( state));

               // make sure we set the select fd-set
               state.directive.read.add( communication::ipc::inbound::device().descriptor());

               return detail::basic_dispatcher< handler_type, Policy>( handler_creator( state));
            }
         } // detail

         template< typename Policy, typename State, typename HC>
         auto create( State& state, HC&& handler_creator) -> decltype( detail::create< Policy>( state, handler_creator))
         {
            return detail::create< Policy>( state, handler_creator);
         }

         template< typename State, typename HC>
         auto create( State& state, HC&& handler_creator) -> decltype( detail::create< detail::policy::Default>( state, handler_creator))
         {
            return detail::create< detail::policy::Default>( state, handler_creator);
         }
         
      } // dispatch
   } // common::communication::select::ipc
} // casual