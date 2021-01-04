//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "gateway/inbound/state.h"


namespace casual
{
   namespace gateway::inbound
   {
      using namespace common;

      namespace state
      {
         std::ostream& operator << ( std::ostream& out, Runlevel value)
         {
            switch( value)
            {
               case Runlevel::running: return out << "running";
               case Runlevel::shutdown: return out << "shutdown";
               case Runlevel::error: return out << "error";
            }
            return out << "<unknown>";
         }

         namespace pending
         {
            Requests::complete_type Requests::consume( const common::Uuid& correlation, platform::time::unit pending)
            {
               if( auto found = common::algorithm::find( m_services, correlation))
               {
                  auto message = algorithm::extract( m_services, std::begin( found));
                  m_size -= Requests::size( message);
                  message.pending = pending;

                  return serialize::native::complete( std::move( message));
               }

               common::code::raise::log( common::code::casual::invalid_argument, "failed to find correlation: ", correlation);
            }

            Requests::complete_type Requests::consume( const common::Uuid& correlation)
            {
               if( auto found = algorithm::find( m_complete, correlation))
               {
                  auto result = algorithm::extract( m_complete, std::begin( found));
                  m_size -= result.payload.size();

                  return result;
               }

               code::raise::log( code::casual::invalid_argument, "failed to find correlation: ", correlation);
            } 

            Requests::Result Requests::consume( const std::vector< common::Uuid>& correlations)
            {
               auto has_correlation = [&correlations]( auto& value)
               {
                  return ! algorithm::find( correlations, value.correlation).empty();
               };

               return {
                  algorithm::extract( m_services, algorithm::filter( m_services, has_correlation)),
                  algorithm::extract( m_complete, algorithm::filter( m_complete, has_correlation)),
               };
            }     
         } // pending



      } // state

      state::external::Connection* State::consume( const common::Uuid& correlation)
      {
         if( auto found = algorithm::find( correlations, correlation))
         {
            auto descriptor = algorithm::extract( correlations, std::begin( found)).descriptor;
            
            if( auto connector = algorithm::find( external.connections, descriptor))
               return connector.data();
         }

         return nullptr;
      }

      bool State::done() const noexcept
      {
         return runlevel > state::Runlevel::running && external.empty();
      }

   } // gateway::inbound

} // casual