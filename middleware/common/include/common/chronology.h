//!
//! casual
//!

#ifndef CHRONOLOGY_H_
#define CHRONOLOGY_H_

#include "common/platform.h"

#include <string>

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

            }

            template< typename D>
            constexpr auto string( D&& duration) { return detail::string< D>::value;}
         } // unit


         namespace from
         {
            common::platform::time::unit string( const std::string& value);
         } // from


      }

   }

}


#endif /* CHRONOLOGY_H_ */
