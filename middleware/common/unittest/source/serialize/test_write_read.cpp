//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!



#include "common/unittest.h"


#include "common/serialize/macro.h"
#include "common/serialize/json.h"
#include "common/serialize/yaml.h"
#include "common/serialize/xml.h"
#include "common/serialize/ini.h"
#include "common/serialize/line.h"
#include "common/serialize/native/binary.h"
#include "common/serialize/archive/consume.h"

#include "common/log.h"

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
            static T write_read( const T& value)
            {
               auto writer = policy_type::writer();
               writer << CASUAL_NAMED_VALUE( value);

               {
                  auto buffer = policy_type::buffer();
                  serialize::writer::consume( writer, buffer);

                  auto reader = policy_type::reader( buffer);
                  T value;
                  reader >> CASUAL_NAMED_VALUE( value);
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
            holder::basic< holder::policy::yaml< std::string>>,
            holder::basic< holder::policy::yaml< platform::binary::type>>,
            holder::basic< holder::policy::yaml< std::stringstream>>,
            holder::basic< holder::policy::relaxed::yaml< std::string>>,
            holder::basic< holder::policy::relaxed::yaml< platform::binary::type>>,
            holder::basic< holder::policy::relaxed::yaml< std::stringstream>>,
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

      TYPED_TEST_CASE( common_serialize_write_read, archive_types);


      template< typename F, typename T>
      void test_value_min_max( T value)
      {
         EXPECT_TRUE( F::write_read( value) == value);
         {
            //std::cerr << std::fixed << "max: " << std::numeric_limits< T>::max() << '\n';
            T value = std::numeric_limits< T>::max() - 1;
            EXPECT_TRUE( F::write_read( value) == value);
         }
         {
            T value = std::numeric_limits< T>::min() + 1;
            EXPECT_TRUE( F::write_read( value) == value);
         }
      }

      TYPED_TEST( common_serialize_write_read, type_bool)
      {
         bool value = true;
         //test_value_min_max< TestFixture>( value);
         EXPECT_TRUE( TestFixture::write_read( value) == true) << TestFixture::write_read( value);
      }


      TYPED_TEST( common_serialize_write_read, type_char)
      {
         char value = 'A';
         //test_value_min_max< TestFixture>( value);
         EXPECT_TRUE( TestFixture::write_read( value) == 'A') << TestFixture::write_read( value);
      }

      TYPED_TEST( common_serialize_write_read, type_short)
      {
         short value = 42;
         test_value_min_max< TestFixture>( value);
      }

      TYPED_TEST( common_serialize_write_read, type_int)
      {
         int value = 42;
         test_value_min_max< TestFixture>( value);
      }

      TYPED_TEST( common_serialize_write_read, type_long)
      {
         long value = 42;
         test_value_min_max< TestFixture>( value);
      }

      TYPED_TEST( common_serialize_write_read, type_string)
      {
         std::string value = "value 42";
         auto result = TestFixture::write_read( value);
         EXPECT_TRUE( result == "value 42") << "result: " << result;
      }

      TYPED_TEST( common_serialize_write_read, type_string_with_new_line)
      {
         std::string value = "first\nother";
         EXPECT_TRUE( TestFixture::write_read( value) == "first\nother");
      }

      // TODO: gives warning from clang and gives failure on OSX with locale "UTF-8"
      TYPED_TEST( common_serialize_write_read, DISABLED_type_extended_string)
      {
         std::string value = u8"Bängen Trålar";
         EXPECT_TRUE( TestFixture::write_read( value) == u8"Bängen Trålar");
      }

      TYPED_TEST( common_serialize_write_read, type_double)
      {
         double value = 42.42;
         //test_value_min_max< TestFixture>( value);
         EXPECT_TRUE( TestFixture::write_read( value) == value) << TestFixture::write_read( value);
      }


      TYPED_TEST( common_serialize_write_read, type_binary)
      {
         platform::binary::type value{ 0, 42, -123, 23, 43, 11, 124};
         EXPECT_TRUE( TestFixture::write_read( value) == value);
      }

      TYPED_TEST( common_serialize_write_read, type_vector_long)
      {
         std::vector< long> value{ 234, 34234, 3242, 4564, 6456, 546, 3453, 78678, 35345};
         EXPECT_TRUE( TestFixture::write_read( value) == value);
      }

      TYPED_TEST( common_serialize_write_read, type_list_long)
      {
         std::list< long> value{ 234, 34234, 3242, 4564, 6456, 546, 3453, 78678, 35345};
         EXPECT_TRUE( TestFixture::write_read( value) == value);
      }

      TYPED_TEST( common_serialize_write_read, type_deque_long)
      {
         std::deque< long> value{ 234, 34234, 3242, 4564, 6456, 546, 3453, 78678, 35345};
         EXPECT_TRUE( TestFixture::write_read( value) == value);
      }

      TYPED_TEST( common_serialize_write_read, type_set_long)
      {
         std::set< long> value{ 234, 34234, 3242, 4564, 6456, 546, 3453, 78678, 35345};
         EXPECT_TRUE( TestFixture::write_read( value) == value);
      }

      TYPED_TEST( common_serialize_write_read, type_map_long_string)
      {
         std::map< long, std::string> value{ { 234, "poo"}, { 34234, "sdkfljs"}, { 3242, "cmx,nvxnvjkjdf"}};
         EXPECT_TRUE( TestFixture::write_read( value) == value);
      }

      TYPED_TEST( common_serialize_write_read, type_vector_vector_long)
      {
         std::mt19937 random{ std::random_device{}()};
         std::vector< std::vector< long>> value( 10);
         for( auto& values : value)
         {
            values = { 234, 34234, 3242, 4564, 6456, 546, 3453, 78678, 35345};
            std::shuffle( std::begin( values), std::end( values), random);
         }
         EXPECT_TRUE( TestFixture::write_read( value) == value);
      }

      TYPED_TEST( common_serialize_write_read, type_tuple)
      {
         std::tuple< int, std::string, char, long, float> value{ 23, "charlie", 'Q', 343534323434, 1.42};
         auto result = TestFixture::write_read( value);
         EXPECT_TRUE( result == value) << CASUAL_NAMED_VALUE( value) << CASUAL_NAMED_VALUE( result);
      }

      TYPED_TEST( common_serialize_write_read, type_vector_tuple)
      {
         using tuple_type = std::tuple< int, std::string, char, long, float>;
         tuple_type tuple{ 23, "charlie", 'Q', 343534323434, 1.42};

         std::vector< tuple_type> value{
            tuple, tuple, tuple, tuple
         };
         EXPECT_TRUE( TestFixture::write_read( value) == value);
      }

   } // common
} // casual

