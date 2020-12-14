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


               auto connection( manager::admin::model::Connection::Bound bound)
               {
                  return [bound]( auto& connection)
                  {
                     manager::admin::model::Connection result;

                     result.bound = bound;
                     result.process = connection.process;
                     result.remote = connection.remote;
                     result.runlevel = static_cast< manager::admin::model::Connection::Runlevel>( connection.runlevel);
                     result.address.local = connection.address.local;
                     result.address.peer = connection.address.peer;

                     return result;
                  };
               }

            } // <unnamed>
         } // local

         manager::State state( configuration::model::gateway::Model configuration)
         {
            Trace trace{ "gateway::transform::state"};

            manager::State state;

            auto set_order = [ order = platform::size::type{ 0}]( auto& outbound) mutable
            {
               outbound.order = ++order;
            };

            for( auto& listener : configuration.listeners)
               state.add( local::listener()( std::move( listener)));

            
            state.reverse.outbounds = algorithm::transform( configuration.reverse.outbounds, [&set_order](  auto& model)
            {
               manager::state::reverse::Outbound result;
               result.configuration = model;
               set_order( result);
               return result;
            });

            state.reverse.inbounds = algorithm::transform( configuration.reverse.inbounds, [](  auto& model)
            {
               manager::state::reverse::Inbound result;
               result.configuration = model;
               return result;
            });

            state.connections.outbound = algorithm::transform( configuration.connections, local::connection());

            // Define the order, hence the priority
            algorithm::for_each( state.connections.outbound, set_order);

            log::line( verbose::log, "state: ", state);

            return state;
         }

         manager::admin::model::State state( const manager::State& state,
            std::vector< message::reverse::inbound::state::Reply> inbounds, 
            std::vector< message::reverse::outbound::state::Reply> outbounds)
         {
            Trace trace{ "gateway::transform::state service"};

            manager::admin::model::State result;


            algorithm::transform( state.connections.outbound, result.connections, local::connection( manager::admin::model::Connection::Bound::out));
            algorithm::transform( state.connections.inbound, result.connections, local::connection( manager::admin::model::Connection::Bound::in));

            algorithm::for_each( outbounds, [&result]( auto& outbound)
            {
               algorithm::transform( outbound.state.connections, result.connections, [&outbound]( auto& connection)
               {
                  manager::admin::model::Connection result;
                  result.address.local = connection.address.local;
                  result.address.peer = connection.address.peer;
                  result.remote = connection.domain;
                  result.process = outbound.process;
                  result.runlevel = decltype( result.runlevel)::online;
                  return result;
               });

               algorithm::transform( outbound.state.listeners, result.listeners, []( auto& listener)
               {
                  manager::admin::model::Listener result;
                  result.address.host = listener.address.host();
                  result.address.port = listener.address.port();
                  return result;
               });

            });


            auto transform_listener = []( const auto& entry)
            {
               manager::admin::model::Listener result;
               result.address.host = entry.address().host();
               result.address.port = entry.address().port();
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
