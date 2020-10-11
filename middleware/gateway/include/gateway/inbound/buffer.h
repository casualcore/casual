//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "casual/platform.h"
#include "common/communication/message.h"
#include "common/message/service.h"
#include "common/serialize/native/complete.h"

#include <vector>

namespace casual
{
   namespace gateway
   {
      namespace inbound
      {
         namespace buffer 
         {
            using size_type = platform::size::type;
            struct Limit
            {
               Limit() = default;
               inline Limit( size_type size, size_type messages)
                  : size( size), messages( messages) {}

               size_type size = 0;
               size_type messages = 0;
            };
         } // buffer 
         struct Buffer 
         {
            using complete_type = common::communication::message::Complete;
            using size_type = buffer::size_type;

            inline Buffer( buffer::Limit limits) : m_limits( limits) {}

            inline buffer::Limit limit() const { return m_limits;}
            inline void limit( buffer::Limit limit) { m_limits = limit;}

            inline bool congested() const
            {
               if( m_limits.size > 0 && m_size > m_limits.size)
                  return true;

               return m_limits.messages > 0 && static_cast< size_type>( m_calls.size() + m_messages.size()) > m_limits.messages;
            }
            
            template< typename M>
            inline auto add( M&& message)
            {
               m_messages.push_back( common::serialize::native::complete( std::forward< M>( message)));
               m_size += m_messages.back().size();
            }

            inline void add( common::message::service::call::callee::Request&& message)
            {
               m_size += Buffer::size( message);
               m_calls.push_back( std::move( message));
            }

            //! consumes pending calls, and sets the 'pending-roundtrip-state'
            complete_type get( const common::Uuid& correlation, platform::time::unit pending);

            complete_type get( const common::Uuid& correlation);

            friend std::ostream& operator << ( std::ostream& out, const Buffer& value);

         private:

            inline static platform::size::type size( const common::message::service::call::callee::Request& message)
            {
               return sizeof( message) + message.buffer.memory.size() + message.buffer.type.size();
            }

            std::vector< common::message::service::call::callee::Request> m_calls;
            std::vector< complete_type> m_messages;
            buffer::size_type m_size = 0;
            buffer::Limit m_limits;
            
         };
      } // inbound
   } // gateway
} // casual