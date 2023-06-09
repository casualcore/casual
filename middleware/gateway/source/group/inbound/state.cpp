//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "gateway/group/inbound/state.h"
#include "gateway/common.h"


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

         namespace in::flight
         {
            void Cache::add( common::strong::file::descriptor::id descriptor, const common::transaction::ID& trid)
            {
               if( ! trid)
                  return;

               if( auto connection = algorithm::find( m_transactions, descriptor))
               {
                  if( ! algorithm::find( connection->second, trid))
                     connection->second.push_back( trid);
               }
               else
               {
                  m_transactions[ descriptor].push_back( trid);
               }
            }

            void Cache::remove( common::strong::file::descriptor::id descriptor, const common::transaction::ID& trid)
            {
               if( auto connection = algorithm::find( m_transactions, descriptor))
               {
                  if( auto found = algorithm::find( connection->second, trid))
                  {
                     connection->second.erase( std::begin( found));

                     if( connection->second.empty())
                        m_transactions.erase( std::begin( connection));
                  }
               }
            }

            void Cache::remove( common::strong::file::descriptor::id descriptor)
            {
               if( auto found = algorithm::find( m_transactions, descriptor))
                  m_transactions.erase( std::begin( found));
            }


            bool Cache::empty( common::strong::file::descriptor::id descriptor) const noexcept
            {
                if( auto found = algorithm::find( m_transactions, descriptor))
                  return found->second.empty();

               return true;
            }
            
            std::vector< common::transaction::ID> Cache::extract( common::strong::file::descriptor::id descriptor)
            {
               Trace trace{ "gateway::group::inbound::state::in::flight::Cache::extract"};

               if( auto found = algorithm::find( m_transactions, descriptor))
                  return algorithm::container::extract( m_transactions, std::begin( found)).second;

               return {};
            }

         } // in::flight


      } // state

      common::strong::file::descriptor::id State::consume( const strong::correlation::id& correlation)
      {
         if( auto found = algorithm::find( correlations, correlation))
            return algorithm::container::extract( correlations, std::begin( found)).descriptor;

         return {};
      }

      tcp::Connection* State::connection( const common::strong::correlation::id& correlation)
      {
         if( auto found = algorithm::find( correlations, correlation))
            return external.connection( found->descriptor);

         return nullptr;
      }

      bool State::disconnectable( common::strong::file::descriptor::id descriptor) const noexcept
      {
         return in_flight_cache.empty( descriptor) && algorithm::find( correlations, descriptor).empty();         
      }

      bool State::done() const noexcept
      {
         return runlevel > state::Runlevel::running && external.empty();
      }

      state::extract::Result State::extract( common::strong::file::descriptor::id descriptor)
      {
         // find possible pending 'lookup' requests
         auto lost = algorithm::container::extract( correlations, algorithm::filter( correlations, predicate::value::equal( descriptor)));
         log::line( verbose::log, "lost: ", lost);

         // remove disconnects, if any
         if( auto found = algorithm::find( pending.disconnects, descriptor))
            pending.disconnects.erase( std::begin( found));

         return state::extract::Result{ 
            external.remove( directive, descriptor),
            pending.requests.consume( algorithm::transform( lost, []( auto& lost){ return lost.correlation;})),
            in_flight_cache.extract( descriptor)
         };
      }

   } // gateway::group::inbound

} // casual