//!
//! casual 
//!

#include "gateway/transform.h"

#include "common/algorithm.h"
#include "common/trace.h"
#include "common/environment.h"

namespace casual
{
   using namespace common;

   namespace gateway
   {
      namespace transform
      {
         namespace local
         {
            namespace
            {
               manager::state::outbound::Connection::Type type( const std::string& value)
               {
                  if( value == "ipc") return manager::state::outbound::Connection::Type::ipc;
                  if( value == "tcp") return manager::state::outbound::Connection::Type::tcp;
                  return manager::state::outbound::Connection::Type::unknown;
               }

               struct Connection
               {

                  manager::state::outbound::Connection operator () ( const config::gateway::Connection& connection) const
                  {
                     manager::state::outbound::Connection result;

                     result.type = local::type( connection.type);
                     result.address = common::environment::string( connection.address);
                     result.restart = connection.restart == "true";

                     return result;
                  }

               };

            } // <unnamed>
         } // local

         manager::State state( const config::gateway::Gateway& configuration)
         {
            Trace trace{ "gateway::transform::state", log::internal::gateway};

            manager::State state;

            common::range::transform( configuration.connections, state.connections.outbound, local::Connection{});


            return state;
         }


      } // transform

   } // gateway


} // casual
