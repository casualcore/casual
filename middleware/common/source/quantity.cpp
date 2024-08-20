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
               struct map
               {
                  bytes_type unit;
                  std::string_view symbol;
                  
                  bool operator == ( bytes_type rhs) const noexcept { return unit == rhs;}
                  bool operator == ( std::string_view rhs) const noexcept { return symbol == rhs;}
               };

               constexpr std::array< map, 10> unit_mappings{{
                  { unit::byte, ""},
                  { unit::byte, symbol::byte},
                  { unit::kilobyte, symbol::kilobyte},
                  { unit::kibibyte, symbol::kibibyte},
                  { unit::megabyte, symbol::megabyte},
                  { unit::mebibyte, symbol::mebibyte},
                  { unit::gigabyte, symbol::gigabyte},
                  { unit::gibibyte, symbol::gibibyte},
                  { unit::terabyte, symbol::terabyte},
                  { unit::tebibyte, symbol::tebibyte}
               }};

               namespace parse
               {
                  std::optional< bytes_type> symbol( std::string_view symbol)
                  {
                     if( auto found = algorithm::find( unit_mappings, symbol))
                        return found->unit;

                     return std::nullopt;
                  }
               } // parse

               namespace string
               {
                  std::string representation( bytes_type value)
                  {
                     auto representable_as_integer = [ &value]( auto& pair)
                     {
                        return value % pair.unit == 0;
                     };

                     // find the largest unit for which _value_ can be respresented as an integer, prefer binary units
                     auto found = algorithm::find_if( range::reverse( unit_mappings), representable_as_integer);
                     casual::assertion( found, "failed to find symbol for byte quantity");
                     return common::string::compose( value / found->unit, found->symbol);
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
