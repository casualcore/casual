//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "gateway/group/outbound/state.h"

#include "common/predicate.h"
#include "common/algorithm/sorted.h"
#include "common/algorithm/random.h"

namespace casual
{
   using namespace common;

   namespace gateway::group::outbound
   {

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
            
            bool Transactions::associate( const common::transaction::ID& trid, common::strong::socket::id descriptor)
            {
               auto gtrid = trid.global();

               if( auto found = algorithm::find( m_transactions, gtrid))
               {
                  if( algorithm::find( found->second, descriptor))
                     return false;
                  
                  found->second.push_back( descriptor);
                  return true;
               }

               m_transactions.emplace( gtrid, std::vector< common::strong::socket::id>{ descriptor});
               return true;
            }

            void Transactions::remove( const common::transaction::ID& trid, common::strong::socket::id descriptor)
            {
               auto gtrid = trid.global();

               if( auto found = algorithm::find( m_transactions, gtrid))
                  if( algorithm::container::erase( found->second, descriptor).empty())
                     algorithm::container::erase( m_transactions, std::begin( found));
            }

            void Transactions::remove( common::transaction::global::id::range gtrid)
            {
               // should compile since c++23.
               // m_transactions.erase( gtrid);

               if( auto found = algorithm::find( m_transactions, gtrid))
                  m_transactions.erase( std::begin( found));
            }

            void Transactions::remove( common::strong::socket::id descriptor)
            {
               for( auto& pair : m_transactions)
                  algorithm::container::erase( pair.second, descriptor);
            }

            bool Transactions::is_associated( const common::transaction::ID& trid, common::strong::socket::id descriptor)
            {
               auto gtrid = trid.global();

               if( auto found = algorithm::find( m_transactions, gtrid))
                  if( algorithm::find( found->second, descriptor))
                     return true;

               return false;
            }


            bool Transactions::contains( common::strong::socket::id descriptor) const noexcept
            {
               for( auto& pair : m_transactions)
                  if( algorithm::contains( pair.second, descriptor))
                     return true;

               return false;
            }
            
         } // pending

         namespace disconnect
         {
            std::string_view description( Directive value)
            {
               switch( value)
               {
                  case Directive::reconnect: return "reconnect";
                  case Directive::remove: return "remove";
               }
               return "<unknown>";
            }
            
         } // disconnect


      } // state

 
      message::outbound::connection::Reconnect State::extract( common::strong::socket::id descriptor)
      {
         Trace trace{ "gateway::group::outbound::State::extract"};

         // remove from transaction cache, if any
         pending.transactions.remove( descriptor);

         auto connection = connections.extract( directive, descriptor);

         return { std::move( connection.configuration), std::move( connection.domain)};
      }

      bool State::done() const
      {
         Trace trace{ "gateway::group::outbound::State::done"};

         if( runlevel <= state::Runlevel::running)
            return false;

         // NOTE: We should only need to check pending.dissociating.empty().
         return pending.dissociating.empty() && tasks.empty() && pending.transactions.empty();
      }

   } // gateway::group::outbound

} // casual
