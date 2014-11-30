//!
//! timeout.cpp
//!
//! Created on: Oct 19, 2014
//!     Author: Lazan
//!

#include "common/call/timeout.h"

#include "common/signal.h"



namespace casual
{
   namespace common
   {
      namespace call
      {

         namespace local
         {
            namespace
            {
               template< typename L>
               auto find( L& limits, Timeout::descriptor_type descriptor) -> decltype( range::make( limits))
               {
                  return range::find_if( limits,
                     chain::Or::link(
                        compare::equal_to( std::mem_fn( &Timeout::Limit::descriptor), bind::value( descriptor)),
                        compare::equal_to( std::mem_fn( &Timeout::Limit::descriptor), bind::value( Timeout::Type::cTransaction))));

               }
            } // <unnamed>
         } // local

         Timeout& Timeout::instance()
         {
            static Timeout singleton;
            return singleton;
         }

         void Timeout::add( descriptor_type descriptor, std::chrono::microseconds timeout)
         {
            if( timeout != std::chrono::microseconds::zero())
            {
               auto now = platform::clock_type::now();
               add( { now + timeout, descriptor}, now);
            }
         }

         void Timeout::add( descriptor_type descriptor, platform::time_point limit)
         {
            add( { std::move( limit), descriptor}, platform::clock_type::now());
         }


         Timeout::Limit Timeout::get() const
         {
            if( m_limits.empty())
            {
               return { time_point::max(), Type::cUnset};
            }
            return m_limits.front();
         }

         bool Timeout::passed( descriptor_type descriptor) const
         {
            auto found = local::find( m_limits, descriptor);

            if( found)
            {
               return found->timeout <= platform::clock_type::now();
            }
            return false;
         }

         bool Timeout::remove( descriptor_type descriptor)
         {
            auto found = range::find_if( m_limits,
                  compare::equal_to( std::mem_fn( &Limit::descriptor), bind::value( descriptor)));

            if( found)
            {
               auto limit = *found;
               m_limits.erase( found.first);

               auto now = platform::clock_type::now();

               if( limit.descriptor == m_current)
               {
                  //
                  // current timer is based on this descriptor, we
                  // have to reset the alarm
                  //
                  set( now);

               }
               return limit.timeout <= now;
            }
            return false;
         }

         void Timeout::clear()
         {
            if( ! m_limits.empty())
            {
               signal::timer::unset();
               m_limits.clear();
            }
            m_current = Type::cUnset;
         }

         void Timeout::add( Limit limit, time_point now)
         {
            m_limits.insert(
               std::lower_bound( std::begin( m_limits), std::end( m_limits), limit),
               std::move( limit));

            set( now);
         }

         void Timeout::set( time_point now)
         {
            //
            // Set the alarm
            //
            auto future = std::upper_bound( std::begin( m_limits), std::end( m_limits), Limit{ now, Type::cUnset});

            if( future != std::end( m_limits))
            {
               if( future->descriptor != m_current)
               {
                  //
                  // We only reset the alarm if it is a new earlier one...
                  //
                  signal::timer::set( future->timeout - now);
                  m_current = future->descriptor;
               }
            }
            else
            {
               m_current = Type::cUnset;
            }
         }

      } // call
   } // common


} // casual
