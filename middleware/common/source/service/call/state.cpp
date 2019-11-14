//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/service/call/state.h"

#include "common/transaction/context.h"

#include "common/communication/ipc.h"

#include "common/exception/xatmi.h"

namespace casual
{
   namespace common
   {
      namespace service
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
               auto found = algorithm::find_if( m_descriptors, predicate::negate( std::mem_fn( &Descriptor::active)));

               if( found)
               {
                  found->active = true;
                  found->timeout.timeout = platform::time::unit{ 0};
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
               auto found = algorithm::find( m_descriptors, descriptor);

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
               auto found = algorithm::find( m_descriptors, descriptor);

               if( found)
               {
                  return found->active;
               }
               return false;
            }

            const State::Pending::Descriptor& State::Pending::get( descriptor_type descriptor) const
            {
               auto found = algorithm::find( m_descriptors, descriptor);
               if( found && found->active)
               {
                  return *found;
               }
               throw exception::xatmi::invalid::Descriptor{ "invalid call descriptor: " + std::to_string( descriptor)};
            }

            const State::Pending::Descriptor& State::Pending::get( const Uuid& correlation) const
            {
               auto found = algorithm::find_if( m_descriptors, [&]( const auto& d){ return d.correlation == correlation;});
               if( found && found->active)
               {
                  return *found;
               }
               throw exception::xatmi::invalid::Descriptor{ string::compose( "failed to locate pending from correlation: ", correlation)};
            }

            signal::timer::Deadline State::Pending::deadline( descriptor_type descriptor, const platform::time::point::type& now) const
            {
               if( descriptor == 0)
               {

               }
               else
               {
                  auto& desc = get( descriptor);
                  return { desc.timeout.deadline(), now};
               }

               return { platform::time::point::type::max(), now};
            }

            void State::Pending::discard( descriptor_type descriptor)
            {
               const auto& holder = get( descriptor);

               //
               // Can't be associated with a transaction
               //
               if( common::transaction::Context::instance().associated( holder.correlation))
               {
                  throw exception::xatmi::transaction::Support{ "descriptor " + std::to_string( holder.descriptor) + " is associated with a transaction"};
               }

               //
               // Discards the correlation (directly if in cache, or later if not)
               //
               communication::ipc::inbound::device().discard( holder.correlation);

               unreserve( descriptor);
            }

            bool State::Pending::empty() const
            {
               return algorithm::all_of( m_descriptors, predicate::negate( std::mem_fn( &Descriptor::active)));
            }

         } // call
      } // service
   } // common

} // casual
