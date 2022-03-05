//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/communication/ipc/message.h"

namespace casual
{
   namespace common::communication::ipc::message
   {
      Complete::Complete( message_type_type type, const correlation_type& correlation) : m_type{ type}, m_correlation{ correlation} {}

      Complete::Complete( common::message::Type type, const correlation_type& correlation, payload_type&& payload)
         : payload{ std::move( payload)}, m_type{ type}, m_correlation{ correlation} {}


      Complete::operator bool() const
      {
         return type() != message_type_type::absent_message;
      }

      bool Complete::complete() const noexcept { return m_unhandled.empty();}

      std::ostream& operator << ( std::ostream& out, const Complete& value)
      {
         stream::write( out, "{ type: ", value.type(), ", correlation: ", value.correlation(), ", size: ",
            value.payload.size(), std::boolalpha, ", complete: ", value.complete(), ", unhandled: [");
         
         algorithm::for_each_interleave( value.m_unhandled, 
            [&]( auto& range)
            {
               out << " { offset: " << range.data() - value.payload.data() << ", size: " << range.size() << '}';
            }, 
            [&out](){ out << ",";});

         return out << "]}";
      }

      bool operator == ( const Complete& complete, const Complete::correlation_type& correlation)
      {
         return complete.correlation() == correlation;
      }

   } // common::communication::ipc::message

} // casual
