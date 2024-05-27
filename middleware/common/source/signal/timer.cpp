//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/signal/timer.h"
#include "common/signal.h"

#include "common/code/system.h"
#include "common/log.h"
#include "common/process.h"

#include <iomanip>

#include <sys/time.h>


std::ostream& operator << ( std::ostream& out, const ::itimerval& value)
{
   std::chrono::duration< double> second = std::chrono::seconds{ value.it_value.tv_sec} + std::chrono::microseconds{ value.it_value.tv_usec};
   return out << std::setprecision( 6) << std::fixed << second.count() << "s";
}

namespace casual
{
   namespace common::signal::timer
   {

      namespace local
      {
         namespace
         {
            unit::type convert( const itimerval& value)
            {
               if( value.it_value.tv_sec == 0 && value.it_value.tv_usec == 0)
                  return {};

               return std::chrono::seconds( value.it_value.tv_sec) + std::chrono::microseconds( value.it_value.tv_usec);
            }

            unit::type get()
            {
               itimerval old{};

               if( ::getitimer( ITIMER_REAL, &old) != 0)
                  code::system::raise( "timer::get");

               return convert( old);
            }

            unit::type set( itimerval& value)
            {
               itimerval old{};

               if( ::setitimer( ITIMER_REAL, &value, &old) != 0)
                  code::system::raise( "timer::set");

               log::line( verbose::log, "timer::set new: ", value, " - old: ", old);

               return convert( old);
            }
         } // <unnamed>
      } // local

      unit::type set( unit::type offset)
      {
         if( ! offset)
            return unset();

         auto microseconds = std::chrono::duration_cast< std::chrono::microseconds>( offset.value());

         if( microseconds <= std::chrono::microseconds::zero())
         {
            // We send the signal directly
            log::line( log::debug, "timer - offset is less than zero: ", offset, " - send alarm directly");
            signal::send( process::id(), code::signal::alarm);
            return local::get();
         }
         else
         {
            
            itimerval value;
            value.it_interval.tv_sec = 0;
            value.it_interval.tv_usec = 0;
            value.it_value.tv_sec = std::chrono::duration_cast< std::chrono::seconds>( microseconds).count();
            value.it_value.tv_usec = ( microseconds % std::chrono::seconds{ 1}).count();

            return local::set( value);
         }
      }

      unit::type get()
      {
         return local::get();
      }

      unit::type unset()
      {
         itimerval value = {};

         return local::set( value);
      }



      Scoped::Scoped( unit::type timeout, platform::time::point::type now)
      {
         auto old = timer::set( timeout);

         if( old)
         {
            m_old = now + old.value();
            log::line( verbose::log, "old timepoint: ", m_old.value().time_since_epoch());
         }
      }

      Scoped::Scoped( unit::type timeout)
         : Scoped( timeout, platform::time::clock::type::now())
      {}

      Scoped::~Scoped()
      {
         log::line( verbose::log, "Scoped::~Scoped(): ", *this);

         if( m_active)
         {
            if( m_old)
               timer::set( m_old.value() - platform::time::clock::type::now());
            else
               timer::unset();
         }
      }

      Scoped::Scoped( Scoped&& other) noexcept = default;
      Scoped& Scoped::operator = ( Scoped&& other) noexcept = default;


      Deadline::Deadline( point::type deadline, platform::time::point::type now)
      {
         if( deadline)
            timer::set( deadline.value() - now);
         else
            timer::unset();
      }

      Deadline::Deadline( platform::time::unit duration)
      {
         timer::set( duration);
      }

      Deadline::~Deadline()
      {
         if( m_active)
            timer::unset();
      }

      Deadline::Deadline( Deadline&&) noexcept = default;
      Deadline& Deadline::operator = ( Deadline&&) noexcept = default;

      std::ostream& operator << ( std::ostream& out, const Deadline& value)
      {
         if( value.m_active)
            return stream::write( out, timer::get());
         return out;
      }

   } // common::signal::timer
} // casual