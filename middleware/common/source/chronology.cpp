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

#include <sstream>
#include <functional>
#include <algorithm>
#include <format>

namespace casual
{
   namespace common::chronology
   {
      namespace utc
      {
         namespace local
         {
            namespace
            {
               auto offset(const auto timepoint)
               {
                  return std::format( "{:%FT%T%Ez}", std::chrono::zoned_time{ std::chrono::current_zone(), timepoint});
               }
            } //
         } // local

         std::string offset( platform::time::point::type timepoint)
         {
            return local::offset( std::chrono::floor<std::chrono::microseconds>( timepoint));
         }

         void offset( std::ostream& out, platform::time::point::type timepoint)
         {
            out << offset(timepoint);
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

      } // to

   } // common::chronology
} // casual
