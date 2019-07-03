//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/exception/handle.h"
#include "common/platform.h"
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
                  void compose( std::string type)
                  {
                     common::buffer::Payload payload{ std::move( type)};

                     while( std::cin.peek() != std::istream::traits_type::eof())
                        payload.memory.push_back( std::cin.get());
                     
                     common::buffer::payload::binary::stream( payload, std::cout); 
                  }

                  void main( int argc, char** argv)
                  {
                     std::string type = common::buffer::type::x_octet();

                     auto completion = []( auto values, auto help) -> std::vector< std::string> 
                     {
                        return {
                           common::buffer::type::x_octet(), 
                           common::buffer::type::binary(),
                           common::buffer::type::yaml(),
                           common::buffer::type::xml(),
                           common::buffer::type::json(),
                           common::buffer::type::ini(), 
                        };
                     };

                     argument::Parse{ R"(Read "binary" data from stdin and compose one buffer (with supplied type) and strems it to stdout)",
                        argument::Option{ std::tie( type), completion, { "--type"}, "type of the composed buffer"}
                     }( argc, argv);
                     
                     compose( std::move( type));
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