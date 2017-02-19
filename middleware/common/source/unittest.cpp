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

               } // <unnamed>
            } // local

            platform::binary::type::value_type byte()
            {
               using limit_type = std::numeric_limits< platform::binary::type::value_type>;

               std::uniform_int_distribution<> distribution( limit_type::min(), limit_type::max());

               return distribution( local::engine());
            }

            platform::binary::type binary( std::size_t size)
            {
               platform::binary::type result( size);

               for( auto& value : result)
               {
                  value = byte();
               }

               return result;
            }
         } // random


      } // unittest
   } // common
} // casual

