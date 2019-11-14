//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/exception/handle.h"
#include "casual/platform.h"
#include "common/argument.h"
#include "common/buffer/type.h"

namespace casual
{
   using namespace common;

   namespace buffer
   {
      namespace tools
      {
         namespace local
         {
            namespace
            {
               void duplicate( platform::size::type count)
               {
                  auto dispatch = [count]( auto&& payload)
                  {
                     for( platform::size::type copies = 0; copies < count; ++copies)
                     {
                        common::buffer::payload::binary::stream( payload, std::cout);
                     }
                  };
                  common::buffer::payload::binary::stream( std::cin, dispatch); 
               }

               void main( int argc, char** argv)
               {
                  platform::size::type count = 1;

                  argument::Parse{ R"(duplicates buffers from stdin to stdout 

   `count` amount of times. 
   )",
                     argument::Option{ std::tie( count), { "--count"}, "number of 'duplications', applied for each buffer"}
                  }( argc, argv);
                  
                  duplicate( count);
               } 

            } // <unnamed>
         } // local
      } // tools
   } // buffer
} // casual

int main( int argc, char** argv)
{
   return casual::common::exception::guard( [&]()
   {
      casual::buffer::tools::local::main( argc, argv);
   });
} 