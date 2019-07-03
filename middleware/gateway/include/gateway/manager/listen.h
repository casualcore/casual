//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "gateway/message.h"

#include "common/communication/tcp.h"
#include "common/communication/select.h"

namespace casual
{
   namespace gateway
   {
      namespace manager
      {
         namespace listen
         {
            using Limit = message::inbound::Limit;

            struct Connection
            {
               common::communication::Socket socket;
               Limit limit;
            };

            struct Entry
            {
               Entry( common::communication::tcp::Address address);
               Entry( common::communication::tcp::Address address, Limit limit);

               inline const common::communication::tcp::Address& address() const { return m_address;}
               inline Limit limit() const { return m_limit;}

               inline auto descriptor() const { return m_socket.descriptor();}

               Connection accept() const;

               
               CASUAL_CONST_CORRECT_SERIALIZE_WRITE(
               { 
                  CASUAL_SERIALIZE_NAME( m_address, "address");
                  CASUAL_SERIALIZE_NAME( m_socket, "socket");
                  CASUAL_SERIALIZE_NAME( m_limit, "limit");
               })

            private:
               common::communication::tcp::Address m_address;
               common::communication::Socket m_socket;
               Limit m_limit;
            };

            struct Dispatch
            {


            };


         } // listen

      } // manager

   } // gateway
} // casual