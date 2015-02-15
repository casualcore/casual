//!
//! timeout.cpp
//!
//! Created on: Oct 19, 2014
//!     Author: Lazan
//!

#include "common/call/timeout.h"

#include "common/signal.h"

#include "common/transaction/context.h"



namespace casual
{
   namespace common
   {
      namespace call
      {

         Timeout& Timeout::instance()
         {
            static Timeout singleton;
            return singleton;
         }

         void Timeout::add( descriptor_type descriptor, std::chrono::microseconds timeout, const time_point& now)
         {
            if( timeout != std::chrono::microseconds::zero() && descriptor != 0)
            {
               m_limits.emplace_back( now + timeout, descriptor);

               current( descriptor, now);

            }
         }

         void Timeout::add( const transaction::ID& trid, const std::chrono::microseconds& timeout, const time_point& now)
         {
            if( timeout != std::chrono::microseconds::zero())
            {
               m_transactions.emplace_back( now + timeout, trid);
            }
         }


         void Timeout::current( descriptor_type descriptor, const time_point& now)
         {
            auto timeout = find( descriptor, now);

            if( timeout != std::chrono::microseconds::max())
            {
               signal::timer::set( timeout);
            }
         }

         void Timeout::unset() noexcept
         {
            signal::timer::unset();
         }

         void Timeout::remove( descriptor_type descriptor) noexcept
         {
            auto found = range::find_if( m_limits,
                  std::bind( equal_to{}, std::bind( &Limit::descriptor, std::placeholders::_1), descriptor));

            if( found)
            {
               m_limits.erase( found.first);
            }
         }

         void Timeout::clear()
         {
            signal::timer::unset();
            m_limits.clear();
            m_transactions.clear();
         }


         std::chrono::microseconds Timeout::find( descriptor_type descriptor, const time_point& now)
         {
            std::chrono::microseconds result = std::chrono::microseconds::max();


            {
               auto found = range::find_if( m_limits,
                     std::bind( equal_to{}, std::bind( &Limit::descriptor, std::placeholders::_1), descriptor));

               if( found)
               {
                  result = std::chrono::duration_cast< std::chrono::microseconds>( found->timeout - now);

               }
            }
            {
               auto found = range::find( m_transactions, common::transaction::Context::instance().current().trid);

               if( found && found->timeout - now < result)
               {
                  result = std::chrono::duration_cast< std::chrono::microseconds>( found->timeout - now);
               }
            }

            return result;
         }


      } // call
   } // common


} // casual
