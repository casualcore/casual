//! 
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "casual/buffer/internal/field/string.h"
#include "casual/buffer/internal/common.h"

#include "common/log/line.h"

#include "common/code/raise.h"
#include "common/code/xatmi.h"


#include <unordered_map>

namespace casual
{
   using namespace common;

   namespace buffer
   {
      namespace internal
      {
         namespace field
         {
            namespace detail
            {
               void check( int result)
               {
                  switch( result)
                  {
                     case CASUAL_FIELD_SUCCESS: return;
                     case CASUAL_FIELD_INVALID_HANDLE: common::code::raise::error( common::code::xatmi::argument);
                     case CASUAL_FIELD_INVALID_ARGUMENT: throw std::invalid_argument{ "invalid argument"};
                     case CASUAL_FIELD_OUT_OF_MEMORY: throw std::bad_alloc{};
                     case CASUAL_FIELD_OUT_OF_BOUNDS: throw std::out_of_range{ "field not consumable"};
                     default: throw std::domain_error{ "system error"};
                  }
               }

            } // detail
            namespace string 
            {
               namespace local
               {
                  namespace
                  {
                     namespace global
                     {
                        std::unordered_map< std::string, Convert> actions;
                     } // global
                     
                     Convert& convert( const std::string& key)
                     {
                        auto found = global::actions.find( key);
                        if( found == std::end( global::actions))
                           throw std::invalid_argument{ "key: " + key + " not registred"};

                        return found->second;
                     }
                      
                  } // <unnamed>
               } // local
               

               
               namespace convert
               {
                  bool registration( std::string key, Convert action)
                  {
                     if( local::global::actions.count( key) > 0)
                        log::line( log::category::warning, "field string convert - key: ", key, " already registred - action: keep current");
                     else
                        local::global::actions.emplace( std::move( key), std::move( action));

                     return true; // dummy
                  }

                  void from( const std::string& key, stream::Input string, char** buffer)
                  {
                     field::stream::Output output{ buffer};
                     local::convert( key).from( string, output);
                  }

                  stream::Output to( const std::string& key, const char* buffer, stream::Output string)
                  {
                     field::stream::Input input{ buffer};
                     local::convert( key).to( input, string);
                     return string;
                  }
                  
               } // convert


            } // string
         } // field
      } // internal
   } // buffer
} // casual