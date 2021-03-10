//!
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/communication/ipc.h"
#include "common/communication/instance.h"
#include "common/communication/select.h"

namespace casual
{
   namespace gateway::group::ipc
   {
      
      namespace manager
      {
         inline auto& service() { return common::communication::instance::outbound::service::manager::device();}
         inline auto& transaction() { return common::communication::instance::outbound::transaction::manager::device();}
         inline auto& gateway() { return common::communication::instance::outbound::gateway::manager::device();}
         namespace optional
         {
            inline auto& queue() { return common::communication::instance::outbound::queue::manager::optional::device();}
         } // optional

      } // manager

      inline auto& inbound() { return common::communication::ipc::inbound::device();}

      namespace flush
      {
         template< typename D, typename M>
         auto send( D&& destination, M&& message)
         {
            auto result = common::communication::device::non::blocking::send( destination, message);
            while( ! result)
            {
               ipc::inbound().flush();
               result = common::communication::device::non::blocking::send( destination, message);
            }
            return result;
         }

         namespace optional
         {
            template< typename D, typename M>
            auto send( D&& destination, M&& message) -> decltype( flush::send( destination, std::forward< M>( message)))
            {
               try 
               {
                  return flush::send( destination, std::forward< M>( message));
               }
               catch( ...)
               {
                  if( common::exception::error().code() != common::code::casual::communication_unavailable)
                     throw;

                  common::log::line( common::communication::log, common::code::casual::communication_unavailable, " failed to send message - action: ignore");
                  return {};
               }
            }
         } // optional
      } // flush

      namespace dispatch
      {
         namespace detail
         {
            template< typename H>
            struct select_handler
            {
               select_handler( H handler) : m_handler{ std::move( handler)} {}
               
               bool operator () ( common::communication::select::tag::consume)
               {
                  // we consume the cache.
                  return common::predicate::boolean( m_handler( ipc::inbound().cached()));
               }

               bool operator() ( common::strong::file::descriptor::id descriptor, common::communication::select::tag::read)
               {
                  if( ipc::inbound().connector().descriptor() != descriptor)
                     return false;

                  m_handler( common::communication::device::non::blocking::next( ipc::inbound()));
                  return true;
               }

            private:
               H m_handler;
            };
         } // detail

         template< typename State, typename HC>
         auto create( State& state, HC&& handler_creator)
         {
            using handler_type = decltype( handler_creator( state));

            // make sure we set the select fd-set
            state.directive.read.add( ipc::inbound().connector().descriptor());

            return detail::select_handler< handler_type>( handler_creator( state));
         }
         
      } // dispatch

   } // gateway::group::ipc
} // casual