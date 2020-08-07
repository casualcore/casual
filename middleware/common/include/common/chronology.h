//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "casual/platform.h"
#include "common/stream.h"

#include <string>
#include <chrono>

namespace casual
{
   namespace common
   {
      namespace chronology
      {
         namespace utc
         {
            //! Format a timepoint to iso 8601 extended date time with UTC offset
            //! @{
            void offset( std::ostream& out, platform::time::point::type time);
            std::string offset( platform::time::point::type time);
            //! @}
         } // utc


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
               struct string< std::chrono::duration< double>> { constexpr static auto value = "s";};

            }

            template< typename D>
            constexpr auto string( D&& duration) { return detail::string< std::decay_t< D>>::value;}
         } // unit


         namespace from
         {
            platform::time::unit string( const std::string& value);
         } // from


         struct format
         {
            template< typename D>
            void operator () ( std::ostream& out, D&& duration) const
            {
               out << duration.count() << unit::string( duration);
            }
         };

         template< typename D>
         std::string duration( D&& duration) 
         { 
            return std::to_string( duration.count()) + unit::string( duration);
         }

      } // chronology

      namespace stream
      {
         namespace customization
         {
            template< typename R, typename P>
            struct point< std::chrono::duration< R, P>>
            {
               template< typename D>
               static void stream( std::ostream& out, D&& duration)
               {
                  out << duration.count() << chronology::unit::string( duration);
               }
            };

            template<>
            struct point< platform::time::point::type>
            {
               static void stream( std::ostream& out, const platform::time::point::type& time)
               {
                  chronology::utc::offset( out, time);
               }
            };
         } // customization

      } // stream
   } // common
} // casual

