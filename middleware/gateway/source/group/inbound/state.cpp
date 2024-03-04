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

            const common::transaction::ID* Cache::associate( const common::transaction::ID& external, common::strong::socket::id descriptor)
            {
               if( auto found = algorithm::find( m_transactions, common::transaction::id::range::global( external)))
               {
                  if( ! algorithm::contains( found->second.descriptors, descriptor))
                     found->second.descriptors.push_back( descriptor);
                  
                  return &found->second.trid;
               }

               return nullptr;
            }

            void Cache::add( const common::transaction::ID& branched_trid, common::strong::socket::id descriptor)
            {
               auto gtrid = common::transaction::id::range::global( branched_trid);

               if( auto found = algorithm::find( m_transactions, gtrid))
               {
                  if( ! algorithm::contains( found->second.descriptors, descriptor))
                     found->second.descriptors.push_back( descriptor);
               }
               else
               {
                  m_transactions.emplace( gtrid, Association{ descriptor, branched_trid});
               }
            }


            bool Cache::associated( common::strong::socket::id descriptor) const noexcept
            {
               // check all transaction associations, and if we find one the `descriptor` is still associated
               if( algorithm::find_if( m_transactions, [ descriptor]( auto& pair){ return pair.second == descriptor;}))
                  return true;

               return false;
            }

            const common::transaction::ID* Cache::find( common::transaction::global::id::range gtrid) const noexcept
            {
               if( auto found = algorithm::find( m_transactions, gtrid))
                  return &found->second.trid;

               return nullptr;
            }

            std::vector< common::transaction::ID> Cache::failed( common::strong::socket::id descriptor) noexcept
            {
               std::vector< common::transaction::ID> result;

               // remove the descriptor in all transaction mappings. We iterate over the whole set.
               algorithm::container::erase_if( m_transactions, [ &result, descriptor]( auto& pair)
               {
                  if( ! algorithm::container::erase( pair.second.descriptors, descriptor).empty())
                     return false;

                  // this was the last descriptor for the trid. It will be "indoubt".
                  result.push_back( pair.second.trid);
                  return true;
               });
                  
               return result;
            }

            void Cache::remove( common::transaction::global::id::range gtrid)
            {
               algorithm::container::erase( m_transactions, gtrid);
            }
        
         } // transaction


      } // state



      bool State::disconnectable( common::strong::socket::id descriptor) const noexcept
      {
         return ! transaction_cache.associated( descriptor) && ! tasks.contains( descriptor);
      }

      bool State::done() const noexcept
      {
         return runlevel > state::Runlevel::running && external.empty();
      }

      state::extract::Result State::extract( common::strong::socket::id descriptor)
      {
         auto transform_correlation = []( auto& value){ return value.correlation();};

         // find possible in-flight request. We deduce this by extracting the correlations that map to the `descriptor`
         auto lost = algorithm::transform_if( tasks.tasks(), transform_correlation, predicate::value::equal( descriptor));
         log::line( verbose::log, "lost: ", lost);

         // remove disconnects, if any
         algorithm::container::erase( pending.disconnects, descriptor);

         return state::extract::Result{ 
            external.remove( directive, descriptor),
            std::move( lost),
            transaction_cache.failed( descriptor)
         };
      }

   } // gateway::group::inbound

} // casual