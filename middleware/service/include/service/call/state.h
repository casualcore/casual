//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "casual/platform.h"
#include "common/strong/id.h"
#include "common/strong/type.h"
#include "common/chronology.h"

#include "common/service/type.h"


namespace casual
{
   namespace service::call
   {
      namespace state
      {
         struct Pending
         {
            Pending();

            //! Reserves a descriptor and associates it to message-correlation
            const common::strong::correlation::id& reserve( const common::strong::correlation::id& correlation);

            void unreserve( const common::strong::correlation::id& correlation);

            //! @throws if `correlation` is not found in pending correlations
            const common::strong::correlation::id& validate( const common::strong::correlation::id& correlation) const;

            //! Tries to discard descriptor
            void discard( const common::strong::correlation::id& correlation);

            //! @returns true if there are no pending replies or associated transactions.
            //!  Thus, it's ok to do a service-forward
            bool empty() const;

            //! @returns all in-flight correlations, and clear state.
            std::vector< common::strong::correlation::id> finalize();

         private:
            std::vector< common::strong::correlation::id> m_correlations;
         };
         
      } // state

      struct State
      {
         state::Pending pending;
         std::optional< common::chronology::time_point> deadline;
      };

   } // service::call
} // casual


