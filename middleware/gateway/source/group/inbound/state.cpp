//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "gateway/group/inbound/state.h"

#include <utility>

namespace casual
{
   namespace gateway::group::inbound
   {
      using namespace common;

      namespace state
      {
         std::string_view description( Runlevel value)
         {
            switch( value)
            {
               case Runlevel::running: return "running";
               case Runlevel::shutdown: return "shutdown";
               case Runlevel::error: return "error";
            }
            return "<unknown>";
         }

         namespace pending
         {
            Requests::complete_type Requests::consume( const strong::correlation::id& correlation, const common::message::service::lookup::Reply& lookup)
            {
               if( auto found = common::algorithm::find( m_services, correlation))
               {
                  auto message = algorithm::container::extract( m_services, std::begin( found));
                  m_size -= Requests::size( message);
                  message.pending = lookup.pending;

                  if( message.service.name != lookup.service.name)
                     message.service.requested = std::exchange( message.service.name, lookup.service.name);

                  return serialize::native::complete< complete_type>( std::move( message));
               }
               
               return Requests::consume( correlation);
            }

            Requests::complete_type Requests::consume( const strong::correlation::id& correlation)
            {
               if( auto found = algorithm::find( m_complete, correlation))
               {
                  auto result = algorithm::container::extract( m_complete, std::begin( found));
                  m_size -= result.payload.size();

                  return result;
               }

               code::raise::error( code::casual::invalid_argument, "failed to find correlation: ", correlation);
            } 

            Requests::Result Requests::consume( const std::vector< strong::correlation::id>& correlations)
            {
               auto has_correlation = [&correlations]( auto& value)
               {
                  return ! algorithm::find( correlations, value).empty();
               };

               return {
                  algorithm::container::extract( m_services, algorithm::filter( m_services, has_correlation)),
                  algorithm::container::extract( m_complete, algorithm::filter( m_complete, has_correlation)),
               };
            }     
         } // pending



      } // state

      tcp::Connection* State::consume( const strong::correlation::id& correlation)
      {
         if( auto found = algorithm::find( correlations, correlation))
         {
            auto descriptor = algorithm::container::extract( correlations, std::begin( found)).descriptor;
            
            if( auto connector = algorithm::find( external.connections(), descriptor))
               return connector.data();
         }

         return nullptr;
      }

      tcp::Connection* State::connection( const common::strong::correlation::id& correlation)
      {
         if( auto found = algorithm::find( correlations, correlation))
            if( auto connector = algorithm::find( external.connections(), found->descriptor))
               return connector.data();

         return nullptr;
      }

      bool State::done() const noexcept
      {
         return runlevel > state::Runlevel::running && external.empty();
      }

   } // gateway::group::inbound

} // casual