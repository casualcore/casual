//!
//! Copyright (c) 2025, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/unittest.h"
#include "casual/xatmi.h"
#include "casual/xatmi/extended.h"
#include "casual/xatmi/explicit.h"


namespace casual
{
   namespace xatmi::extended
   {
      namespace local
      {
         namespace
         {
            auto extract_header( const char* buffer)
            {
               std::vector< std::string> result;
               ::casual_header_browse( buffer, []( const char* header, void* context) -> int
               {
                  auto state = static_cast< std::vector< std::string>*>( context);
                  state->push_back( header);
                  return 0;

               }, &result);

               return result;
            }
         } // <unnamed>
      } // local


      TEST( xatmi_header, associate)
      {
         common::unittest::Trace trace;

         const char* buffer = ::tpalloc( "X_OCTET", nullptr, 1024);
         ASSERT_TRUE( buffer != nullptr);

         std::array< const char*, 3> headers{ "a:foo", "b:bar", "c:baz" };

         EXPECT_TRUE( ::casual_header_associate( buffer, headers.data(), headers.size()) == 0);

         {
            auto headers = local::extract_header( buffer);

            EXPECT_TRUE( std::ranges::equal( headers, headers)) << CASUAL_NAMED_VALUE( headers);
         }

         ::tpfree( buffer);

         // expect the header to be disassociated
         {
            auto headers = local::extract_header( buffer);
            EXPECT_TRUE( headers.empty()) << CASUAL_NAMED_VALUE( headers);
         }
      }

      TEST( xatmi_header, associate_not_buffer_handle__expect_error)
      {
         common::unittest::Trace trace;

         const char* buffer = "not a buffer handle";         

         std::array< const char*, 3> headers{ "a:foo", "b:bar", "c:baz" };

         EXPECT_TRUE( ::casual_header_associate( buffer, headers.data(), headers.size()) == -1);
         EXPECT_TRUE( ::tperrno == TPEINVAL);
      }

      TEST( xatmi_header, associate_realloc)
      {
         common::unittest::Trace trace;

         const char* origin_buffer = ::tpalloc( "X_OCTET", nullptr, 64);
         ASSERT_TRUE( origin_buffer != nullptr);

         std::array< const char*, 3> headers{ "a:foo", "b:bar", "c:baz" };

         EXPECT_TRUE( ::casual_header_associate( origin_buffer, headers.data(), headers.size()) == 0);

         auto new_handle = ::tprealloc( origin_buffer, 1024);

         // old handle should be disassociated
         EXPECT_TRUE( local::extract_header( origin_buffer).empty());

         {
            // the header should be associated with the new handle
            auto headers = local::extract_header( new_handle);

            EXPECT_TRUE( std::ranges::equal( headers, headers)) << CASUAL_NAMED_VALUE( headers);
         }

         ::tpfree( new_handle);

         // expect the header to be disassociated
         {
            auto headers = local::extract_header( new_handle);
            EXPECT_TRUE( headers.empty()) << CASUAL_NAMED_VALUE( headers);
         }
      }
         
   } // xatmi::extended

} // casual


