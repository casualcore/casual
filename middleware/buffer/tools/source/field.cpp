//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/environment.h"
#include "casual/buffer/field.h"
#include "casual/buffer/internal/field.h"

#include <stdexcept>
#include <iostream>


int main( int argc, char* argv[])
{
   try
   {
      if( argc > 1)
      {
         casual::common::environment::variable::set( "CASUAL_FIELD_TABLE", argv[1]);
      }

      const auto types = casual::buffer::field::internal::type_to_name();


      const auto names = casual::buffer::field::internal::name_to_id();


      for( const auto& pair : names)
      {
         std::cout << "#define " << pair.first << '\t' << pair.second << '\t';
         std::cout << "/* ";
         std::cout << "number: " << (pair.second % CASUAL_FIELD_TYPE_BASE) << '\t';
         std::cout << "type: " << types.at( pair.second / CASUAL_FIELD_TYPE_BASE);
         std::cout << " */";
         std::cout << '\n';
      }

      return 0;

   }
   catch( const std::exception& e)
   {
      std::cerr << e.what() << '\n';
   }
   catch( ...)
   {
      std::cerr << "Unknown failure" << '\n';
   }

   return -1;

}
