//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!



#include "common/unittest.h"
#include "common/unittest/random/archive.h"


#include "common/serialize/macro.h"
#include "common/serialize/json.h"
#include "common/serialize/yaml.h"
#include "common/serialize/xml.h"
#include "common/serialize/ini.h"
#include "common/serialize/line.h"
#include "common/serialize/native/binary.h"
#include "common/serialize/archive/consume.h"


#include "common/log.h"
#include "common/code/casual.h"
#include "common/code/xatmi.h"

#include "../../include/test_vo.h"


#include <string>
#include <random>
#include <set>
#include <vector>
#include <deque>
#include <fstream>


namespace casual
{
   namespace common
   {
      namespace holder
      {
         template< typename P>
         struct basic
         {
            using policy_type = P;

            template< typename T>
            static auto string_document( const T& value) 
            {
               auto writer = policy_type::writer();
               writer << CASUAL_NAMED_VALUE( value);

               std::stringstream out;
               serialize::writer::consume( writer, out);
               return std::move( out).str();
            }

            template< typename T>
            static auto validate( T& reader, traits::priority::tag< 1>) -> decltype( reader.validate())
            {
               reader.validate();
            }

            template< typename T>
            static auto validate( T& reader, traits::priority::tag< 0>)
            {
               // no-op
            }

            template< typename T>
            static T write_read( const T& origin)
            {
               auto writer = policy_type::writer();
               writer << CASUAL_NAMED_VALUE_NAME( origin, "value");

               {
                  auto buffer = policy_type::buffer();
                  serialize::writer::consume( writer, buffer);

                  auto reader = policy_type::reader( buffer);
                  T value;
                  reader >> CASUAL_NAMED_VALUE( value);
                  validate( reader, traits::priority::tag< 1>{});
                  return value;
               }
            }

            template< typename T>
            static T unnamed_write_read( const T& value)
            {
               auto writer = policy_type::writer();
               writer << value;

               {
                  auto buffer = policy_type::buffer();
                  serialize::writer::consume( writer, buffer);

                  //if( serialize::traits::need::named< std::decay_t< decltype( writer)>>::value)
                  //   log_buffer( buffer);

                  auto reader = policy_type::reader( buffer);
                  T value;
                  reader >> value;
                  validate( reader, traits::priority::tag< 1>{});
                  return value;
               }
            }
         };

         namespace policy
         {
            template< typename B>
            struct base
            {
               static B buffer() { return B{};}
            };

            template< typename B>
            struct json : base< B>
            {
               template< typename T>
               static auto reader( T&& buffer) { return serialize::json::strict::reader( buffer);}

               static auto writer() { return serialize::json::pretty::writer();}
            };

            namespace relaxed    
            {
               template< typename B>
               struct json : policy::json< B>
               {
                  template< typename T>
                  static auto reader( T&& buffer) { return serialize::json::relaxed::reader( buffer);}
               };
            } // relaxed

            namespace consumed
            {
               template< typename B>
               struct json : policy::json< B>
               {
                  template< typename T>
                  static auto reader( T&& buffer) { return serialize::json::consumed::reader( buffer);}
               };
            } // relaxed  

            template< typename B>
            struct yaml : base< B>
            {
               template< typename T>
               static auto reader( T&& buffer) { return serialize::yaml::strict::reader( buffer);}

               static auto writer() { return serialize::yaml::writer();}
            };

            namespace relaxed    
            {
               template< typename B>
               struct yaml : policy::yaml< B>
               {
                  template< typename T>
                  static auto reader( T&& buffer) { return serialize::yaml::relaxed::reader( buffer);}
               };
            } // relaxed

            namespace consumed
            {
               template< typename B>
               struct yaml : policy::yaml< B>
               {
                  template< typename T>
                  static auto reader( T&& buffer) { return serialize::yaml::consumed::reader( buffer);}
               };
            } // relaxed  

            template< typename B>
            struct xml : base< B>
            {
               template< typename T>
               static auto reader( T&& buffer) { return serialize::xml::strict::reader( buffer);}

               static auto writer() { return serialize::xml::writer();}
            };

            namespace relaxed    
            {
               template< typename B>
               struct xml : policy::xml< B>
               {
                  template< typename T>
                  static auto reader( T&& buffer) { return serialize::xml::relaxed::reader( buffer);}
               };
            } // relaxed  

            struct binary : base< platform::binary::type>
            {
               template< typename T>
               static auto reader( T&& buffer) { return serialize::native::binary::reader( buffer);}

               static auto writer() { return serialize::native::binary::writer();}
            };

         } // policy
      } // holder


      template <typename H>
      struct common_serialize_write_read : public ::testing::Test, public H
      {
      };

      using archive_types = ::testing::Types<
            holder::basic< holder::policy::json< std::string>>,
            holder::basic< holder::policy::json< platform::binary::type>>,
            holder::basic< holder::policy::json< std::stringstream>>,
            holder::basic< holder::policy::relaxed::json< std::string>>,
            holder::basic< holder::policy::relaxed::json< platform::binary::type>>,
            holder::basic< holder::policy::relaxed::json< std::stringstream>>,
            holder::basic< holder::policy::consumed::json< std::string>>,
            holder::basic< holder::policy::yaml< std::string>>,
            holder::basic< holder::policy::yaml< platform::binary::type>>,
            holder::basic< holder::policy::yaml< std::stringstream>>,
            holder::basic< holder::policy::relaxed::yaml< std::string>>,
            holder::basic< holder::policy::relaxed::yaml< platform::binary::type>>,
            holder::basic< holder::policy::relaxed::yaml< std::stringstream>>,
            holder::basic< holder::policy::consumed::yaml< std::string>>,
            holder::basic< holder::policy::xml< std::string>>,
            holder::basic< holder::policy::xml< platform::binary::type>>,
            holder::basic< holder::policy::xml< std::stringstream>>,
            holder::basic< holder::policy::relaxed::xml< std::string>>,
            holder::basic< holder::policy::relaxed::xml< platform::binary::type>>,
            holder::basic< holder::policy::relaxed::xml< std::stringstream>>,
            //holder::ini< archive::policy::Strict>,  // cannot handle nested containers yet
            //holder::ini< archive::policy::Relaxed>, // cannot handle nested containers yet
            holder::basic< holder::policy::binary>
       >;

      TYPED_TEST_SUITE( common_serialize_write_read, archive_types);


      template< typename F, typename T>
      void test_value_min_max( T value)
      {
         EXPECT_TRUE( F::write_read( value) == value);
         {
            //std::cerr << std::fixed << "max: " << std::numeric_limits< T>::max() << '\n';
            T value = std::numeric_limits< T>::max(); // - 1;
            EXPECT_TRUE( F::write_read( value) == value);
         }
         {
            T value = std::numeric_limits< T>::min(); // + 1;
            EXPECT_TRUE( F::write_read( value) == value);
         }
      }

      TYPED_TEST( common_serialize_write_read, type_bool)
      {
         unittest::Trace trace;

         bool value = true;
         //test_value_min_max< TestFixture>( value);
         EXPECT_TRUE( TestFixture::write_read( value) == true) << TestFixture::write_read( value);
      }


      TYPED_TEST( common_serialize_write_read, type_char)
      {
         unittest::Trace trace;

         char value = 'A';
         //test_value_min_max< TestFixture>( value);
         EXPECT_TRUE( TestFixture::write_read( value) == 'A') << TestFixture::write_read( value);
      }

      TYPED_TEST( common_serialize_write_read, type_short)
      {
         unittest::Trace trace;

         short value = 42;
         test_value_min_max< TestFixture>( value);
      }

      TYPED_TEST( common_serialize_write_read, type_int)
      {
         unittest::Trace trace;

         int value = 42;
         test_value_min_max< TestFixture>( value);
      }

      TYPED_TEST( common_serialize_write_read, type_long)
      {
         unittest::Trace trace;

         long value = 42;
         test_value_min_max< TestFixture>( value);
      }

      TYPED_TEST( common_serialize_write_read, type_string)
      {
         unittest::Trace trace;

         std::string value = "value 42";
         auto result = TestFixture::write_read( value);
         EXPECT_TRUE( result == "value 42") << "result: " << result;
      }

      TYPED_TEST( common_serialize_write_read, type_string_with_new_line)
      {
         unittest::Trace trace;

         std::string value = "first\nother";
         EXPECT_TRUE( TestFixture::write_read( value) == "first\nother");
      }

      TYPED_TEST( common_serialize_write_read, type_string__complex)
      {
         unittest::Trace trace;

         // -6tL~|O,-<f;>#
         // Lr#jG<5S0#4:\>^y{{Njw00]9x'05*qJ
         // "{}#[]"

         std::string value{ "?HM'wz'o"};

         EXPECT_TRUE( TestFixture::write_read( value) == value) << "value: '" << value; 
      }

      TYPED_TEST( common_serialize_write_read, type_char__ws)
      {
         unittest::Trace trace;

         const char value = ' ';
         EXPECT_NO_THROW( 
            EXPECT_TRUE( TestFixture::write_read( value) == value) << "value: '" << value; 
         ) << "document:\n" << TestFixture::string_document( value);
      }


      TYPED_TEST( common_serialize_write_read, type_string_all_printable_ascii)
      {
         unittest::Trace trace;

         std::string string{ " "};

         for( int value = 32; value < 127; value++)
         {
            string[ 0] = static_cast< char>( value);
            auto result = TestFixture::write_read( string);
            EXPECT_TRUE( result == string) << "value: " << value << ", string: '" << string << "', result: '" << result << "'";
         }
            
      }

      // TODO: gives warning from clang and gives failure on OSX with locale "UTF-8"
      TYPED_TEST( common_serialize_write_read, DISABLED_type_extended_string)
      {
         unittest::Trace trace;

         std::string value = u8"Bängen Trålar";
         EXPECT_TRUE( TestFixture::write_read( value) == u8"Bängen Trålar");
      }

      TYPED_TEST( common_serialize_write_read, type_double)
      {
         unittest::Trace trace;

         double value = 42.42;
         //test_value_min_max< TestFixture>( value);
         EXPECT_TRUE( TestFixture::write_read( value) == value) << TestFixture::write_read( value);
      }


      TYPED_TEST( common_serialize_write_read, type_binary)
      {
         unittest::Trace trace;

         auto value = unittest::random::create< platform::binary::type>();

         EXPECT_TRUE( ! value.empty());
         EXPECT_TRUE( TestFixture::write_read( value) == value);
      }
/*
      TYPED_TEST( common_serialize_write_read, type_array_long)
      {
         unittest::Trace trace;

         auto value = unittest::random::create< std::array< long, 10>>();

         EXPECT_TRUE( ! value.empty());
         EXPECT_TRUE( TestFixture::write_read( value) == value);
      }
*/

      TYPED_TEST( common_serialize_write_read, type_vector_long)
      {
         unittest::Trace trace;

         auto value = unittest::random::create< std::vector< long>>();
         EXPECT_TRUE( ! value.empty());
         EXPECT_TRUE( TestFixture::write_read( value) == value);
      }

      TYPED_TEST( common_serialize_write_read, type_list_long)
      {
         unittest::Trace trace;

         auto value = unittest::random::create< std::list< long>>();
         EXPECT_TRUE( ! value.empty());
         EXPECT_TRUE( TestFixture::write_read( value) == value);
      }

      TYPED_TEST( common_serialize_write_read, type_deque_long)
      {
         unittest::Trace trace;

         auto value = unittest::random::create< std::deque< long>>();
         EXPECT_TRUE( ! value.empty());
         EXPECT_TRUE( TestFixture::write_read( value) == value);
      }

      TYPED_TEST( common_serialize_write_read, type_set_long)
      {
         unittest::Trace trace;

         auto value = unittest::random::create< std::set< long>>();
         EXPECT_TRUE( ! value.empty());
         EXPECT_TRUE( TestFixture::write_read( value) == value);
      }

      TYPED_TEST( common_serialize_write_read, type_map_long_string)
      {
         unittest::Trace trace;

         auto value = unittest::random::create< std::map< long, std::string>>();
         EXPECT_TRUE( ! value.empty());
         EXPECT_TRUE( TestFixture::write_read( value) == value);
      }

      TYPED_TEST( common_serialize_write_read, type_vector_vector_long)
      {
         unittest::Trace trace;

         auto value = unittest::random::create< std::vector< std::vector< long>>>();
         EXPECT_TRUE( ! value.empty());
         EXPECT_TRUE( TestFixture::write_read( value) == value);
      }

      TYPED_TEST( common_serialize_write_read, type_tuple)
      {
         unittest::Trace trace;
         
         // TODO maintainence - float, double does not work for all formats due to lack of precision, 
         //auto value = unittest::random::create< std::tuple< int, std::string, char, long, float, double>>();

         auto value = unittest::random::create< std::tuple< int, std::string, char, long, platform::binary::type, short>>();
         
         auto result = TestFixture::write_read( value);
         EXPECT_TRUE( result == value) << " " << CASUAL_NAMED_VALUE( value) << '\n' 
            << CASUAL_NAMED_VALUE( result) << "\n" 
            << "document: " << TestFixture::string_document( value);
      }

      namespace local
      {
         namespace
         {
            struct Foo : compare::Equality< Foo>
            {
               std::string a;
               long b = 0;
               bool c = false;
               std::tuple< int, std::string> d;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE( a);
                  CASUAL_SERIALIZE( b);
                  CASUAL_SERIALIZE( c);
                  CASUAL_SERIALIZE( d);
               )

               auto tie() const 
               {
                  return std::tie( a, b, c, d);
               }
            };
         } // <unnamed>
      } // local

      TYPED_TEST( common_serialize_write_read, unnamed_type_composit)
      {
         unittest::Trace trace;

         auto value = unittest::random::create< local::Foo>();

         EXPECT_NO_THROW(
         {
            EXPECT_TRUE( TestFixture::unnamed_write_read( value) == value);
         }) << CASUAL_NAMED_VALUE( value);
      }

      TYPED_TEST( common_serialize_write_read, unnamed_type_vector_long)
      {
         unittest::Trace trace;

         auto value = unittest::random::create< std::vector< long>>();

         EXPECT_TRUE( TestFixture::unnamed_write_read( value) == value);
      }

      TYPED_TEST( common_serialize_write_read, unnamed_type_tuple)
      {
         unittest::Trace trace;

         auto value = unittest::random::create< std::tuple< int, std::string, char, long, platform::binary::type, short>>();

         EXPECT_NO_THROW(
            EXPECT_TRUE( TestFixture::unnamed_write_read( value) == value);
         ) << CASUAL_NAMED_VALUE( value);
      }

      TYPED_TEST( common_serialize_write_read, unnamed_type_vector_composit)
      {
         unittest::Trace trace;

         auto value = unittest::random::create< std::vector< local::Foo>>();

         EXPECT_NO_THROW(
         {
            EXPECT_TRUE( TestFixture::unnamed_write_read( value) == value);
         }) << value;
         

      }

      TYPED_TEST( common_serialize_write_read, type_code_casual)
      {
         std::error_code value = code::casual::invalid_path;
         EXPECT_TRUE( TestFixture::write_read( value) == code::casual::invalid_path);
      }

      TYPED_TEST( common_serialize_write_read, type_code_xatmi)
      {
         std::error_code value = code::xatmi::protocol;
         EXPECT_TRUE( TestFixture::write_read( value) == code::xatmi::protocol);
      }

      TYPED_TEST( common_serialize_write_read, type_code_errc)
      {
         auto value = std::make_error_code( std::errc::no_such_process);
         EXPECT_TRUE( TestFixture::write_read( value) == std::errc::no_such_process);
      }

   } // common
} // casual

