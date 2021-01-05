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
            
            state.outbounds = algorithm::transform( configuration.outbounds, []( auto& model)
            {
               manager::state::Outbound result;
               result.configuration = model;
               return result;
            });

            state.inbounds = algorithm::transform( configuration.inbounds, [](  auto& model)
            {
               manager::state::Inbound result;
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
               auto transform_connection = [&result]( auto bound)
               {
                  return [&result, bound]( auto& reply)
                  {
                     algorithm::transform( reply.state.connections, result.connections, [&reply, bound]( auto& connection)
                     {
                        manager::admin::model::Connection result;
                        result.alias = reply.state.alias;
                        result.bound = bound;
                        result.address.local = connection.address.local;
                        result.address.peer = connection.address.peer;
                        result.remote = connection.domain;
                        result.process = reply.process;

                        {
                           if( ! result.address.local.empty() && ! result.address.peer.empty())
                              result.runlevel = decltype( result.runlevel)::online;
                        };
                        return result;
                     });
                  };
               };

               algorithm::for_each( std::get< 0>( inbounds), transform_connection( manager::admin::model::connection::Bound::in));
               algorithm::for_each( std::get< 1>( inbounds), transform_connection( manager::admin::model::connection::Bound::in));
               algorithm::for_each( std::get< 0>( outbounds), transform_connection( manager::admin::model::connection::Bound::out));
               algorithm::for_each( std::get< 1>( outbounds), transform_connection( manager::admin::model::connection::Bound::out));

               algorithm::sort( result.connections);
            }

            // listeners
            {
               auto transform_listeners = [&result]( auto& reply)
               {
                  algorithm::transform( reply.state.listeners, result.listeners, [&reply]( auto& listener)
                  {
                     manager::admin::model::Listener result;
                     result.alias = reply.state.alias;
                     result.address.host = listener.address.host();
                     result.address.port = listener.address.port();
                     //result.limit.size = reply.state.limit.size;
                     //result.limit.messages = reply.state.limit.messages;
                     return result;
                  });
               };

               algorithm::for_each( std::get< 0>( inbounds), transform_listeners);
               algorithm::for_each( std::get< 1>( outbounds), transform_listeners);

               algorithm::sort( result.listeners);
            }

            return result;
         }

      } // transform
   } // gateway
} // casual
