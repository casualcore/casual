//
// field_header.cpp
//
//  Created on: 8 feb 2015
//      Author: Kristone
//

#include "common/environment.h"
#include "buffer/field.h"
#include "buffer/internal/field.h"

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
         std::cout << std::endl;
      }

      return 0;

   }
   catch( const std::exception& e)
   {
      std::cerr << e.what() << std::endl;
   }
   catch( ...)
   {
      std::cerr << "Unknown failure" << std::endl;
   }

   return -1;

}
