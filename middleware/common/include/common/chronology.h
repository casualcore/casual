//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "casual/platform.h"
#include "common/stream/customization.h"
#include "common/serialize/value.h"

#include <string>
#include <chrono>

namespace casual
{
   namespace common::chronology
   {
      using time_point = platform::time::clock::type::time_point;
      using duration = time_point::duration;

      namespace utc
      {
         //! Format a timepoint to iso 8601 extended date time with UTC offset
         //! @{
         void offset( std::ostream& out, time_point time);
         std::string offset( time_point time);
         //! @}
      } // utc

      //! @returns a time_point that represents 'empty/nill' timepoint
      constexpr time_point empty() noexcept { return time_point{};}

      namespace unit
      {
         namespace detail
         {
            template< typename D>
            struct string;

            template<>
            struct string< std::chrono::nanoseconds> { constexpr static auto value = "ns";};

            template<>
            struct string< std::chrono::microseconds> { constexpr static auto value = "us";};

            template<>
            struct string< std::chrono::milliseconds> { constexpr static auto value = "ms";};

            template<>
            struct string< std::chrono::seconds> { constexpr static auto value = "s";};

            template<>
            struct string< std::chrono::minutes> { constexpr static auto value = "min";};

            template<>
            struct string< std::chrono::hours> { constexpr static auto value = "h";};

            template<>
            struct string< std::chrono::duration< double>> { constexpr static auto value = "s";};

         }

         template< typename D>
         constexpr auto string() { return detail::string< std::decay_t< D>>::value;}
      } // unit


      namespace from
      {
         duration string( const std::string& value);
      } // from

      namespace to
      {
         std::string string( std::chrono::nanoseconds duration);

         template< typename R, typename P>
         auto string( std::chrono::duration< R, P> duration) 
         { 
            return to::string( std::chrono::duration_cast< std::chrono::nanoseconds>( duration));
         }
      } // to

   } // common::chronology


   // specialization for stream/log customization
   namespace common::stream::customization::supersede
   {
      template< typename R, typename P>
      struct point< std::chrono::duration< R, P>>
      {
         template< typename D>
         static void stream( std::ostream& out, D&& duration)
         {
            out << duration.count() << chronology::unit::string< D>();
         }
      };

      template<>
      struct point< chronology::time_point>
      {
         static void stream( std::ostream& out, const chronology::time_point& time)
         {
            chronology::utc::offset( out, time);
         }
      };

   } // common::stream::customization::supersede


   // specialization for serialization customization
   namespace common::serialize::customize
   {
      template< typename R, typename P, typename A>
      struct Value< std::chrono::duration< R, P>, A>
      {
         using value_type = std::chrono::duration< R, P>;

         template< typename V> 
         static void write( A& archive, V&& value, const char* name)
         {
            value::write( archive, std::chrono::duration_cast< platform::time::serialization::unit>( value).count(), name);
         }

         static bool read( A& archive, value_type& value, const char* name)
         {
            platform::time::serialization::unit::rep representation;

            if( value::read( archive, representation, name))
            {
               value = std::chrono::duration_cast< value_type>( platform::time::serialization::unit{ representation});
               return true;
            }
            return false;
         }
      };

      template< typename A>
      struct Value< chronology::time_point, A>
      {
         static void write( A& archive, chronology::time_point value, const char* name)
         {
            value::write(
               archive, 
               std::chrono::time_point_cast< platform::time::serialization::unit>( value).time_since_epoch(), 
               name);
         }

         static bool read( A& archive, chronology::time_point& value, const char* name)
         {
            platform::time::serialization::unit duration;
            if( value::read( archive, duration, name))
            {
               value = chronology::time_point{ std::chrono::duration_cast< chronology::duration>( duration)};
               return true;
            }
            return false;
         }
      };
      
   } // common::serialize::customize

} // casual

