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
         namespace generic
         {
            namespace local
            {
               namespace
               {
                  void extract( bool print_type)
                  {
                     auto dispatch = [print_type]( auto&& payload)
                     {
                        if( print_type)
                           std::cerr << payload.type << '\n';

                        std::cout.write( payload.memory.data(), payload.memory.size());
                        std::cout.flush();                        
                     };
                     
                     common::buffer::payload::binary::stream( std::cin, dispatch); 
                  }

                  void main( int argc, char** argv)
                  {
                     bool print_type = false;

                     argument::Parse{ R"(Read the buffers from stdin and extract the payload and streams it to stdout)",
                        argument::Option{ argument::option::toggle( print_type), { "--print-type"}, "prints the type of the buffer(s) to stderr"}
                     }( argc, argv);
                     
                     extract( print_type);
                  }
                  
               } // <unnamed>
            } // local
         } // generic

      } // tools
   } // buffer
   
} // casual


int main( int argc, char** argv)
{
   return casual::common::exception::guard( [&]()
   {
      casual::buffer::tools::generic::local::main( argc, argv);
   });
} 