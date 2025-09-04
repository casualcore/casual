//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "service/call/state.h"

#include "transaction/context.h"

#include "common/communication/ipc.h"

#include "common/code/raise.h"
#include "common/code/xatmi.h"

namespace casual
{
   namespace service::call
   {
      namespace state
      {
         namespace local
         {
            namespace
            {
               auto is_inactive = []( auto& correlation){ return ! correlation.valid();};
               auto is_active = []( auto& correlation){ return correlation.valid();};
            } // <unnamed>
         } // local

         Pending::Pending()
         {
            m_correlations.reserve( 4);
         }

         const common::strong::correlation::id& Pending::reserve( const common::strong::correlation::id& correlation)
         {
            if( auto found = common::algorithm::find_if( m_correlations, local::is_inactive))
            {
               *found = correlation;
               return *found;
            }

            return m_correlations.emplace_back( correlation);
         }


         void Pending::unreserve( const common::strong::correlation::id& correlation)
         {
            if( auto found = common::algorithm::find( m_correlations, correlation))
               *found = {};
            else
               common::code::raise::error( common::code::xatmi::descriptor, "invalid call correlation: ", correlation);
         } 

         const common::strong::correlation::id& Pending::validate( const common::strong::correlation::id& correlation) const
         {
            if( common::algorithm::contains( m_correlations, correlation))
               return correlation;

            common::code::raise::error( common::code::xatmi::descriptor, "failed to locate pending from correlation: ", correlation);
         }


         void Pending::discard( const common::strong::correlation::id& correlation)
         {
            if( auto found = common::algorithm::find( m_correlations, correlation))
            {
               // Can't be associated with a transaction
               if( casual::transaction::context().associated( *found))
                  common::code::raise::error( common::code::xatmi::transaction, "correlation is associated with a transaction - ", *found);

               // Discards the correlation (directly if in cache, or later if not)
               common::communication::ipc::inbound::device().discard( *found);

               unreserve( *found);
            }
         }

         bool Pending::empty() const
         {
            return common::algorithm::all_of( m_correlations, local::is_inactive);
         }

         std::vector< common::strong::correlation::id> Pending::finalize()
         {
            common::Trace trace{ "common::service::call::state::Pending::finalize"};

            std::vector< common::strong::correlation::id> result;

            for( auto& correlation : m_correlations | std::views::filter( local::is_active))
               result.push_back( std::exchange( correlation, {}));

            return result;
         }

      } // state


   } // service::call

} // casual
