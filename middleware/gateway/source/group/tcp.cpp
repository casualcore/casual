//!
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "gateway/group/tcp.h"

#include "casual/assert.h"

#include "common/communication/ipc.h"
#include "common/string/compose.h"
#include "common/posix.h"

namespace casual
{
   namespace gateway::group::tcp
   {
      using namespace common;

      Connection::~Connection()
      {
         // do an explict shutdown of the socket, to make sure that the peer gets notified about the close, and then the 
         // socket dtor will take care of the close.

         // TODO: we should probably have a specific tcp socket type that does this in its dtor.

         if( auto descriptor = m_device.connector().socket().descriptor())
            if( posix::log::result( ::shutdown( descriptor.value(), SHUT_WR), "failed to shutdown socket: ", descriptor))
               log::debug( "gateway::group::tcp::Connection shutdown descriptor: ", descriptor);
      }

      void Connection::unsent( common::communication::select::Directive& directive)
      {
         Trace trace{ "gateway::group::tcp::Connection::unsent"};

         while( ! m_unsent.empty())
         {
            if( common::communication::device::non::blocking::send( m_device, m_unsent.front()))
               m_unsent.erase( std::begin( m_unsent));
            else
               return;
         }

         directive.write_remove( descriptor());
      }
      
      common::strong::correlation::id Connection::send( common::communication::select::Directive& directive, complete_type&& complete)
      {
         Trace trace{ "gateway::group::tcp::Connection::send"};

         if( m_unsent.empty())
         {
            if( common::communication::device::non::blocking::send( m_device, complete))
               return complete.correlation();

            directive.write_add( descriptor());
         }

         // we just push it to unsent and wait for 'select' to trigger 'unsent'
         m_unsent.push_back( std::move( complete));
         return m_unsent.back().correlation();
      }

   } // gateway::group::tcp
} // casual