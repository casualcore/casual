//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/chronology.h"

#include "common/environment.h"
#include "common/range/adapter.h"

#include "common/code/raise.h"
#include "common/code/casual.h"

#include <ctime>

#include <sstream>
#include <iomanip>
#include <functional>
#include <algorithm>

namespace casual
{
   namespace common::chronology
   {
      namespace local
      {
         namespace
         {
            namespace detail
            {
               namespace global
               {
                  struct local_time_t
                  {
                     local_time_t()
                     {
                        // initialize the TZ stuff...
                        // To be conformant we "need to do this".
                        ::tzset();
                     }
                     
                     void operator()( const ::time_t& timer, ::tm& time)
                     {
                        ::localtime_r( &timer, &time);
                     }
                  };

                  local_time_t local_time{};

               } // global
               
               auto format( std::ostream& out, const tm& time, std::chrono::microseconds fraction, traits::priority::tag< 1>) 
                  -> decltype( void( time.tm_gmtoff))
               {
                  // just to help when we go to hours and minutes.
                  const auto offset = std::abs( time.tm_gmtoff);
                  
                  out << std::setfill( '0') <<
                  std::setw( 4) << time.tm_year + 1900 << '-' <<
                  std::setw( 2) << time.tm_mon + 1 << '-' <<
                  std::setw( 2) << time.tm_mday << 'T' <<
                  std::setw( 2) << time.tm_hour << ':' <<
                  std::setw( 2) << time.tm_min << ':' <<
                  std::setw( 2) << time.tm_sec << '.' <<
                  std::setw( 6) << fraction.count() << 
                  ( time.tm_gmtoff < 0 ? '-' : '+') << 
                  // get the 'hour' part
                  std::setw( 2) << offset / 3600 << ':' << 
                  // get the 'minute' part
                  std::setw( 2) << ( offset % 3600) / 60;
               }

               // implement an alternative implementation if tm does not have a tm_gmtoff on some platform.
               // we don't do it until we know there is one we _suport_
               // void format( std::ostream& out, const tm& time, std::chrono::microseconds fraction, traits::priority::tag< 0>)
               
               
            } // detail

            void format( std::ostream& out, platform::time::point::type timepoint)
            {
               if( timepoint == platform::time::point::limit::zero())
                  return;

               auto timer = platform::time::clock::type::to_time_t( timepoint);

               ::tm time;
               detail::global::local_time( timer, time);

               const auto us = std::chrono::duration_cast< std::chrono::microseconds>( timepoint.time_since_epoch());

               detail::format( out, time, us % ( 1000 * 1000), traits::priority::tag< 1>{});
            }

         } // <unnamed>
      } // local

      namespace utc
      {
         std::string offset( platform::time::point::type timepoint)
         {
            std::ostringstream out;
            offset( out, timepoint);
            return std::move( out).str();
         }

         void offset( std::ostream& out, platform::time::point::type timepoint)
         {
            local::format( out, timepoint);
         }
      } // utc

      namespace from
      {
         namespace local
         {
            namespace
            {
               template< typename R>
               platform::time::unit string( R&& value)
               {
                  auto is_ws = []( auto c){ return c != ' ';};

                  // trim ws.
                  value = algorithm::find_if( range::reverse( algorithm::find_if( value, is_ws)), is_ws);

                  auto split = algorithm::divide_if( value, []( auto c)
                  {
                     return c < '0' || c > '9';
                  });

                  // extract the count part
                  auto count = [number = std::get< 0>( split)]( )
                  {
                     using count_type = decltype( platform::time::unit{}.count());
                     if( number)
                        return common::string::from< count_type>( string::view::make( number));
                     return count_type{ 0};
                  }();

                  // extract the unit part, and convert to std::string_view to be able to compare to char*
                  auto unit = string::view::make( std::get< 1>( split));

                  if( unit.empty() || unit == "s") return std::chrono::seconds( count);                     
                  if( unit == "us") return std::chrono::microseconds( count);
                  if( unit == "ms") return std::chrono::milliseconds( count);
                  if( unit == "min") return std::chrono::minutes( count);
                  if( unit == "h") return std::chrono::hours( count);
                  if( unit == "d") return std::chrono::hours( count * 24);
                  
                  if( unit == "ns") 
                  {
                     auto point = std::chrono::nanoseconds( count);
                     if constexpr( std::is_same_v< platform::time::unit, std::chrono::nanoseconds>)
                        return std::chrono::duration_cast< platform::time::unit>( point);
                     else
                        return std::chrono::round< platform::time::unit>( point);
                  }

                  code::raise::error( code::casual::invalid_argument, "invalid time representation: ", string::view::make( value));
               }

            } // <unnamed>
         } // local

         platform::time::unit string( const std::string& value)
         {
            using time_unit = platform::time::unit;

            // we split on '+', and provide _next range_ for the range-adapter 
            auto next_range = []( auto range)
            { 
               range = algorithm::find_if( range, []( auto c){ return c != ' ';});
               return algorithm::split( range, '+');
            };
            
            auto accumulate_time = []( time_unit current, auto range)
            {
               return current + local::string( range);
            };

            return algorithm::accumulate( 
               range::adapter::make( next_range, range::make( value)), 
               time_unit{}, 
               accumulate_time);
         }
      } // from

      namespace to
      {
         namespace local
         {
            namespace
            {
               template< typename D, typename T> 
               bool stream( std::ostream& out, std::chrono::nanoseconds duration, T unit, bool delimiter)
               {
                  auto value = std::chrono::duration_cast< D>( duration % unit);
                  if( value == T::zero())
                     return delimiter;
                  
                  if( delimiter)
                     common::stream::write( out, " + ", value);
                  else 
                     common::stream::write( out, value);

                  return true;
               };
            } // <unnamed>
         } // local

         std::string string( std::chrono::nanoseconds duration)
         {
            if( duration == platform::time::unit::zero())
               return {};

            std::ostringstream out;

            // initialize with hours
            auto delimiter = [&]()
            {
               auto value = std::chrono::duration_cast< std::chrono::hours>( duration);

               if( value == std::chrono::hours::zero())
                  return false;

               common::stream::write( out, value);
               return true;
            }();

            delimiter = local::stream< std::chrono::minutes>( out, duration, std::chrono::hours{ 1}, delimiter);
            delimiter = local::stream< std::chrono::seconds>( out, duration, std::chrono::minutes{ 1}, delimiter);
            delimiter = local::stream< std::chrono::milliseconds>( out, duration, std::chrono::seconds{ 1}, delimiter);
            delimiter = local::stream< std::chrono::microseconds>( out, duration, std::chrono::milliseconds{ 1}, delimiter);
            local::stream< std::chrono::nanoseconds>( out, duration, std::chrono::microseconds{ 1}, delimiter);

            
            return std::move( out).str();
         }

      } // from

   } // common::chronology
} // casual
