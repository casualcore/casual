//!
//! timeout.h
//!
//! Created on: Oct 19, 2014
//!     Author: Lazan
//!

#ifndef CASUAL_COMMON_CALL_TIMEOUT_H_
#define CASUAL_COMMON_CALL_TIMEOUT_H_

#include "common/platform.h"
#include "common/algorithm.h"

#include <chrono>

namespace casual
{
   namespace common
   {
      namespace call
      {

         class Timeout
         {
         public:

            using descriptor_type = platform::descriptor_type;
            using time_point = platform::time_point;

            enum Type
            {
               cUnset = -1, // Invalid call descriptor
               cTransaction = 0, // to indicate that the timeout is 'global' to current transaction
            };

            struct Limit
            {
               time_point timeout;
               descriptor_type descriptor;

               friend bool operator < ( const Limit& lhs, const Limit& rhs) { return lhs.timeout < rhs.timeout;}
            };


            static Timeout& instance();

            void add( descriptor_type descriptor, std::chrono::microseconds timeout);
            void add( descriptor_type descriptor, time_point limit);

            //!
            //! @return the earliest deadline
            //!
            Limit get() const;


            //!
            //! @return true if timeout has passed for descriptor
            //!
            bool passed( descriptor_type descriptor) const;

            //!
            //! Removes the timeout.
            //!
            //! @return true if the timeout has passed
            //!
            bool remove( descriptor_type descriptor);


            void clear();




         private:

            Timeout() = default;

            using timeouts_type = std::vector< Limit>;

            using range_type = decltype( range::make( timeouts_type().begin(), timeouts_type().end()));


            void add( Limit limit, time_point now);

            void set( time_point now);


            timeouts_type m_limits;
            descriptor_type m_current = Type::cUnset;
         };


      } // call
   } // common


} // casual

#endif // TIMEOUT_H_
