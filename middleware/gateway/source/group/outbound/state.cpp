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
      namespace local
      {
         namespace
         {
            namespace global
            {
               const transaction::ID trid;
            } // global
            
         } // <unnamed>
      } // local

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
               auto gtrid = transaction::id::range::global( trid);

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
               auto gtrid = transaction::id::range::global( trid);

               if( auto found = algorithm::find( m_transactions, gtrid))
                  if( algorithm::container::erase( found->second, descriptor).empty())
                     algorithm::container::erase( m_transactions, std::begin( found));
            }

            bool Transactions::is_associated( const common::transaction::ID& trid, common::strong::socket::id descriptor)
            {
               auto gtrid = transaction::id::range::global( trid);

               if( auto found = algorithm::find( m_transactions, gtrid))
                  if( algorithm::find( found->second, descriptor))
                     return true;

               return false;
            }

            std::vector< common::transaction::global::ID> Transactions::extract( common::strong::socket::id descriptor)
            {
               std::vector< common::transaction::global::ID> result;

               algorithm::container::erase_if( m_transactions, [ &result, descriptor]( auto& pair)
               {
                  if( auto found = algorithm::find( pair.second, descriptor))
                  {
                     result.push_back( pair.first);

                     if( algorithm::container::erase( pair.second, std::begin( found)).empty())
                        return true;
                  }
                  return false;
               });

               return result;
            }

            bool Transactions::contains( common::strong::socket::id descriptor) const noexcept
            {
               for( auto& pair : m_transactions)
                  if( algorithm::contains( pair.second, descriptor))
                     return true;

               return false;
            }
            
         } // pending

      } // state

      state::extract::Result State::failed( common::strong::socket::id descriptor)
      {
         Trace trace{ "gateway::group::outbound::State::failed"};
         log::line( verbose::log, "descriptor: ", descriptor);

         // clean the disconnecting state.
         algorithm::container::erase( disconnecting, descriptor);

         return {
            connections.extract( directive, descriptor),
            reply_destination.extract( descriptor),
            pending.transactions.extract( descriptor)
         };
      }

      bool State::idle( common::strong::socket::id descriptor) const noexcept
      {
         Trace trace{ "gateway::group::outbound::State::idle"};

         return ! pending.transactions.contains( descriptor) && ! reply_destination.contains( descriptor) && ! tasks.contains( descriptor);
      }

      bool State::done() const
      {
         if( runlevel <= state::Runlevel::running)
            return false;

         return tasks.empty() && pending.transactions.empty() && reply_destination.empty();
      }

   } // gateway::group::outbound

} // casual