//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/strong/id.h"
#include "common/algorithm/random.h"
#include "common/transcode.h"
#include "common/stream.h"

#include <ostream>

namespace casual
{
   namespace common::strong
   {
      namespace resource
      {
         std::ostream& policy::stream( std::ostream& out, platform::resource::native::type value)
         {
            if( value < 0) 
               return out << "E-" << std::abs( value);
            if( value > 0) 
               return out << "L-" << value;
            return out << "nil";
         }

         platform::resource::native::type policy::generate()
         {
            static platform::resource::native::type value{};
            return ++value;
         }

      } // resource

      namespace execution
      {
         namespace span
         {
            policy::value_type policy::generate() 
            {
               value_type result;
               auto random = algorithm::random::value< std::uint64_t>();
               algorithm::copy( std::as_bytes( std::span{ &random, 1}), result);
               return result;
            }

            bool policy::valid( const value_type& value) noexcept 
            { 
               return algorithm::any_of( value, []( auto byte)
               { 
                  return byte != std::byte{ 0};
               });
            }

            std::ostream& policy::stream( std::ostream& out, const value_type& value)
            {
               if( valid( value))
                  stream::write( out, value);
               return out;
            }

            
         } // span

      } // execution

   } // common::strong
} // casual
