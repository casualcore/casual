//!
//! state.cpp
//!
//! Created on: Jul 12, 2015
//!     Author: Lazan
//!

#include "common/call/state.h"

#include "common/transaction/context.h"

#include "common/communication/ipc.h"


namespace casual
{
   namespace common
   {
      namespace call
      {

         State::Pending::Pending()
          : m_descriptors{
            { 1, false },
            { 2, false },
            { 3, false },
            { 4, false },
            { 5, false },
            { 6, false },
            { 7, false },
            { 8, false }}
         {

         }


         State::Pending::Descriptor& State::Pending::reserve( const Uuid& correlation)
         {
            auto& descriptor = reserve();

            descriptor.correlation = correlation;

            return descriptor;
         }

         State::Pending::Descriptor& State::Pending::reserve()
         {
            auto found = range::find_if( m_descriptors, negate( std::mem_fn( &Descriptor::active)));

            if( found)
            {
               found->active = true;
               found->timeout.timeout = std::chrono::microseconds{ 0};
               return *found;
            }
            else
            {
               m_descriptors.emplace_back( m_descriptors.back().descriptor + 1, true);
               return m_descriptors.back();
            }
         }

         void State::Pending::unreserve( descriptor_type descriptor)
         {
            auto found = range::find( m_descriptors, descriptor);

            if( found)
            {
               found->active = false;
            }
            else
            {
               throw exception::xatmi::invalid::Descriptor{ "invalid call descriptor: " + std::to_string( descriptor)};
            }
         }

         bool State::Pending::active( descriptor_type descriptor) const
         {
            auto found = range::find( m_descriptors, descriptor);

            if( found)
            {
               return found->active;
            }
            return false;
         }

         const State::Pending::Descriptor& State::Pending::get( descriptor_type descriptor) const
         {
            auto found = range::find( m_descriptors, descriptor);
            if( found && found->active)
            {
               return *found;
            }
            throw exception::xatmi::invalid::Descriptor{ "invalid call descriptor: " + std::to_string( descriptor)};
         }

         signal::timer::Deadline State::Pending::deadline( descriptor_type descriptor, const platform::time_point& now) const
         {
            if( descriptor == 0)
            {

            }
            else
            {
               auto& desc = get( descriptor);
               return { desc.timeout.deadline(), now};
            }

            return { platform::time_point::max(), now};
         }

         void State::Pending::discard( descriptor_type descriptor)
         {
            //
            // Can't be associated with a transaction
            //
            if( transaction::Context::instance().associated( descriptor))
            {
               throw exception::xatmi::transaction::Support{ "descriptor " + std::to_string( descriptor) + " is associated with a transaction"};
            }

            //
            // Discards the correlation (directly if in cache, or later if not)
            //
            communication::ipc::inbound::device().discard( get( descriptor).correlation);

            unreserve( descriptor);
         }

         bool State::Pending::empty() const
         {
            return range::all_of( m_descriptors, negate( std::mem_fn( &Descriptor::active)));
         }

      } // call
   } // common

} // casual
