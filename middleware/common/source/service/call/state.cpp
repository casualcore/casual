//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/service/call/state.h"

#include "common/transaction/context.h"

#include "common/communication/ipc.h"

#include "common/code/raise.h"
#include "common/code/xatmi.h"

namespace casual
{
   namespace common::service::call
   {
      namespace state
      {
         Pending::Pending()
            : m_descriptors{
            { 1, false },
            { 2, false },
            { 3, false },
            { 4, false },
            { 5, false },
            { 6, false },
            { 7, false },
            { 8, false }}
         {}

         pending::Descriptor& Pending::reserve( const correlation_type& correlation)
         {
            auto& descriptor = reserve();

            descriptor.correlation = correlation;

            return descriptor;
         }

         pending::Descriptor& Pending::reserve()
         {
            if( auto found = algorithm::find_if( m_descriptors, predicate::negate( std::mem_fn( &pending::Descriptor::active))))
            {
               found->active = true;
               found->target = {};
               return *found;
            }
            else
            {
               m_descriptors.emplace_back( m_descriptors.back().descriptor + 1, true);
               return m_descriptors.back();
            }
         }

         void Pending::unreserve( descriptor_type descriptor)
         {
            if( auto found = algorithm::find( m_descriptors, descriptor))
               found->active = false;
            else
               code::raise::error( code::xatmi::descriptor, "invalid call descriptor: ", descriptor);
         }

         bool Pending::active( descriptor_type descriptor) const
         {
            if( auto found = algorithm::find( m_descriptors, descriptor))
               return found->active;
            
            return false;
         }

         const pending::Descriptor& Pending::get( descriptor_type descriptor) const
         {
            auto found = algorithm::find( m_descriptors, descriptor);
            if( ! found || ! found->active)
               code::raise::error( code::xatmi::descriptor, "invalid call descriptor: ", descriptor);

            return *found;
         }

         pending::Descriptor& Pending::get( descriptor_type descriptor)
         {
            auto found = algorithm::find( m_descriptors, descriptor);
            if( ! found || ! found->active)
               code::raise::error( code::xatmi::descriptor, "invalid call descriptor: ", descriptor);

            return *found;
         }

         const pending::Descriptor& Pending::get( const correlation_type& correlation) const
         {
            auto found = algorithm::find_if( m_descriptors, [&]( const auto& d){ return d.correlation == correlation;});
            if( ! ( found && found->active))
               code::raise::error( code::xatmi::descriptor, "failed to locate pending from correlation: ", correlation);
               
            return *found;
         }

         void Pending::discard( descriptor_type descriptor)
         {
            const auto& holder = get( descriptor);

            // Can't be associated with a transaction
            if( common::transaction::Context::instance().associated( holder.correlation))
               code::raise::error( code::xatmi::transaction, "descriptor is associated with a transaction - ", holder.descriptor);

            // Discards the correlation (directly if in cache, or later if not)
            communication::ipc::inbound::device().discard( holder.correlation);

            unreserve( descriptor);
         }

         bool Pending::empty() const
         {
            return algorithm::all_of( m_descriptors, predicate::negate( std::mem_fn( &pending::Descriptor::active)));
         }

      } // state


   } // common::service::call

} // casual
