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
      Complete::Complete( message_type_type type, const Uuid& correlation) : type{ type}, correlation{ correlation} {}

      Complete::Complete( common::message::Type type, const Uuid& correlation, payload_type&& payload)
         : type{ type}, correlation{ correlation}, payload{ std::move( payload)} {}


      Complete::operator bool() const
      {
         return type != message_type_type::absent_message;
      }

      bool Complete::complete() const { return m_unhandled.empty();}

      std::ostream& operator << ( std::ostream& out, const Complete& value)
      {
         return out << "{ type: " << value.type << ", correlation: " << value.correlation << ", size: "
               << value.payload.size() << std::boolalpha << ", complete: " << value.complete() << '}';
      }

      bool operator == ( const Complete& complete, const Uuid& correlation)
      {
         return complete.correlation == correlation;
      }

   } // common::communication::ipc::message

} // casual
