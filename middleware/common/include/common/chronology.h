//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "common/platform.h"
#include "common/log/stream.h"

#include <string>
#include <chrono>

namespace casual
{
   namespace common
   {
      namespace chronology
      {
         std::string local();
         std::string local( const platform::time::point::type& time);
         std::string universal();
         std::string universal( const platform::time::point::type& time);

         namespace unit
         {
            namespace detail
            {
               template< typename D>
               struct string;

               template<>
               struct string<  std::chrono::nanoseconds> { constexpr static auto value = "ns";};

               template<>
               struct string<  std::chrono::microseconds> { constexpr static auto value = "us";};

               template<>
               struct string<  std::chrono::milliseconds> { constexpr static auto value = "ms";};

               template<>
               struct string<  std::chrono::seconds> { constexpr static auto value = "s";};

            }

            template< typename D>
            constexpr auto string( D&& duration) { return detail::string< std::decay_t< D>>::value;}
         } // unit


         namespace from
         {
            common::platform::time::unit string( const std::string& value);
         } // from

         struct format
         {
            template< typename D>
            void operator () ( std::ostream& out, D&& duration) const
            {
               out << duration.count() << unit::string( duration);
            }
         };

      } // chronology

      namespace log
      {
         template< typename R, typename P>
         struct has_formatter< std::chrono::duration< R, P>> : std::true_type
         {
            using formatter = chronology::format;
         };
      } // log


   } // common
} // casual



