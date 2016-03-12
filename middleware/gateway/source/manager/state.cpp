//!
//! casual 
//!

#include "gateway/manager/state.h"


namespace casual
{

   namespace gateway
      {
         namespace manager
         {
            namespace state
            {
               namespace local
               {
                  namespace
                  {
                     std::string type( state::base_connection::Type type)
                     {
                        switch( type)
                        {
                           case base_connection::Type::ipc: { return "ipc";}
                           case base_connection::Type::tcp: { return "tcp";}
                           default: return "unknown";
                        }
                     }
                  } // <unnamed>
               } // local

               bool operator == ( const base_connection& lhs, common::platform::pid_type rhs)
               {
                  return lhs.process.pid;
               }


               namespace inbound
               {
                  std::ostream& operator << ( std::ostream& out, const Connection& value)
                  {
                     return out << "{ type: " << local::type( value.type) << ", process: " << value.process << ", remote: " << value.remote << '}';
                  }
               }

               namespace outbound
               {
                  std::ostream& operator << ( std::ostream& out, const Connection& value)
                  {
                     return out << "{ type: " << local::type( value.type) << ", process: " << value.process << ", remote: " << value.remote << '}';
                  }
               }

            } // state
         } // manager
      } // gateway
} // casual
