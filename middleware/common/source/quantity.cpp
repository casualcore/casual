//! 
//! Copyright (c) 2024, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!
#include "common/quantity.h"

#include "common/algorithm.h"
#include "common/string.h"
#include "common/range.h"

#include "common/string/compose.h"

#include "common/code/raise.h"
#include "common/code/casual.h"

#include "casual/assert.h"

#include <string>
#include <map>
#include <optional>

namespace casual
{
   namespace common::quantity
   {

      namespace bytes
      {
         namespace local
         {
            namespace
            {
               namespace parse
               {
                  std::optional< bytes_type> symbol( std::string_view symbol)
                  {
                     static const std::map< std::string_view, bytes_type> symbol_map{
                        { "", unit::byte },
                        { symbol::byte, unit::byte },
                        { symbol::kilobyte, unit::kilobyte },
                        { symbol::kibibyte, unit::kibibyte },
                        { symbol::megabyte, unit::megabyte },
                        { symbol::mebibyte, unit::mebibyte },
                        { symbol::gigabyte, unit::gigabyte },
                        { symbol::gibibyte, unit::gibibyte },
                        { symbol::terabyte, unit::terabyte },
                        { symbol::tebibyte, unit::tebibyte }
                     };

                     if( auto found = symbol_map.find( symbol); found != std::end( symbol_map))
                        return found->second;

                     return std::nullopt;
                  }
               } // parse

               namespace string
               {
                  std::string representation( bytes_type value)
                  {
                     static const std::map< bytes_type, std::string_view> unit_map{
                        { unit::byte, symbol::byte},
                        { unit::kilobyte, symbol::kilobyte},
                        { unit::kibibyte, symbol::kibibyte},
                        { unit::megabyte, symbol::megabyte},
                        { unit::mebibyte, symbol::mebibyte},
                        { unit::gigabyte, symbol::gigabyte},
                        { unit::gibibyte, symbol::gibibyte},
                        { unit::terabyte, symbol::terabyte},
                        { unit::tebibyte, symbol::tebibyte}
                     };

                     auto representable_as_integer = [ &value]( auto& pair)
                     {
                        return value % pair.first == 0;
                     };

                     // find the largest unit for which _value_ can be respresented as an integer, prefer binary units
                     auto found = algorithm::find_if( range::reverse( unit_map), representable_as_integer);
                     casual::assertion( found, "failed to find symbol for byte quantity");
                     return common::string::compose( value / found->first, found->second);
                  }
               } // string
            } // <unnamed>
         } // local

         namespace from
         {
            bytes_type string( std::string value)
            {
               auto is_space = []( auto& c){ return c == ' ';};

               auto [ count, symbol] = algorithm::divide_if( algorithm::remove_if( value, is_space), []( auto& c)
               {
                  return c < '0' || c > '9';
               });
               
               if( auto unit = local::parse::symbol( string::view::make( symbol)))
               {
                  auto base = string::from< bytes_type>( string::view::make( count));

                  return base * unit.value();
               }

               code::raise::error( code::casual::invalid_argument, "invalid byte size representation: ", string::view::make( value));
            }
         } // from

         namespace to
         {
            std::string string( bytes_type value)
            {
               return local::string::representation( value);
            }
         } // to

      } // bytes
   } // common::quantity
} // casual
