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
#include "common/transaction/id.h"

#include <chrono>
#include <functional>

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


            struct base_t
            {
               base_t() = default;
               base_t( time_point timeout) : timeout( timeout) {}

               time_point timeout;

               friend bool operator < ( const base_t& lhs, const base_t& rhs) { return lhs.timeout < rhs.timeout;}
               friend bool operator < ( const base_t& lhs, const time_point& rhs) { return lhs.timeout < rhs;}
               friend bool operator < ( const time_point& lhs, const base_t& rhs) { return lhs < rhs.timeout;}
            };

            struct Limit : base_t
            {
               Limit() = default;
               Limit( time_point timeout, descriptor_type descriptor) : base_t( timeout), descriptor( descriptor) {}

               descriptor_type descriptor;
            };

            struct Transaction : base_t
            {
               Transaction() = default;
               Transaction( time_point timeout, transaction::ID trid) : base_t( timeout), trid( trid) {}

               transaction::ID trid;

               friend bool operator == ( const Transaction& lhs, const transaction::ID& rhs) { return lhs.trid < rhs;}
            };


            static Timeout& instance();

            void add( descriptor_type descriptor, std::chrono::microseconds timeout, const time_point& now = platform::clock_type::now());
            void add( const transaction::ID& trid, const std::chrono::microseconds& timeout, const time_point& now = platform::clock_type::now());

            void current( descriptor_type descriptor, const time_point& now = platform::clock_type::now());

            void unset() noexcept;


            //!
            //! Removes a timeout.
            //!
            void remove( descriptor_type descriptor) noexcept;


            void clear();



            struct Unset
            {
               using descriptor_type = platform::descriptor_type;

               Unset( descriptor_type& descriptor)
               {
                  call::Timeout::instance().current( descriptor);
               }

               ~Unset()
               {
                  call::Timeout::instance().unset();
               }
            };


         private:

            Timeout() = default;


            using timeouts_type = std::vector< Limit>;
            using transaction_timeout = std::vector< Transaction>;

            using range_type = decltype( range::make( timeouts_type().begin(), timeouts_type().end()));


            std::chrono::microseconds find( descriptor_type descriptor, const time_point& now);

            timeouts_type m_limits;
            transaction_timeout m_transactions;
         };

      } // call
   } // common


} // casual

#endif // TIMEOUT_H_
