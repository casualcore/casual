//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "casual/buffer/field.h"


#include "casual/platform.h"
#include "common/algorithm.h"
#include "common/network/byteorder.h"
#include "common/execute.h"

#include "xatmi.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <iomanip>

namespace casual
{
   namespace buffer
   {
      namespace
      {
         struct Value
         {
            char v_char = 'a';
            short v_short = 42;
            long v_long = 1024 * 1024;
            float v_float = 1 / float( 42);
            double v_double = 1 / double( 42);
            std::string v_string = "casual";
            platform::binary::type v_binary = { std::byte{ '1'}, std::byte{ '2'}, std::byte{ '3'}, std::byte{ '1'}, std::byte{ '2'}, std::byte{ '3'}, std::byte{ '1'}, std::byte{ '2'}, std::byte{ '3'}, std::byte{ '1'}, std::byte{ '2'}, std::byte{ '3'}};

            auto net_char() const { return common::network::byteorder::encode( v_char);}
            auto net_short() const { return common::network::byteorder::encode( v_short);}
            auto net_long() const { return common::network::byteorder::encode( v_long);}
            auto net_float() const { return common::network::byteorder::encode( v_float);}
            auto net_double() const { return common::network::byteorder::encode( v_double);}

         };

         const auto FLD_SHORT = CASUAL_FIELD_SHORT * CASUAL_FIELD_TYPE_BASE + 1000 + 1;
         const auto FLD_LONG = CASUAL_FIELD_LONG * CASUAL_FIELD_TYPE_BASE + 1000 + 1;
         const auto FLD_CHAR = CASUAL_FIELD_CHAR * CASUAL_FIELD_TYPE_BASE + 1000 + 1;
         const auto FLD_FLOAT = CASUAL_FIELD_FLOAT * CASUAL_FIELD_TYPE_BASE + 2000 + 1;
         const auto FLD_DOUBLE = CASUAL_FIELD_DOUBLE * CASUAL_FIELD_TYPE_BASE + 2000 + 1;
         const auto FLD_STRING = CASUAL_FIELD_STRING * CASUAL_FIELD_TYPE_BASE + 2000 + 1;
         const auto FLD_BINARY = CASUAL_FIELD_BINARY * CASUAL_FIELD_TYPE_BASE + 2000 + 1;


         void dump( std::ostream& out)
         {
            Value value;

            char* buffer = tpalloc( CASUAL_FIELD, nullptr, 0);

             auto deleter = common::execute::scope( [&](){
                tpfree( buffer);
             });

            casual_field_add_char( &buffer, FLD_CHAR, value.v_char);
            casual_field_add_short( &buffer, FLD_SHORT, value.v_short);
            casual_field_add_long( &buffer, FLD_LONG, value.v_long);
            casual_field_add_float( &buffer, FLD_FLOAT, value.v_float);
            casual_field_add_double( &buffer, FLD_DOUBLE, value.v_double);
            casual_field_add_string( &buffer, FLD_STRING, value.v_string.c_str());
            auto string_like = common::view::binary::to_string_like( value.v_binary);
            casual_field_add_binary( &buffer, FLD_BINARY, string_like.data(),  string_like.size());

            long size = 0;
            long used = 0;
            casual_field_explore_buffer( buffer, &size, &used);

            std::cout << "buffer size: " << size << " - used: " << used << '\n';
            out.write( buffer, used);
            out.flush();
         }

         template< typename T>
         auto encode( T value) { return common::network::byteorder::encode( value);}

         auto encode( std::size_t value) { return common::network::byteorder::encode( static_cast< long>( value));}

         void markdown( std::ostream& out)
         {
            out << R"(
# casual buffer fielded protocol

Defines the binary representation of the fielded buffer

Every field in the buffer has the following parts: `<field-id><size><data>`


| part          | network type | network size  | comments
|---------------|--------------|---------------|---------
| field-id      | uint64       | 8             |
| size          | uint64       | 8             | the size of the data part
| data          | <depends>    | <depends>     | Depends on the _type_ of the field id.


## example

### host representation

 type   | field-id     | size value   | value       
--------|--------------|--------------|------------ 
)";
            Value value;

            {
               out << std::fixed;
               out << "char    | " << FLD_CHAR       << "  |      " << sizeof( value.v_char)    << " | " << value.v_char << '\n';
               out << "short   | " << FLD_SHORT      << "  |      " << sizeof( value.v_short)  << " | " << value.v_short << '\n';
               out << "long    | " << FLD_LONG       << "  |      " << sizeof( value.v_long)  << " | " << value.v_long << '\n';
               out << "float   | " << FLD_FLOAT      << "  |      " << sizeof( value.v_float) << " | "  << std::setprecision( std::numeric_limits<float>::digits10 + 1) << value.v_float << '\n';
               out << "double  | " << FLD_DOUBLE     << "  |      " << sizeof( value.v_double) << " | " << std::setprecision( std::numeric_limits<double>::digits10 + 1) << value.v_double << '\n';
               out << "string  | " << FLD_STRING     << "  |      " << value.v_string.size()  << " | " << value.v_string << '\n';
               auto string_like = common::view::binary::to_string_like( value.v_binary);
               out << "binary  | " << FLD_BINARY     << "  |      " << string_like.size()  << " | "; out.write( string_like.data(), string_like.size()) ;out << '\n';

               out << '\n';
            }

            out << R"(

### network representation

 type   | field-id     | size value   | value      
--------|--------------|--------------|-------------
)";

            {
               out << std::fixed;
               out << "char    | " << encode( FLD_CHAR)    << "  |      " << encode( sizeof( value.net_char()))   << " | " << value.net_char()  << '\n';
               out << "short   | " << encode( FLD_SHORT)   << "  |      " << encode( sizeof( value.net_short()))  << " | " << value.net_short()  << '\n';
               out << "long    | " << encode( FLD_LONG )   << "  |      " << encode( sizeof( value.net_long()))  << " | " << value.net_long() << '\n';
               out << "float   | " << encode( FLD_FLOAT)   << "  |      " << encode( sizeof( value.net_float())) << " | " << value.net_float()  << '\n';
               out << "double  | " << encode( FLD_DOUBLE)  << "  |      " << encode( sizeof( value.net_double())) << " | " << value.net_double() << '\n';
               out << "string  | " << encode( FLD_STRING)  << "  |      " << encode( value.v_string.size())   << " | " << value.v_string << '\n';
               auto string_like = common::view::binary::to_string_like( value.v_binary);
               out << "binary  | " << encode( FLD_BINARY)  << "  |      " << encode( string_like.size())   << " | "; out.write( string_like.data(), string_like.size()); out << '\n';

               out << '\n';
            }
         }

         int main( int argc, char **argv)
         {
            auto directory = std::filesystem::path( __FILE__).parent_path().parent_path();

            std::ofstream binary( directory / "field.bin" , std::ios::binary);
            dump( binary);

            std::ofstream markdown( directory / "field.md");
            buffer::markdown( markdown);

            return 0;
         }
      }
   } // buffer

} // casual



int main( int argc, char **argv)
{
   return casual::buffer::main( argc, argv);
}

