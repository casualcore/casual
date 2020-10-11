//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "gateway/inbound/buffer.h"

#include "common/algorithm.h"

namespace casual
{
   using namespace common;

   namespace gateway
   {
      namespace inbound
      {

         Buffer::complete_type Buffer::get( const common::Uuid& correlation)
         {
            if( auto found = algorithm::find( m_messages, correlation))
            {
               auto result = algorithm::extract( m_messages, std::begin( found));
               m_size -= result.payload.size();

               return result;
            }

            code::raise::log( code::casual::invalid_argument, "failed to find correlation: ", correlation);
         }

         Buffer::complete_type Buffer::get( const common::Uuid& correlation, platform::time::unit pending)
         {
            if( auto found = common::algorithm::find( m_calls, correlation))
            {
               auto message = algorithm::extract( m_calls, std::begin( found));
               m_size -= Buffer::size( message);
               message.pending = pending;

               return serialize::native::complete( std::move( message));
            }

            common::code::raise::log( common::code::casual::invalid_argument, "failed to find correlation: ", correlation);
         }

         std::ostream& operator << ( std::ostream& out, const Buffer& value)
         {
            return out << "{ size: " << value.m_size
               << ", messages: " << value.m_messages.size()
               << ", limit: { messages: " << value.m_limits.messages << ", size: " << value.m_limits.size << '}'
               << '}';
         }

      } // inbound
   } // gateway
} // casual