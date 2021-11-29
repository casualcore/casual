//!
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "domain/discovery/handle.h"

#include "domain/message/discovery.h"
#include "domain/discovery/common.h"

#include "common/message/handle.h"
#include "common/message/event.h"
#include "common/event/listen.h"
#include "common/algorithm.h"
#include "common/event/send.h"
#include "common/communication/ipc/flush/send.h"

namespace casual
{
   using namespace common;

   namespace domain::discovery::handle
   {
      namespace local
      {
         namespace
         {

            namespace extract
            {
               struct Destination
               {
                  strong::ipc::id ipc;
                  strong::correlation::id correlation;

                  CASUAL_LOG_SERIALIZE( 
                     CASUAL_SERIALIZE( ipc);
                     CASUAL_SERIALIZE( correlation);
                  )
               };

               //! extracts and replaces process and correlation for the message
               //! @return the destination
               template< typename M>
               auto destination( M& message)
               {
                  return Destination{
                     std::exchange( message.process, common::process::handle()).ipc,
                     std::exchange( message.correlation, {})
                  };
               }
            } // extract


            namespace detail
            {
               namespace send
               {
                  template< typename R>
                  auto requests( const State& state, R&& range, const message::discovery::Request& message)
                  {
                     Trace trace{ "discovery::handle::local::detail::send::requests"};
                     assert( ! message.correlation);
                     assert( message.process == process::handle());

                     auto result = algorithm::accumulate( range, state.coordinate.discovery.empty_pendings(), [&message]( auto result, const auto& point)
                     {
                        if( auto correlation = communication::ipc::flush::optional::send( point.process.ipc, message))
                           result.emplace_back( correlation, point.process.pid);

                        return result;
                     });

                     log::line( verbose::log, "pending: ", result);
                     return result;
                  }

               } // send

               namespace normalize
               {
                  void content( message::discovery::reply::Content& content)
                  {
                     auto equal_name = []( auto& l, auto& r)
                     {
                        return l.name == r.name;
                     };

                     auto service_order = []( auto& l, auto& r)
                     {
                        auto tie = []( auto& value){ return std::tie( value.name, value.type, value.transaction);};
                        return tie( l) < tie( r);
                     };

                     algorithm::trim( content.services, algorithm::unique( algorithm::sort( content.services, service_order), equal_name));


                     auto queue_order = []( auto& l, auto& r)
                     {
                        auto tie = []( auto& value){ return std::tie( value.name, value.retries);};
                        return tie( l) < tie( r);
                     };

                     algorithm::trim( content.queues, algorithm::unique( algorithm::sort( content.queues, queue_order), equal_name));

                  }
               } // normalize

    

            } // detail

            namespace internal
            {
               auto registration( State& state)
               {
                  return [&state]( const message::discovery::internal::registration::Request& message)
                  {
                     Trace trace{ "discovery::handle::local::internal::registration"};
                     log::line( verbose::log, "message: ", message);

                     state.agents.registration( message);

                     // send reply
                     communication::ipc::flush::optional::send( message.process.ipc, common::message::reverse::type( message));
                  };
               }
            } // internal

            namespace external
            {
               auto registration( State& state)
               {
                  return [&state]( const message::discovery::external::registration::Request& message)
                  {
                     Trace trace{ "discovery::handle::local::external::registration"};
                     log::line( verbose::log, "message: ", message);

                     state.agents.registration( message);

                     // send event so others can do discovery, if they need to.
                     common::event::send( common::message::event::discoverable::Avaliable{ common::process::handle()});

                     // send reply
                     communication::ipc::flush::optional::send( message.process.ipc, common::message::reverse::type( message));
                  };
               }

               auto request( State& state)
               {
                  return [&state]( message::discovery::external::Request&& message)
                  {
                     Trace trace{ "discovery::handle::local::external::request"};
                     log::line( verbose::log, "message: ", message);

                     if( state.runlevel > decltype( state.runlevel())::running)
                     {
                        // we don't take any more request.
                        communication::ipc::flush::optional::send( message.process.ipc, common::message::reverse::type( message));
                        return;
                     }

                     // mutate the message and extract 'reply destination'
                     auto destination = local::extract::destination( message);

                     message::discovery::Request request{ process::handle()};
                     request.content = std::move( message.content);

                     // send request to all externals, if any.
                     auto pending = detail::send::requests( state, state.agents.external(), request);

                     // note: everything captured needs to by value (besides State if used)
                     state.coordinate.discovery( std::move( pending), [destination]( auto replies, auto failed)
                     {
                        Trace trace{ "discovery::handle::local::external::request coordinate"};
                        
                        // we could be the sender, if so, discard the reply.
                        if( destination.ipc == common::process::handle().ipc)
                           return;

                        message::discovery::external::Reply message;
                        message.correlation = destination.correlation;

                        communication::ipc::flush::optional::send( destination.ipc, message);
                     });
                  };
               }

               namespace advertised
               {
                  // message::discovery::external::advertised::Reply

                  auto reply( State& state)
                  {
                     return [&state]( message::discovery::external::advertised::Reply&& message)
                     {
                        Trace trace{ "discovery::handle::local::external::advertised::reply"};
                        log::line( verbose::log, "message: ", message);

                        state.coordinate.advertised( std::move( message));
                     };
                  }
                  
               } // advertised

            } // external

            auto request( State& state)
            {
               return [&state]( message::discovery::Request&& message)
               {
                  Trace trace{ "discovery::handle::local::request"};
                  log::line( verbose::log, "message: ", message);

                  if( state.runlevel > decltype( state.runlevel())::running)
                  {
                     // we don't take any more request.
                     communication::ipc::flush::optional::send( message.process.ipc, common::message::reverse::type( message));
                     return;
                  }

                  auto handle_request = []( State& state, auto&& range, message::discovery::Request& message)
                  {
                     // mutate the message and extract 'reply destination'
                     auto destination = local::extract::destination( message);
                     log::line( verbose::log, "destination: ", destination);

                     auto pending = detail::send::requests( state, range, message);

                     // note: everything captured needs to by value (besides State if used)
                     state.coordinate.discovery( std::move( pending), [destination]( auto replies, auto failed)
                     {
                        Trace trace{ "discovery::handle::local::request coordinate"};

                        message::discovery::Reply message{ process::handle()};
                        message.correlation = destination.correlation;

                        for( auto& reply : replies)
                        {
                           algorithm::append( std::move( reply.content.services), message.content.services);
                           algorithm::append( std::move( reply.content.queues), message.content.queues);
                        }
                        local::detail::normalize::content( message.content);
                        log::line( verbose::log, "message: ", message);

                        communication::ipc::flush::optional::send( destination.ipc, message);
                     });
                  };

                  // should we ask all 'agents' or only the local inbounds
                  if( message.directive == decltype( message.directive)::forward)
                     handle_request( state, state.agents.all(), message);
                  else
                     handle_request( state, state.agents.internal(), message);
               };
            }

            auto reply( State& state)
            {
               return [&state]( message::discovery::Reply&& message)
               {
                  Trace trace{ "discovery::handle::local::reply"};
                  log::line( verbose::log, "message: ", message);

                  state.coordinate.discovery( std::move( message));
         
               };
            }

            namespace rediscovery
            {
               auto request( State& state)
               {
                  return [&state]( message::discovery::rediscovery::Request&& message)
                  {
                     Trace trace{ "discovery::handle::local::rediscovery::request"};
                     log::line( verbose::log, "message: ", message);

                     if( state.runlevel > decltype( state.runlevel())::running)
                     {
                        // we don't take any more request.
                        communication::ipc::flush::optional::send( message.process.ipc, common::message::reverse::type( message));
                        return;
                     }

                     // mutate the message and extract 'reply destination', and set our self as receiver
                     auto destination = local::extract::destination( message);

                     auto pending = algorithm::accumulate( state.agents.rediscover(), state.coordinate.rediscovery.empty_pendings(), [&message]( auto result, const auto& point)
                     {
                        if( auto correlation = communication::ipc::flush::optional::send( point.process.ipc, message))
                           result.emplace_back( correlation, point.process.pid);

                        return result;
                     });

                     // note: everything captured needs to by value (besides State if used)
                     state.coordinate.rediscovery( std::move( pending), [destination]( auto replies, auto failed)
                     {
                        Trace trace{ "discovery::handle::local::rediscovery::request"};

                        message::discovery::rediscovery::Reply message;
                        message.correlation = destination.correlation;
                        communication::ipc::flush::optional::send( destination.ipc, message);
                     });
                  };
               }

               auto reply( State& state)
               {
                  return [&state]( message::discovery::rediscovery::Reply&& message)
                  {
                     Trace trace{ "discovery::handle::local::rediscovery::reply"};
                     log::line( verbose::log, "message: ", message);

                     state.coordinate.rediscovery( std::move( message));
                  };
               }
               
            } // rediscovery

            namespace event
            {
               namespace process
               {
                  
                  auto exit( State& state)
                  {
                     return [&state]( const common::message::event::process::Exit& event)
                     {
                        Trace trace{ "discovery::handle::local::event::process::exit"};
                        log::line( verbose::log, "event: ", event);

                        state.agents.remove( event.state.pid);
                        state.coordinate.discovery.failed( event.state.pid);
                        state.coordinate.rediscovery.failed( event.state.pid);
                     };
                  }
               } // process

               namespace discoverable
               {
                  auto available( State& state)
                  {
                     return [&state]( const common::message::event::discoverable::Avaliable& event)
                     {
                        Trace trace{ "discovery::handle::local::event::discoverable::available"};
                        common::log::line( verbose::log, "event: ", event);

                        // we know there's a new _discoverable_ available.
                        // collect all known advertised external services/queues, and when we get'em
                        // we discover these

                        auto pending = algorithm::accumulate( state.agents.external(), state.coordinate.advertised.empty_pendings(), []( auto result, const auto& point)
                        {   
                           if( auto correlation = communication::ipc::flush::optional::send( point.process.ipc, message::discovery::external::advertised::Request{ common::process::handle()}))
                              result.emplace_back( correlation, point.process.pid);

                           return result;
                        });

                        // note: everything captured needs to by value (besides State if used)
                        state.coordinate.advertised( std::move( pending), [&state]( auto replies, auto failed)
                        {
                           Trace trace{ "discovery::handle::local::event::discoverable::available replied"};

                           message::discovery::external::Request request{ common::process::handle()};
                           request.content = algorithm::accumulate( replies, message::discovery::request::Content{}, []( auto result, auto& reply)
                           {
                              return result += std::move( reply.content);
                           });

                           common::log::line( verbose::log, "request: ", request);

                           // we use the regular external handler to send and coordinate external::Request.
                           local::external::request( state)( std::move( request));
                        });
                     };
                  }
                  
               } // discoverable

            } // event

            namespace shutdown
            {
               auto request( State& state)
               {
                  return [&state]( const common::message::shutdown::Request& message)
                  {
                     Trace trace{ "discovery::handle::local::shutdown::request"};
                     log::line( verbose::log, "message: ", message);

                     state.runlevel = decltype( state.runlevel())::shutdown;
                  };
               }
            } // shutdown

         } // <unnamed>
      } // local

      dispatch_type create( State& state)
      {
         return {
            common::message::handle::defaults( common::communication::ipc::inbound::device()),
            common::event::listener( 
               local::event::process::exit( state),
               local::event::discoverable::available( state)),
            local::external::registration( state),
            local::external::request( state),
            local::external::advertised::reply( state),
            local::internal::registration( state),
            local::request( state),
            local::reply( state),
            local::rediscovery::request( state),
            local::rediscovery::reply( state),
            local::shutdown::request( state)
         };
      }

   } // domain::discovery::handle
} // casual