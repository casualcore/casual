//! 
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once


#include "common/message/type.h"
#include "common/message/pending.h"

#include "common/communication/message.h"
#include "common/marshal/complete.h"

namespace casual
{
   namespace eventually
   {
      namespace send
      {
         namespace detail
         {
            struct Request : common::message::basic_message< common::message::Type::eventually_send_message>
            {
               Request( common::message::pending::Message&& message)
                  : message{ std::move( message)} {}
               
               Request() = default;

               common::message::pending::Message message;

               CASUAL_CONST_CORRECT_MARSHAL(
               {
                  archive & message;
               })
               friend std::ostream& operator << ( std::ostream& out, const Request& rhs);
            };

            

            //! if `casual-eventually-send` is not available this will throw
            void message( const common::message::pending::Message& message);

            template< typename M>
            void message( const common::process::Handle& destination, M&& message)
            {
               detail::message( common::message::pending::Message{ std::forward< M>( message), destination});
            }

         } // detail

         void message( const common::message::pending::Message& message);

         template< typename M>
         void message( const common::process::Handle& destination, M&& message)
         {
            send::message( common::message::pending::Message{ std::forward< M>( message), destination});
         }
    
      } // send
      
   } // eventually
} // casual