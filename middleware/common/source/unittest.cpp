//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/unittest.h"


//#include "common/message/event.h"
//#include "common/message/handle.h"

#include "common/exception/system.h"

// std
#include <random>

namespace casual
{
   namespace common
   {
      namespace unittest
      {
         namespace local
         {
            namespace
            {
               size_type transform_size( size_type size)
               {
                  const size_type size_type_size = sizeof( platform::binary::type::size_type);

                  if( size < size_type_size)
                  {
                     throw exception::system::invalid::Argument{ "mockup message size is to small"};
                  }
                  return size - size_type_size;
               }
            } // <unnamed>
         } // local

         Message::Message() = default;
         Message::Message( size_type size) : payload( local::transform_size( size)) {}

         size_type Message::size() const { return payload.size() + sizeof( platform::binary::type::size_type);}

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

            platform::binary::type::value_type byte()
            {
               auto distribution = local::distribution();

               return distribution( local::engine());
            }

            platform::binary::type binary( size_type size)
            {
               platform::binary::type result( size);

               local::randomize( result);

               return result;
            }

            std::string string( size_type size)
            {
               std::string result( size, '\0');

               std::uniform_int_distribution< short> distribution( 32, 123);

               for( auto& value : result)
                  value = distribution( local::engine());

               return result;
            }

            unittest::Message message( size_type size)
            {
               unittest::Message result( size);

               local::randomize( result.payload);
               return result;
            }
         } // random

      } // unittest
   } // common
} // casual

