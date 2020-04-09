//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "gateway/transform.h"
#include "gateway/common.h"

#include "common/algorithm.h"
#include "common/environment/normalize.h"
#include "common/event/send.h"
#include "common/exception/handle.h"

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
               auto connection()
               {
                  return []( auto&& connection)
                  {
                     manager::state::outbound::Connection result;

                     result.address.peer = connection.address;
                     result.restart = connection.lifetime.restart;
                     result.services = std::move( connection.services);
                     result.queues = std::move( connection.queues);

                     environment::normalize( result);
                     return result;
                  };
               }

               auto listener()
               {
                  return []( auto&& value) -> manager::listen::Entry
                  {
                     manager::listen::Limit limit;
                     {
                        limit.messages = value.limit.messages;
                        limit.size = value.limit.size;
                     }

                     return { value.address, limit};
                  };

               }


               namespace vo
               {
                  struct Connection
                  {
                     manager::admin::model::Connection operator() ( const manager::state::inbound::Connection& value) const
                     {
                        auto result = transform( value);
                        result.bound = manager::admin::model::Connection::Bound::in;

                        return result;
                     }

                     manager::admin::model::Connection operator() ( const manager::state::outbound::Connection& value) const
                     {
                        auto result = transform( value);
                        result.bound = manager::admin::model::Connection::Bound::out;

                        return result;
                     }

                  private:

                     template< typename T>
                     manager::admin::model::Connection transform( const T& value) const
                     {
                        manager::admin::model::Connection result;

                        result.process = value.process;
                        result.remote = value.remote;
                        result.runlevel = static_cast< manager::admin::model::Connection::Runlevel>( value.runlevel);
                        result.address.local = value.address.local;
                        result.address.peer = value.address.peer;

                        return result;
                     }

                  };
               } // vo
            } // <unnamed>
         } // local

         manager::State state( configuration::model::gateway::Model configuration)
         {
            Trace trace{ "gateway::transform::state"};

            manager::State state;

            for( auto& listener : configuration.listeners)
               state.add( local::listener()( std::move( listener)));

            state.connections.outbound = algorithm::transform( configuration.connections, local::connection());

            // Define the order, hence the priority
            algorithm::for_each( state.connections.outbound, [order = platform::size::type{ 0}]( auto& outbound) mutable
            {
               outbound.order = ++order;
            });

            log::line( verbose::log, "state: ", state);

            return state;
         }

         manager::admin::model::State state( const manager::State& state)
         {
            Trace trace{ "gateway::transform::state service"};

            manager::admin::model::State result;


            algorithm::transform( state.connections.outbound, result.connections, local::vo::Connection{});
            algorithm::transform( state.connections.inbound, result.connections, local::vo::Connection{});


            auto transform_listener = []( const manager::listen::Entry& entry)
            {
               manager::admin::model::Listener result;
               result.address.host = entry.address().host;
               result.address.port = entry.address().port;
               result.limit.size = entry.limit().size;
               result.limit.messages = entry.limit().messages;
               
               return result;
            };

            algorithm::transform( state.listeners(), result.listeners, transform_listener);

            return result;
         }

      } // transform
   } // gateway
} // casual
