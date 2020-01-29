//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once

#include "common/unittest/log.h"

#include "casual/platform.h"
#include "common/log.h"
#include "common/serialize/macro.h"
#include "common/message/type.h"
#include "common/execute.h"


#include <array>
#include <iostream>

namespace casual
{
   namespace common
   {
      namespace unittest
      {
         using size_type = platform::size::type;

            
         struct Message : common::message::basic_message< common::message::Type::unittest_message>
         {
            Message();

            //! Will adjust the size of the paylad, and exclude the size of
            //! the 'payload size'.
            //!
            //! payload-size = size - sizeof( platform::binary::type::size_type)
            //!
            //! @param size the size of what is transported.
            Message( size_type size);

            //! @return the transport size
            //!   ( payload-size + sizeof( platform::binary::type::size_type)
            size_type size() const;

            CASUAL_CONST_CORRECT_SERIALIZE(
            {
               // we don't serialize execution
               //base_type::serialize( archive);
               CASUAL_SERIALIZE( payload);
            })

            // cores on ubuntu 16.04 when size gets over 5M.
            // guessing that the stack gets to big...
            // We switch to a vector instead.
            // std::array< char, size> payload;
            platform::binary::type payload;
         };


         namespace random
         {
            unittest::Message message( size_type size);

            platform::binary::type binary( size_type size);
            std::string string( size_type size);


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


         namespace capture
         {
            namespace standard
            {
               inline auto out( std::ostream& out)
               {
                  auto origin = std::cout.rdbuf( out.rdbuf());

                  return execute::scope( [=](){
                     std::cout.rdbuf( origin);
                  });
               }

            } // standard
         } // capture

      } // unittest
   } // common
} // casual


