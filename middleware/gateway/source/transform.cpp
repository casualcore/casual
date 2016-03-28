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

               struct Listener
               {

                  communication::tcp::Address operator () ( const config::gateway::Listener& value) const
                  {
                     return { value.address};
                  }

               };

               namespace vo
               {
                  struct Connection
                  {
                     manager::admin::vo::inbound::Connection operator() ( const manager::state::inbound::Connection& value) const
                     {
                        auto result = transform< manager::admin::vo::inbound::Connection>( value);

                        return result;
                     }

                     manager::admin::vo::outbound::Connection operator() ( const manager::state::outbound::Connection& value) const
                     {
                        auto result = transform< manager::admin::vo::outbound::Connection>( value);

                        return result;
                     }

                  private:

                     template< typename R, typename T>
                     R transform( const T& value) const
                     {
                        R result;

                        result.process = value.process;
                        result.remote = value.remote;
                        result.runlevel = static_cast< typename R::Runlevel>( value.runlevel);
                        result.type = static_cast< typename R::Type>( value.type);

                        return result;
                     }

                  };

               } // vo

            } // <unnamed>
         } // local

         manager::State state( const config::gateway::Gateway& configuration)
         {
            Trace trace{ "gateway::transform::state", log::internal::gateway};

            manager::State state;

            range::transform( configuration.listeners, state.configuration.listeners, local::Listener{});
            range::transform( configuration.connections, state.connections.outbound, local::Connection{});

            return state;
         }

         manager::admin::vo::State state( const manager::State& state)
         {
            Trace trace{ "gateway::transform::state service", log::internal::gateway};

            manager::admin::vo::State result;


            range::transform( state.connections.inbound, result.connections.inbound, local::vo::Connection{});
            range::transform( state.connections.outbound, result.connections.outbound, local::vo::Connection{});


            return result;

         }


      } // transform

   } // gateway


} // casual
