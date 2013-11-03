/*
 * chronology.cpp
 *
 *  Created on: 5 maj 2013
 *      Author: Kristone
 */

#include "common/chronology.h"

#include <ctime>

#include <sstream>
#include <iomanip>
#include <functional>

namespace casual
{

namespace common
{

namespace chronology
{

   namespace internal
   {

      template<typename F>
      std::string format( const common::time_type& time, F function)
      {
         //
         // to_time_t does not exist as a static member in common::clock_type
         //
         const std::time_t seconds = std::chrono::duration_cast< std::chrono::seconds>( time.time_since_epoch()).count();
         const auto tm = function( &seconds);

         const auto ms = std::chrono::duration_cast< std::chrono::milliseconds>( time.time_since_epoch());

         std::ostringstream result;
         result << std::setfill( '0') <<
            std::setw( 4) << tm->tm_year + 1900 << '-' <<
            std::setw( 2) << tm->tm_mon + 1 << '-' <<
            std::setw( 2) << tm->tm_mday << 'T' <<
            std::setw( 2) << tm->tm_hour << ':' <<
            std::setw( 2) << tm->tm_min << ':' <<
            std::setw( 2) << tm->tm_sec << '.' <<
            std::setw( 3) << ms.count() % 1000;
         return result.str();
      }

   }


   std::string local()
   {
      return local( common::clock_type::now());
   }

   std::string local( const common::time_type& time)
   {
      return internal::format( time, &std::localtime);
   }

   std::string universal()
   {
      return local( common::clock_type::now());
   }

   std::string universal( const common::time_type& time)
   {
      return internal::format( time, &std::gmtime);
   }

} // chronology

} // utility

} // casual
