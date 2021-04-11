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

      struct State
      {

         struct Pending
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

               friend bool operator == ( descriptor_type cd, const Descriptor& d) { return cd == d.descriptor;}
               friend bool operator == ( const Descriptor& d, descriptor_type cd) { return cd == d.descriptor;}
            };


            Pending();

            //! Reserves a descriptor and associates it to message-correlation
            Descriptor& reserve( const correlation_type& correlation);

            void unreserve( descriptor_type descriptor);

            bool active( descriptor_type descriptor) const;

            const Descriptor& get( descriptor_type descriptor) const;
            const Descriptor& get( const correlation_type& correlation) const;
            Descriptor& get( descriptor_type descriptor);

            //! Tries to discard descriptor, throws if fail.
            void discard( descriptor_type descriptor);

            //! @returns true if there are no pending replies or associated transactions.
            //!  Thus, it's ok to do a service-forward
            bool empty() const;

         private:

            Descriptor& reserve();

            std::vector< Descriptor> m_descriptors;

         } pending;
      };

   } // common::service::call
} // casual


