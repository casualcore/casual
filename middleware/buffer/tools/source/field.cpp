//
// field_header.cpp
//
//  Created on: 8 feb 2015
//      Author: Kristone
//

#include <unordered_map>
#include <string>

#include "common/environment.h"
#include "buffer/field.h"

#include <stdexcept>
#include <iostream>


namespace casual
{
   namespace buffer
   {
      namespace field
      {
         namespace repository
         {
            //
            // Defined in field.cpp
            //

            std::unordered_map<std::string,int> name_to_type();
            std::unordered_map<int,std::string> type_to_name();

            std::unordered_map<std::string,long> name_to_id();
            std::unordered_map<long,std::string> id_to_name();
         }

      }

   }

}


int main( int argc, char* argv[])
{
   try
   {
      if( argc > 1)
      {
         casual::common::environment::variable::set( "CASUAL_FIELD_TABLE", argv[1]);
      }

      const auto types = casual::buffer::field::repository::type_to_name();


      const auto names = casual::buffer::field::repository::name_to_id();


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
