//!
//! casual 
//!

#ifndef CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_UNITTEST_H_
#define CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_UNITTEST_H_

#include "common/platform.h"
#include "common/log.h"
#include "common/marshal/marshal.h"
#include "common/message/type.h"

#include <gtest/gtest.h>


#include <array>

namespace casual
{
   namespace common
   {
      namespace unittest
      {
         namespace clean
         {
            //!
            //! tries to clean all signals when scope is entered and exited
            //!
            struct Scope
            {
               Scope();
               ~Scope();
            };

         } // clean


         struct Trace : clean::Scope
         {
            Trace()
            {
               auto test_info = ::testing::UnitTest::GetInstance()->current_test_info();
               log::trace << "TEST( " << test_info->test_case_name() << "." << test_info->name() << ") - in\n";
            }
            ~Trace()
            {
               auto test_info = ::testing::UnitTest::GetInstance()->current_test_info();
               log::trace << "TEST( " << test_info->test_case_name() << "." << test_info->name() << ") - out\n";
            }
         };

         namespace message
         {

            template< std::size_t size, common::message::Type type = common::message::Type::MOCKUP_BASE>
            struct basic_message : common::message::basic_message< type>
            {

               CASUAL_CONST_CORRECT_MARSHAL(
               {
                  // we don't serialize execution
                  //base_type::marshal( archive);
                  archive & payload;
               })

               std::array< char, size> payload;
            };

         } // message

         namespace random
         {
            platform::binary::type binary( std::size_t size);


            platform::binary::type::value_type byte();

            template< typename R>
            auto range( R&& range) -> decltype( std::forward< R>( range))
            {
               for( auto& value : range)
               {
                  value = byte();
               }
               return std::forward< R>( range);
            }
         } // random
      } // unittest
   } // common
} // casual

#endif // CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_UNITTEST_H_
