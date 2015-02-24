//!
//! string.cpp
//!
//! Created on: Apr 6, 2014
//!     Author: Lazan
//!

#include "common/string.h"
#include <cxxabi.h>


namespace casual
{
   namespace common
   {
      namespace type
      {
         namespace internal
         {
            std::string name( const std::type_info& type)
            {
               int status;
               return abi::__cxa_demangle( type.name(), 0, 0, &status);
            }
         } // internal
      } // type


      namespace string
      {

         std::vector< std::string> split( const std::string& line, typename std::string::value_type delimiter)
         {
            std::vector< std::string> result;

            auto current = std::begin( line);

            while( current != std::end( line))
            {
               auto found = std::find( current, std::end( line), delimiter);

               result.emplace_back( current, found);

               current = found;

               if( current != std::end( line))
               {
                  ++current;
               }
            }
            return result;
         }

         namespace adjacent
         {

            std::vector< std::string> split( const std::string& line, typename std::string::value_type delimiter)
            {
               typedef typename std::string::value_type value_type;

               std::vector< std::string> result;

               auto current = std::begin( line);

               while( current != std::end( line))
               {
                  auto found = std::find( current, std::end( line), delimiter);

                  if( current != found)
                  {
                     result.emplace_back( current, found);
                  }

                  current = std::find_if( found, std::end( line), [=]( value_type value) { return value != delimiter;});
               }
               return result;
            }

         } // adjacent

      } // string
   } // common
} // casual
