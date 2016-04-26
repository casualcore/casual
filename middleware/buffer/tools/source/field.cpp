//
// field_header.cpp
//
//  Created on: 8 feb 2015
//      Author: Kristone
//

#include <map>
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

            std::map<std::string,int> name_to_type();
            std::map<int,std::string> type_to_name();

            std::map<std::string,long> name_to_id();
            std::map<long,std::string> id_to_name();
         }

      }

   }

}


int main( int argc, char* argv[])
{
   if( argc > 1)
   {
      casual::common::environment::variable::set( "CASUAL_FIELD_TABLE", argv[1]);
   }

   try
   {
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

   return -1;

}
