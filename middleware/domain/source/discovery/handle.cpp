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
                  Uuid correlation;

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
                        if( auto correlation = communication::device::blocking::optional::send( point.process.ipc, message))
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

            namespace inbound
            {
               auto registration( State& state)
               {
                  return [&state]( const message::discovery::inbound::Registration& message)
                  {
                     Trace trace{ "discovery::handle::local::inbound::registration"};
                     log::line( verbose::log, "message: ", message);

                     state.agents.registration( message);
                  };
               }
            } // inbound

            namespace outbound
            {
               auto registration( State& state)
               {
                  return [&state]( const message::discovery::outbound::Registration& message)
                  {
                     Trace trace{ "discovery::handle::local::outbound::registration"};
                     log::line( verbose::log, "message: ", message);

                     state.agents.registration( message);

                      // send event so others can do discovery, if they need to.
                     common::event::send( common::message::event::discoverable::Avaliable{ common::process::handle()});                           

                  };
               }

               auto request( State& state)
               {
                  return [&state]( message::discovery::outbound::Request&& message)
                  {
                     Trace trace{ "discovery::handle::local::outbound::request"};
                     log::line( verbose::log, "message: ", message);

                     if( state.runlevel > decltype( state.runlevel())::running)
                     {
                        // we don't take any more request.
                        communication::device::blocking::optional::send( message.process.ipc, common::message::reverse::type( message));
                        return;
                     }

                     // mutate the message and extract 'reply destination'
                     auto destination = local::extract::destination( message);

                     message::discovery::Request request{ process::handle()};
                     request.content = std::move( message.content);

                     // send request to all outbounds, if any.
                     auto pending = detail::send::requests( state, state.agents.outbounds(), request);

                     // note: everything captured needs to by value (besides State if used)
                     state.coordinate.discovery( std::move( pending), [destination]( auto replies, auto failed)
                     {
                        Trace trace{ "discovery::handle::local::outbound::request coordinate"};

                        message::discovery::outbound::Reply message;
                        message.correlation = destination.correlation;

                        communication::device::blocking::optional::send( destination.ipc, message);
                     });
                  };
               }

            } // outbound

            auto request( State& state)
            {
               return [&state]( message::discovery::Request&& message)
               {
                  Trace trace{ "discovery::handle::local::request"};
                  log::line( verbose::log, "message: ", message);

                  if( state.runlevel > decltype( state.runlevel())::running)
                  {
                     // we don't take any more request.
                     communication::device::blocking::optional::send( message.process.ipc, common::message::reverse::type( message));
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

                        communication::device::blocking::optional::send( destination.ipc, message);
                     });
                  };

                  // should we ask all 'agents' or only the local inbounds
                  if( message.directive == decltype( message.directive)::forward)
                     handle_request( state, state.agents.all(), message);
                  else
                     handle_request( state, state.agents.inbounds(), message);
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
                        communication::device::blocking::optional::send( message.process.ipc, common::message::reverse::type( message));
                        return;
                     }

                     // mutate the message and extract 'reply destination', and set our self as receiver
                     auto destination = local::extract::destination( message);

                     auto pending = algorithm::accumulate( state.agents.rediscovers(), state.coordinate.rediscovery.empty_pendings(), [&message]( auto result, const auto& point)
                     {
                        if( auto correlation = communication::device::blocking::optional::send( point.process.ipc, message))
                           result.emplace_back( correlation, point.process.pid);

                        return result;
                     });

                     // note: everything captured needs to by value (besides State if used)
                     state.coordinate.rediscovery( std::move( pending), [destination]( auto replies, auto failed)
                     {
                        Trace trace{ "discovery::handle::local::rediscovery::request"};

                        message::discovery::rediscovery::Reply message;
                        message.correlation = destination.correlation;
                        communication::device::blocking::optional::send( destination.ipc, message);
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

            namespace event::process
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

            } // event::process

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
            common::event::listener( local::event::process::exit( state)),
            local::outbound::registration( state),
            local::outbound::request( state),
            local::inbound::registration( state),
            local::request( state),
            local::reply( state),
            local::rediscovery::request( state),
            local::rediscovery::reply( state),
            local::shutdown::request( state)
         };
      }

   } // domain::discovery::handle
} // casual