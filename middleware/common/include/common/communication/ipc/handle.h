//!
//! casual
//!

#ifndef CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_COMMUNICATION_IPC_HANDLE_H_
#define CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_COMMUNICATION_IPC_HANDLE_H_

#include "common/platform.h"

#include "common/marshal/marshal.h"


#include <ostream>

namespace casual
{
   namespace common
   {

      namespace communication
      {
         namespace ipc
         {

            class Handle
            {
            public:
               using native_type = platform::ipc::handle::type;

               Handle() = default;
               inline explicit Handle( native_type id) : m_id( id) {}             

               inline native_type native() const { return m_id;}

               inline explicit operator bool () const { return m_id >= 0;}

               inline friend bool operator == ( Handle lhs, Handle rhs) { return lhs.m_id == rhs.m_id; }
               inline friend bool operator != ( Handle lhs, Handle rhs) { return ! ( lhs == rhs); }
               inline friend bool operator < ( Handle lhs, Handle rhs) { return lhs.m_id < rhs.m_id; }


               inline friend std::ostream& operator << ( std::ostream& out, Handle value) { return out << value.m_id;}

               CASUAL_CONST_CORRECT_MARSHAL(
               {
                  archive & m_id;
               })

            private:
               native_type m_id = -1;
            };

         } // ipc
      } // communication
   } // common
} // casual

#endif