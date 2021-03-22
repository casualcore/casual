//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/unittest.h"

#include "common/code/raise.h"
#include "common/code/casual.h"


// std
#include <random>

namespace casual
{
   namespace common::unittest
   {

      Message::Message() = default;
      
      Message::Message( platform::binary::type payload) 
         : payload{ std::move( payload)} {}

      Message::Message( platform::size::type size) 
      : Message{ random::binary( size)} {}

      namespace message::transport
      {
         unittest::Message size( platform::size::type size)
         {
            constexpr platform::size::type overhead = sizeof( platform::size::type) + sizeof( Uuid::uuid_type);
            assert( size >= overhead);

            return { size - overhead};
         }
         
      } // message::transport


      namespace random
      {

         namespace local
         {
            namespace
            {
               std::mt19937& engine()
               {
                  //static std::random_device device;
                  static std::mt19937 engine{ std::random_device{}()};
                  return engine;
               }

               auto distribution()
               {
                  using limit_type = std::numeric_limits< platform::binary::type::value_type>;

                  std::uniform_int_distribution<> distribution( limit_type::min(), limit_type::max());

                  return distribution;
               }

               template< typename C>
               void randomize( C& container)
               {
                  auto dist = local::distribution();

                  for( auto& value : container)
                  {
                     value = dist( local::engine());
                  }
               }

            } // <unnamed>
         } // local

         namespace detail
         {
            long long integer()
            {
               std::uniform_int_distribution< long long> distribution( std::numeric_limits< long long>::min(), std::numeric_limits< long long>::max());
               return distribution( local::engine());
            }
         } // detail

         platform::binary::type::value_type byte()
         {
            auto distribution = local::distribution();

            return distribution( local::engine());
         }

         platform::binary::type binary( platform::size::type size)
         {
            platform::binary::type result( size);

            local::randomize( result);

            return result;
         }

         std::string string( platform::size::type size)
         {
            std::string result( size, '\0');

            std::uniform_int_distribution< short> distribution( 32, 123);

            for( auto& value : result)
               value = distribution( local::engine());

            return result;
         }

      } // random

   } // common::unittest
} // casual

