//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/unittest.h"

#include "common/serialize/line.h"
#include "common/serialize/macro.h"
#include "common/serialize/create.h"

#include "common/code/xa.h"

#include <filesystem>

namespace casual
{
   namespace common
   {
      namespace serialize
      {
         namespace local
         {
            namespace
            {
               struct A 
               {
                  long long_value{};
                  short short_value{};

                  CASUAL_LOG_SERIALIZE(
                     CASUAL_SERIALIZE( long_value);
                     CASUAL_SERIALIZE( short_value);
                  )
               };

               struct B
               {
                  long long_value{};
                  short short_value{};

                  CASUAL_LOG_SERIALIZE(
                     CASUAL_SERIALIZE( long_value);
                     CASUAL_SERIALIZE( short_value);
                  )

                  friend std::ostream& operator << ( std::ostream& out, const B& value) { return out << "overridden";}
               };

               struct C
               {
                  long id;
                  std::filesystem::path path;

                  CASUAL_LOG_SERIALIZE(
                     CASUAL_SERIALIZE( id);
                     CASUAL_SERIALIZE( path);
                  )
               };

               struct D
               {
                  code::xa code;

                  CASUAL_LOG_SERIALIZE(
                     CASUAL_SERIALIZE( code);
                  )
               };

            } // <unnamed>
         } // local

         TEST( casual_serialize_line, serialize_composite)
         {
            common::unittest::Trace trace;

            local::A value{ 42, 2};

            std::ostringstream out;
            stream::write( out, CASUAL_NAMED_VALUE( value));

            EXPECT_TRUE( out.str() == "value: { long_value: 42, short_value: 2}") << "out.str(): " << out.str();
         }

         TEST( casual_serialize_line, explicit_serialize_composite)
         {
            common::unittest::Trace trace;

            local::A value{ 42, 2};

            auto writer = line::Writer{};
            writer << CASUAL_NAMED_VALUE( value);

            auto string = writer.consume();

            EXPECT_TRUE( string == "value: { long_value: 42, short_value: 2}") << "string: " << string;
         }

         TEST( casual_serialize_line, serialize_overridden_ostream_stream_operator)
         {
            common::unittest::Trace trace;

            local::B value{ 42, 2};

            std::ostringstream out;
            stream::write( out, CASUAL_NAMED_VALUE( value));

            EXPECT_TRUE( out.str() == "value: overridden") << CASUAL_NAMED_VALUE( value);
         }

         TEST( casual_serialize_line, create_writer)
         {
            common::unittest::Trace trace;

            local::A value{ 42, 2};

            auto archive = line::writer();            
            archive << CASUAL_NAMED_VALUE( value);

            auto result = archive.consume< std::string>();

            EXPECT_TRUE( result == R"(value: { long_value: 42, short_value: 2})" ) << CASUAL_NAMED_VALUE( result);
         }


         TEST( casual_serialize_line, create_writer_from)
         {
            common::unittest::Trace trace;

            local::A value{ 42, 2};

            auto archive = serialize::create::writer::from( "line");
            
            archive << CASUAL_NAMED_VALUE( value);

            EXPECT_TRUE( archive.consume< std::string>() == R"(value: { long_value: 42, short_value: 2})" ) << CASUAL_NAMED_VALUE( value);
         }

         TEST( casual_serialize_line, write_tuple)
         {
            common::unittest::Trace trace;

            auto tuple = std::make_tuple( 1, std::string{"foo"}, true);

            auto archive = serialize::create::writer::from( "line");
            
            archive << CASUAL_NAMED_VALUE( tuple);

            EXPECT_TRUE( archive.consume< std::string>() == R"(tuple: [ 1, foo, true])") << CASUAL_NAMED_VALUE( tuple);
         }

         TEST( casual_serialize_line, write_composit_with_path)
         {
            common::unittest::Trace trace;

            local::C value{ 42, "/a/b/c/d.txt"};

            std::ostringstream out;
            stream::write( out, CASUAL_NAMED_VALUE( value));
         

            EXPECT_TRUE( out.str() == R"(value: { id: 42, path: "/a/b/c/d.txt"})") << "out: " << out.str();
         }

         TEST( casual_serialize_line, write_composit_with_code)
         {
            common::unittest::Trace trace;

            local::D value{ code::xa::duplicate_xid};

            std::ostringstream out;
            stream::write( out, "value: ", value);
         
            EXPECT_TRUE( out.str() == R"(value: { code: [xa:XAER_DUPID]})") << "out: " << out.str();
            
         }


         
      } //serialize
   } // common
} // casual