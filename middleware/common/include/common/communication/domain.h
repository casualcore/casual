//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once


#include "common/communication/socket.h"
#include "common/communication/message.h"

#include "common/uuid.h"
#include "common/flag.h"
#include "common/string.h"
#include "common/environment.h"

namespace casual
{
   namespace common
   {
      namespace communication
      {
         namespace domain
         {
            struct Address
            {
               Address( Uuid key) : m_key( std::move( key)) {}
               Address() : Address{ uuid::make()} {}
               
               std::string name() const 
               { 
                  return string::compose( environment::directory::temporary(), '/', m_key, ".socket");
               }

            private:
               Uuid m_key;
            };

            namespace native
            {
               Socket create();

               void bind( const Socket& socket, const Address& address);

               enum class Flag : long
               {
                  non_blocking = platform::flag::value( platform::flag::tcp::no_wait)
               };

               void listen( const Socket& socket);
               Socket accept( const Socket& socket);

               Uuid send( const Socket& socket, const communication::message::Complete& complete, common::Flags< Flag> flags);
               communication::message::Complete receive( const Socket& socket, common::Flags< Flag> flags);

               

            } // native
            void connect( const Socket& socket, const Address& address);
   



         } // domain
      
      } // communication
   } // common
} // casual
