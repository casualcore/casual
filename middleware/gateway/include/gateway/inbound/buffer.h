//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "casual/platform.h"
#include "common/communication/message.h"

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
               return ( m_limits.size > 0 && m_size < m_limits.size)
                  || ( m_limits.messages > 0 && static_cast< size_type>( m_messages.size()) < m_limits.messages);
            }

            inline void add( complete_type&& message) 
            {
               m_messages.push_back( std::move( message));
               m_size += m_messages.back().size();
            }

            complete_type get( const common::Uuid& correlation);

            friend std::ostream& operator << ( std::ostream& out, const Buffer& value);

         private:
            std::vector< complete_type> m_messages;
            buffer::size_type m_size = 0;
            buffer::Limit m_limits;
            
         };
      } // inbound
   } // gateway
} // casual