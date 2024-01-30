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
           
         namespace transaction
         {

            const common::transaction::ID* Cache::associate( common::strong::socket::id descriptor, const common::transaction::ID& external)
            {
               if( auto found = algorithm::find( m_associations, descriptor))
               {
                  if( ! found->trids.contains( external))
                     found->trids.insert( external);
               }
               else
               {
                  m_associations.emplace_back( descriptor, external);
               }

               if( auto found = algorithm::find( m_transactions, common::transaction::id::range::global( external)))
               {
                  if( ! algorithm::find( found->second.descriptors, descriptor))
                     found->second.descriptors.push_back( descriptor);
                  
                  return &found->second.trid;
               }

               return nullptr;
            }

            void Cache::dissociate( common::strong::socket::id descriptor, const common::transaction::ID& external)
            {
               if( auto found = algorithm::find( m_associations, descriptor))
               {
                  found->trids.erase( external);
                  if( found->trids.empty())
                     algorithm::container::erase( m_associations, std::begin( found));
               }

               // remove the descriptor, and the mapping for the gtrid if there are no associated descriptors.
               if( auto found = algorithm::find( m_transactions, common::transaction::id::range::global( external)))
                  if( auto found_descriptor = algorithm::find( found->second.descriptors, descriptor))
                     if( algorithm::container::erase( found->second.descriptors, std::begin( found_descriptor)).empty())
                        algorithm::container::erase( m_transactions, std::begin( found));
            }

            void Cache::add_branch( common::strong::socket::id descriptor, const common::transaction::ID& branched_trid)
            {
               auto gtrid = common::transaction::id::range::global( branched_trid);

               if( auto found = algorithm::find( m_transactions, gtrid))
               {
                  if( ! algorithm::find( found->second.descriptors, descriptor))
                     found->second.descriptors.push_back( descriptor);
               }
               else
               {
                  m_transactions.emplace( gtrid, Map{ descriptor, branched_trid});
               }
            }


            bool Cache::associated( common::strong::socket::id descriptor) const noexcept
            {
               if( auto found = algorithm::find( m_associations, descriptor))
                  return true;

               return false;
            }

            const common::transaction::ID* Cache::find( common::transaction::global::id::range gtrid) const noexcept
            {
               if( auto found = algorithm::find( m_transactions, gtrid))
                  return &found->second.trid;

               return nullptr;
            }

            std::vector< common::transaction::ID> Cache::extract( common::strong::socket::id descriptor) noexcept
            {
               // remove the descriptor in all transaction mappings. We iterate over the whole set.
               algorithm::container::erase_if( m_transactions, [ descriptor]( auto& pair)
               {
                  return algorithm::container::erase( pair.second.descriptors, descriptor).empty();
               });

               // extract all associated external trids with the descriptor.
               if( auto found = algorithm::find( m_associations, descriptor))
               {
                  auto association = algorithm::container::extract( m_associations, std::begin( found));
                  return algorithm::transform( association.trids, std::identity{});
               }
                  
               return {};
            }
            
         } // transaction


      } // state

      common::strong::socket::id State::consume( const strong::correlation::id& correlation)
      {
         if( auto found = algorithm::find( correlations, correlation))
            return algorithm::container::extract( correlations, std::begin( found)).descriptor;

         return {};
      }

      tcp::Connection* State::connection( const common::strong::correlation::id& correlation)
      {
         if( auto found = algorithm::find( correlations, correlation))
            return external.find_external( found->descriptor);

         return nullptr;
      }

      bool State::disconnectable( common::strong::socket::id descriptor) const noexcept
      {
         return ! algorithm::find( correlations, descriptor) && ! transaction_cache.associated( descriptor);
      }

      bool State::done() const noexcept
      {
         return runlevel > state::Runlevel::running && external.empty();
      }

      state::extract::Result State::extract( common::strong::socket::id descriptor)
      {

         // find possible in-flight request. We deduce this by extracting the correlations that map to the `descriptor`
         auto lost = algorithm::container::extract( correlations, algorithm::filter( correlations, predicate::value::equal( descriptor)));
         log::line( verbose::log, "lost: ", lost);

         // remove disconnects, if any
         algorithm::container::erase( pending.disconnects, descriptor);

          auto transform_correlation = []( auto& value){ return value.correlation;};

         return state::extract::Result{ 
            external.remove( directive, descriptor),
            algorithm::transform( lost, transform_correlation),
            transaction_cache.extract( descriptor)
         };
      }

   } // gateway::group::inbound

} // casual