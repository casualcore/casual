//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!



#include "casual/buffer/field.h"
#include "casual/buffer/internal/field.h"

#include "common/environment.h"
#include "common/argument.h"
#include "common/exception/handle.h"

#include <stdexcept>
#include <iostream>

namespace casual
{
   using namespace common;
   namespace buffer
   {
      namespace field
      {
         namespace local
         {
            namespace
            {
               void generate()
               {
                  const auto types = casual::buffer::field::internal::type_to_name();
                  const auto names = casual::buffer::field::internal::name_to_id();

                  algorithm::for_each( names, [&types]( auto& pair)
                  {
                     std::cout << "#define " << pair.first << '\t' << pair.second << '\t';
                     std::cout << "/* ";
                     std::cout << "number: " << (pair.second % CASUAL_FIELD_TYPE_BASE) << '\t';
                     std::cout << "type: " << types.at( pair.second / CASUAL_FIELD_TYPE_BASE);
                     std::cout << " */";
                     std::cout << '\n';
                  });
               }

               void main( int argc, char* argv[])
               {
                  std::vector< std::string> tables;
                  argument::Parse{ "generates a field definition header file",
                     argument::Option{ std::tie( tables), { "--tables"}, R"(field table paths to generate from

if not provided, environment variable CASUAL_FIELD_TABLE will be used.
)"}
                  }( argc, argv);

                  if( ! tables.empty())
                     casual::common::environment::variable::set( "CASUAL_FIELD_TABLE", string::join( tables, '|'));

                  generate();

               }
            } // <unnamed>
         } // local

      } // field
   } // buffer
} // casual


int main( int argc, char* argv[])
{
   return casual::exception::guard( std::cerr, [=]()
   {
      casual::buffer::field::local::main( argc, argv);
   });
}
