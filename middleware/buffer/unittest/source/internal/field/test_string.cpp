//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/unittest.h"

#include "casual/buffer/internal/field/string.h"
#include "casual/buffer/field.h"

#include "common/algorithm.h"

#include "xatmi.h"

#include <memory>

namespace casual
{
   using namespace common;

   namespace buffer
   {
      namespace local
      {
         namespace
         {
            struct fld 
            {
               enum : long 
               {
                  string_1 = CASUAL_FIELD_STRING * CASUAL_FIELD_TYPE_BASE + 2000 + 1,
                  string_2,
                  string_3,
                  string_4,
                  string_5,
                  string_6,

                  long_1 = CASUAL_FIELD_LONG * CASUAL_FIELD_TYPE_BASE + 2000 + 1,
                  long_2,
                  long_3,
                  long_4,
                  long_5,
                  long_6,
               };
               
            };
            // represent the generated to/from string stuff
            namespace generated
            {
               namespace key_empty
               {
                  auto action = internal::field::string::Convert
                  {
                     []( internal::field::stream::Input& input, internal::field::string::stream::Output output)  // to
                     {
                        return output;
                     },
                     []( internal::field::string::stream::Input input, internal::field::stream::Output& output)  // from
                     {
                     }
                  };
                  CASUAL_MAYBE_UNUSED auto registred = internal::field::string::convert::registration( "key_empty", std::move( action)); 
                  
               } // key_empty

               namespace key_copy
               {
                  auto action = internal::field::string::Convert{
                     []( internal::field::stream::Input& input, internal::field::string::stream::Output& output)  // to string
                     {
                        auto value = input.get< local::fld::string_1>();
                        auto view = output.consume( ::strlen( value));
                        std::copy( value, value + view.size, std::begin( view));
                        //return output;
                     },
                     []( internal::field::string::stream::Input input, internal::field::stream::Output& output)  // from string
                     {
                        auto view = input.view();
                        output.add< local::fld::string_1>( view.data);
                     }
                  };
                  CASUAL_MAYBE_UNUSED auto registred = internal::field::string::convert::registration( "key_copy", std::move( action)); 
                  
               } // key_copy

               namespace key_1
               {
                  auto action = internal::field::string::Convert{
                     []( auto& input, auto& output)  // to-/from- string
                     {
                        internal::field::string::format< local::fld::string_1, internal::field::string::Alignment::left>::string( input, output, 10, ' ');
                        internal::field::string::format< local::fld::string_2, internal::field::string::Alignment::left>::string( input, output, 20, ' ');
                        internal::field::string::format< local::fld::string_3, internal::field::string::Alignment::left>::string( input, output, 30, ' ');
                     }
                  };
                  CASUAL_MAYBE_UNUSED auto registred = internal::field::string::convert::registration( "key_1", std::move( action)); 
                  
               } // key_1

               namespace key_2
               {
                  auto action = internal::field::string::Convert{
                     []( auto& input, auto& output)  // to-/from- string
                     {
                        internal::field::string::format< local::fld::string_1, internal::field::string::Alignment::right>::string( input, output, 10, '1');
                        internal::field::string::format< local::fld::string_2, internal::field::string::Alignment::right>::string( input, output, 20, '2');
                        internal::field::string::format< local::fld::string_3, internal::field::string::Alignment::right>::string( input, output, 30, '3');
                     }
                  };
                  CASUAL_MAYBE_UNUSED auto registred = internal::field::string::convert::registration( "key_2", std::move( action)); 
                  
               } // key_2

               namespace key_3
               {
                  auto action = internal::field::string::Convert{
                     []( auto& input, auto& output)  // to-/from- string
                     {
                        internal::field::string::format< local::fld::long_1, internal::field::string::Alignment::right>::string( input, output, 10, '0');
                        internal::field::string::format< local::fld::long_2, internal::field::string::Alignment::right>::string( input, output, 20, ' ');
                        internal::field::string::format< local::fld::long_3, internal::field::string::Alignment::right>::string( input, output, 30, ' ');
                     }
                  };
                  CASUAL_MAYBE_UNUSED auto registred = internal::field::string::convert::registration( "key_3", std::move( action)); 
                  
               } // key_3          
               
            } // generated

            struct Buffer
            {
               Buffer( long size) : m_data{ ::tpalloc( CASUAL_FIELD, nullptr, size)} {}
               ~Buffer() { ::tpfree( m_data);}
               
               char** get() {  return &m_data;}
               const char* get() const { return m_data;}
            private:
               char* m_data;
            };
             
         } // <unnamed>
      } // local

      TEST( buffer_internal_field_string, empty_stream_consume__expect_throw)
      {
         common::unittest::Trace trace;

         using stream_type = internal::field::string::stream::Input;

         auto stream = stream_type{ stream_type::view_type{}};

         EXPECT_THROW({
            stream.consume( 2);
         }, std::exception);
      }

      TEST( buffer_internal_field_string, missing_key__expect_throw)
      {
         common::unittest::Trace trace;

         using stream_type = internal::field::string::stream::Output;

         auto stream = stream_type{ stream_type::view_type{}};

         EXPECT_THROW({
            internal::field::string::convert::to( "non-existing-key", nullptr, stream);
         }, std::invalid_argument);
      }


      TEST( buffer_internal_field_string, to_string__key_empty__expect_nothing)
      {
         common::unittest::Trace trace;

         using stream_type = internal::field::string::stream::Output;
         auto stream = stream_type{ stream_type::view_type{}};

         EXPECT_NO_THROW({
            internal::field::string::convert::to( "key_empty", nullptr, stream);
         });
      }

      TEST( buffer_internal_field_string, to_string__key_copy__expect_copy)
      {
         common::unittest::Trace trace;

         std::string value{ "some string"};
         local::Buffer buffer{ 128};
         {
            internal::field::stream::Output output{ buffer.get()};
            output.add< local::fld::string_1>( value.c_str());
         }
         
         
         std::vector< char> data( 128, '\0');
         using stream_type = internal::field::string::stream::Output;
         auto stream = stream_type{ stream_type::view_type( data.data(), data.size())};
         
         auto view = internal::field::string::convert::to( "key_copy", *buffer.get(), stream).view();

         EXPECT_TRUE( view.size == 11) << view.size;
         EXPECT_TRUE( algorithm::equal( view, value));
      }


      TEST( buffer_internal_field_string, from_string__key_1)
      {
         common::unittest::Trace trace;

         std::string value{ "first     second              third                         "};
         
         using stream_type = internal::field::string::stream::Input;
         auto stream = stream_type{ stream_type::view_type( value.data(), value.size())};

         local::Buffer buffer{ 128};
         internal::field::string::convert::from( "key_1", stream, buffer.get());

         internal::field::stream::Input input{ *buffer.get()};
         EXPECT_TRUE( input.get< local::fld::string_1>() == std::string{ "first"});
         EXPECT_TRUE( input.get< local::fld::string_2>() == std::string{ "second"});
         EXPECT_TRUE( input.get< local::fld::string_3>() == std::string{ "third"});
      }


      TEST( buffer_internal_field_string, from_string__key_2)
      {
         common::unittest::Trace trace;

         std::string value{ "11111first22222222222222second3333333333333333333333333third"};
         
         using stream_type = internal::field::string::stream::Input;
         auto stream = stream_type{ stream_type::view_type( value.data(), value.size())};

         local::Buffer buffer{ 128};
         internal::field::string::convert::from( "key_2", stream, buffer.get());

         internal::field::stream::Input input{ *buffer.get()};
         EXPECT_TRUE( input.get< local::fld::string_1>() == std::string{ "first"});
         EXPECT_TRUE( input.get< local::fld::string_2>() == std::string{ "second"});
         EXPECT_TRUE( input.get< local::fld::string_3>() == std::string{ "third"});
      }


      TEST( buffer_internal_field_string, to_string__key_1)
      {
         common::unittest::Trace trace;

         local::Buffer buffer{ 128};
         {
            internal::field::stream::Output output{ buffer.get()};
            output.add< local::fld::string_1>( "first");
            output.add< local::fld::string_2>( "second");
            output.add< local::fld::string_3>( "third");
         }

         std::vector< char> data( 10 + 20 + 30, '\0');
         using stream_type = internal::field::string::stream::Output;
         auto stream = stream_type{ stream_type::view_type( data.data(), data.size())};

         auto view = internal::field::string::convert::to( "key_1", *buffer.get(), stream).view();
         const std::string expected{ "first     second              third                         "};

         EXPECT_TRUE( common::algorithm::equal( view, expected));
      }

      TEST( buffer_internal_field_string, to_string__key_2)
      {
         common::unittest::Trace trace;

         local::Buffer buffer{ 128};
         {
            internal::field::stream::Output output{ buffer.get()};
            output.add< local::fld::string_1>( "first");
            output.add< local::fld::string_2>( "second");
            output.add< local::fld::string_3>( "third");
         }

         std::vector< char> data( 10 + 20 + 30, '\0');
         using stream_type = internal::field::string::stream::Output;
         auto stream = stream_type{ stream_type::view_type( data.data(), data.size())};

         auto view = internal::field::string::convert::to( "key_2", *buffer.get(), stream).view();
         const std::string expected{ "11111first22222222222222second3333333333333333333333333third"};

         EXPECT_TRUE( common::algorithm::equal( view, expected));
      }

      
      TEST( buffer_internal_field_string, to_string__key_3)
      {
         common::unittest::Trace trace;

         local::Buffer buffer{ 128};
         {
            internal::field::stream::Output output{ buffer.get()};
            output.add< local::fld::long_1>( 100);
            output.add< local::fld::long_2>( 200);
            output.add< local::fld::long_3>( 300);
         }

         std::vector< char> data( 10 + 20 + 30 + 1, '\0');
         using stream_type = internal::field::string::stream::Output;
         auto stream = stream_type{ stream_type::view_type( data.data(), data.size())};

         auto view = internal::field::string::convert::to( "key_3", *buffer.get(), stream).view();
         const std::string expected{ "0000000100                 200                           300"};

         EXPECT_TRUE( common::algorithm::equal( view, expected)) << "view: " << data.data();
      }
      
   } // buffer

} // casual