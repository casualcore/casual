//!
//! casual 
//!

#include "gateway/transform.h"
#include "gateway/common.h"

#include "common/algorithm.h"
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
               manager::state::outbound::Connection::Type type( common::message::domain::configuration::gateway::Connection::Type value)
               {
                  using connection_type = common::message::domain::configuration::gateway::Connection::Type;

                  switch( value)
                  {
                     case connection_type::ipc: return manager::state::outbound::Connection::Type::ipc;
                     case connection_type::tcp: return manager::state::outbound::Connection::Type::tcp;
                  }

                  return manager::state::outbound::Connection::Type::unknown;
               }

               struct Connection
               {

                  manager::state::outbound::Connection operator () ( const common::message::domain::configuration::gateway::Connection& connection) const
                  {
                     manager::state::outbound::Connection result;

                     result.type = local::type( connection.type);
                     result.address.push_back( common::environment::string( connection.address));
                     result.restart = connection.restart;
                     result.services = std::move( connection.services);
                     result.queues = std::move( connection.queues);

                     return result;
                  }
               };

               struct Listener
               {



                  manager::Listener operator () ( const common::message::domain::configuration::gateway::Listener& value) const
                  {
                     manager::Listener::Limit limit;
                     {
                        limit.messages = value.limit.messages;
                        limit.size = value.limit.size;
                     }

                     return { value.address, limit};
                  }

               };

               namespace vo
               {
                  struct Connection
                  {
                     manager::admin::vo::Connection operator() ( const manager::state::inbound::Connection& value) const
                     {
                        auto result = transform( value);
                        result.bound = manager::admin::vo::Connection::Bound::in;

                        return result;
                     }

                     manager::admin::vo::Connection operator() ( const manager::state::outbound::Connection& value) const
                     {
                        auto result = transform( value);
                        result.bound = manager::admin::vo::Connection::Bound::out;

                        return result;
                     }

                  private:

                     template< typename T>
                     manager::admin::vo::Connection transform( const T& value) const
                     {
                        manager::admin::vo::Connection result;

                        result.process = value.process;
                        result.remote = value.remote;
                        result.runlevel = static_cast< manager::admin::vo::Connection::Runlevel>( value.runlevel);
                        result.type = static_cast< manager::admin::vo::Connection::Type>( value.type);
                        result.address = value.address;

                        return result;
                     }

                  };

               } // vo

            } // <unnamed>
         } // local

         manager::State state( const common::message::domain::configuration::Domain& configuration)
         {
            Trace trace{ "gateway::transform::state"};

            manager::State state;

            algorithm::transform( configuration.gateway.listeners, state.listeners, local::Listener{});
            algorithm::transform( configuration.gateway.connections, state.connections.outbound, local::Connection{});

            //
            // Define the order, hence the priority
            //
            {
               std::size_t order = 0;
               for( auto& connection : state.connections.outbound)
               {
                  connection.order = ++order;
               }
            }


            return state;
         }

         manager::admin::vo::State state( const manager::State& state)
         {
            Trace trace{ "gateway::transform::state service"};

            manager::admin::vo::State result;


            algorithm::transform( state.connections.outbound, result.connections, local::vo::Connection{});
            algorithm::transform( state.connections.inbound, result.connections, local::vo::Connection{});


            return result;

         }


      } // transform

   } // gateway


} // casual
