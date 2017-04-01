//!
//! casual 
//!

#include "common/unittest.h"

#include "common/signal.h"


#include <random>

namespace casual
{
   namespace common
   {
      namespace unittest
      {
         namespace clean
         {

            Scope::Scope() { signal::clear();}
            Scope::~Scope() { signal::clear();}


         } // clean

         namespace local
         {
            namespace
            {
               std::size_t transform_size( std::size_t size)
               {
                  const auto size_type_size = sizeof( platform::binary::type::size_type);

                  if( size < size_type_size)
                  {
                     throw exception::invalid::Argument{ "mockup message size is to small"};
                  }
                  return size - size_type_size;
               }
            } // <unnamed>
         } // local

         Message::Message() = default;
         Message::Message( std::size_t size) : payload( local::transform_size( size)) {}

         std::size_t Message::size() const { return payload.size() + sizeof( platform::binary::type::size_type);}

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

            platform::binary::type binary( std::size_t size)
            {
               platform::binary::type result( size);

               local::randomize( result);

               return result;
            }

            unittest::Message message( std::size_t size)
            {
               unittest::Message result( size);

               local::randomize( result.payload);
               return result;
            }
         } // random


      } // unittest
   } // common
} // casual

