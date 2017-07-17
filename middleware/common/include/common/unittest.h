//!
//! casual 
//!

#ifndef CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_UNITTEST_H_
#define CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_UNITTEST_H_

#include "common/platform.h"
#include "common/log.h"
#include "common/marshal/marshal.h"
#include "common/message/type.h"
#include "common/communication/ipc.h"

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
               log::debug << "TEST( " << test_info->test_case_name() << "." << test_info->name() << ") - in\n";
            }
            ~Trace()
            {
               auto test_info = ::testing::UnitTest::GetInstance()->current_test_info();
               log::debug << "TEST( " << test_info->test_case_name() << "." << test_info->name() << ") - out\n";
            }
         };

            
         struct Message : common::message::basic_message< common::message::Type::MOCKUP_BASE>
         {
            Message();

            //!
            //! Will adjust the size of the paylad, and exclude the size of
            //! the 'payload size'.
            //!
            //! payload-size = size - sizeof( platform::binary::type::size_type)
            //!
            //! @param size the size of what is transported.
            //!
            Message( std::size_t size);


            //!
            //! @return the transport size
            //!   ( payload-size + sizeof( platform::binary::type::size_type)
            //!
            std::size_t size() const;

            CASUAL_CONST_CORRECT_MARSHAL(
            {
               // we don't serialize execution
               //base_type::marshal( archive);
               archive & payload;
            })

            //
            // cores on ubuntu 16.04 when size gets over 5M.
            // guessing that the stack gets to big...
            // We switch to a vector instead.
            // std::array< char, size> payload;
            platform::binary::type payload;
         };


         namespace random
         {
            unittest::Message message( std::size_t size);

            platform::binary::type binary( std::size_t size);


            platform::binary::type::value_type byte();

            template< typename R>
            decltype( auto) range( R&& range)
            {
               for( auto& value : range)
               {
                  value = byte();
               }
               return std::forward< R>( range);
            }
         } // random

         namespace domain
         {
            namespace manager
            {
               //!
               //! Waits for the domain manager to boot
               //!
               void wait( communication::ipc::inbound::Device& device);

            } // manager

         } // domain

      } // unittest
   } // common
} // casual

#endif // CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_UNITTEST_H_
