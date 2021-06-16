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


            } // <unnamed>
         } // local

         manager::State state( configuration::model::gateway::Model configuration)
         {
            Trace trace{ "gateway::transform::state"};

            manager::State state;
            
            state.outbound.groups = algorithm::transform( configuration.outbound.groups, []( auto& model)
            {
               manager::state::outbound::Group result;
               result.configuration = model;
               return result;
            });

            state.inbound.groups = algorithm::transform( configuration.inbound.groups, []( auto& model)
            {
               manager::state::inbound::Group result;
               result.configuration = model;
               return result;
            });

            log::line( verbose::log, "state: ", state);

            return state;
         }

         manager::admin::model::State state( const manager::State& state,
            std::tuple< std::vector< message::inbound::state::Reply>, std::vector< message::inbound::reverse::state::Reply>> inbounds, 
            std::tuple< std::vector< message::outbound::state::Reply>, std::vector< message::outbound::reverse::state::Reply>> outbounds)
         {
            Trace trace{ "gateway::transform::state service"};
            log::line( verbose::log, "inbounds: ", inbounds);
            log::line( verbose::log, "outbounds: ", outbounds);

            manager::admin::model::State result;

            // connections
            {
               auto transform = [&result]( auto connect, auto bound)
               {
                  return [&result, connect, bound]( auto& reply)
                  {
                     algorithm::transform( reply.state.connections, result.connections, [&reply, connect, bound]( auto& connection)
                     {
                        manager::admin::model::Connection result;
                        result.group = reply.state.alias;
                        result.connect = connect;
                        result.bound = bound;
                        result.descriptor = connection.descriptor;
                        result.address.local = connection.address.local;
                        result.address.peer = connection.address.peer;
                        result.remote = connection.domain;
                        result.created = connection.created;

                        // deprecated remove in 2.0
                        result.process = reply.process; 

                        return result;
                     });
                  };
               };

               using Bound = manager::admin::model::connection::Bound;
               using Phase = manager::admin::model::connection::Phase;

               algorithm::for_each( std::get< 0>( inbounds), transform( Phase::regular, Bound::in));
               algorithm::for_each( std::get< 1>( inbounds), transform( Phase::reversed, Bound::in));
               algorithm::for_each( std::get< 0>( outbounds), transform( Phase::regular, Bound::out));
               algorithm::for_each( std::get< 1>( outbounds), transform( Phase::reversed, Bound::out));

               algorithm::sort( result.connections);
            }

            // groups
            {
               using Phase = manager::admin::model::connection::Phase;

               auto set_general = []( auto& reply, auto& result)
               {
                  result.process = reply.process;
                  result.alias = reply.state.alias;
                  result.note = reply.state.note;
               };

               auto transform_inbound = [set_general]( auto connect)
               {
                  return [set_general, connect]( auto& reply)
                  {
                     manager::admin::model::inbound::Group result;
                     set_general( reply, result);
                     result.connect = connect;
                     result.limit.size = reply.state.limit.size;
                     result.limit.messages = reply.state.limit.messages;
                     return result;
                  };
               };

               auto transform_outbound = [set_general]( auto connect)
               {
                  return [set_general, connect]( auto& reply)
                  {
                     manager::admin::model::outbound::Group result;
                     set_general( reply, result);
                     result.connect = connect;
                     result.order = reply.state.order;

                     result.pending.messages = algorithm::transform( reply.state.pending.messages, []( auto& message)
                     {
                        return manager::admin::model::outbound::pending::Message{
                           message.type,
                           message.count
                        };
                     });

                     return result;
                  };
               };
               

               algorithm::transform( std::get< 0>( inbounds), std::back_inserter( result.inbound.groups), transform_inbound( Phase::regular));
               algorithm::transform( std::get< 1>( inbounds), std::back_inserter( result.inbound.groups), transform_inbound( Phase::reversed));
               algorithm::transform( std::get< 0>( outbounds), std::back_inserter( result.outbound.groups), transform_outbound( Phase::regular));
               algorithm::transform( std::get< 1>( outbounds), std::back_inserter( result.outbound.groups), transform_outbound( Phase::reversed));

            }

            // listeners
            {
               auto transform = [&result]( auto bound)
               {
                  return [&result, bound]( auto& reply)
                  {
                     algorithm::transform( reply.state.listeners, result.listeners, [&reply, bound]( auto& listener)
                     {
                        manager::admin::model::Listener result;
                        result.group = reply.state.alias;
                        result.address.host = listener.address.host();
                        result.address.port = listener.address.port();
                        result.bound = bound;
                        result.created = listener.created;
                        return result;
                     });
                  };
               };

               algorithm::for_each( std::get< 0>( inbounds), transform( manager::admin::model::connection::Bound::in));
               algorithm::for_each( std::get< 1>( outbounds), transform( manager::admin::model::connection::Bound::out));

               algorithm::sort( result.listeners);
            }

            // services
            {
               auto transform = [&result]( auto& reply)
               {
                  algorithm::for_each( reply.state.correlation.services, [&result]( auto& routing_entry)
                  {
                     auto name = routing_entry.name;
                     auto connections = routing_entry.connections;

                     decltype( typename decltype(result.services)::value_type().connections) transformed_connections;

                     algorithm::transform( connections, transformed_connections,[]( auto& connection)
                     {
                        typename decltype(transformed_connections)::value_type value{ connection.pid, connection.descriptor};
                        return value;
                     });

                     auto found = algorithm::find_if( result.services, [name]( auto service)
                     {
                        return name == service.name;
                     });
     
                     if (found)
                        algorithm::append( transformed_connections, found->connections);
                     else
                     {
                        typename decltype( result.services)::value_type value;
                        value.name = name;
                        value.connections = transformed_connections;
                        result.services.push_back( std::move( value));
                     }
                  });
               };
               algorithm::for_each( std::get< 0>( outbounds), transform);
               algorithm::for_each( std::get< 1>( outbounds), transform);
               algorithm::sort( result.services);
            }

            // queue
            {
               auto transform = [&result]( auto& reply)
               {
                  algorithm::for_each( reply.state.correlation.queues, [&result]( auto& routing_entry)
                  {
                     auto name = routing_entry.name;
                     auto connections = routing_entry.connections;
 
                     decltype( typename decltype(result.queues)::value_type().connections) transformed_connections;

                     algorithm::transform( connections, transformed_connections,[]( auto& connection)
                     {
                        typename decltype( transformed_connections)::value_type value{ connection.pid, connection.descriptor};
                        return value;
                     });

                     auto found = algorithm::find_if( result.queues, [name]( auto queue)
                     {
                        return name == queue.name;
                     });

                     if (found)
                        algorithm::append( transformed_connections, found->connections);
                     else
                     {
                        typename decltype( result.queues)::value_type value;
                        value.name = name;
                        value.connections = transformed_connections;
                        result.queues.push_back( std::move( value));
                     }
                  });
               };
               algorithm::for_each( std::get< 0>( outbounds), transform);
               algorithm::for_each( std::get< 1>( outbounds), transform);
               algorithm::sort( result.queues);
            }

            return result;
         }

      } // transform
   } // gateway
} // casual
