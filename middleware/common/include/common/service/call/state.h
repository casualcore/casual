//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "casual/platform.h"
#include "common/strong/id.h"
#include "common/strong/type.h"
#include "common/service/type.h"
#include "common/message/type.h"

namespace casual
{
   namespace common::service::call
   {
      using descriptor_type = platform::descriptor::type;
      using correlation_type = strong::correlation::id;

      namespace state
      {
         namespace pending
         {
            struct Descriptor
            {
               using Contract = common::service::execution::timeout::contract::Type;

               Descriptor( descriptor_type descriptor, bool active = true)
                  : descriptor( descriptor), active( active) {}

               descriptor_type descriptor;
               bool active;
               correlation_type correlation;
               common::strong::process::id target{};
               Contract contract{ Contract::linger};

               inline friend bool operator == ( const Descriptor& lhs, descriptor_type rhs) { return lhs.descriptor == rhs;}
            };
            
         } // pending

         struct Pending
         {
            Pending();

            //! Reserves a descriptor and associates it to message-correlation
            pending::Descriptor& reserve( const correlation_type& correlation);

            void unreserve( descriptor_type descriptor);

            bool active( descriptor_type descriptor) const;

            const pending::Descriptor& get( descriptor_type descriptor) const;
            const pending::Descriptor& get( const correlation_type& correlation) const;
            pending::Descriptor& get( descriptor_type descriptor);

            //! Tries to discard descriptor, throws if fail.
            void discard( descriptor_type descriptor);

            //! @returns true if there are no pending replies or associated transactions.
            //!  Thus, it's ok to do a service-forward
            bool empty() const;

         private:

            pending::Descriptor& reserve();
            std::vector< pending::Descriptor> m_descriptors;
         };
         
      } // state

      struct State
      {
         state::Pending pending;
         std::optional< platform::time::point::type> deadline;
      };

   } // common::service::call
} // casual


