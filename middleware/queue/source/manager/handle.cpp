//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "queue/manager/handle.h"
#include "queue/manager/admin/server.h"
#include "queue/manager/transform.h"
#include "queue/common/log.h"
#include "queue/common/ipc/message.h"


#include "domain/discovery/api.h"

#include "configuration/message.h"

#include "common/process.h"
#include "common/server/lifetime.h"
#include "common/server/handle/call.h"
#include "common/event/listen.h"
#include "common/event/send.h"
#include "common/algorithm/compare.h"
#include "common/message/handle.h"


#include "common/code/raise.h"
#include "common/code/casual.h"

#include "common/communication/instance.h"


namespace casual
{
   using namespace common;

   namespace queue::manager
   {

      namespace handle
      {

         namespace local
         {
            namespace
            {

               template< typename G, typename M>
               void send( State& state, G&& groups, M&& message)
               {
                  // Try to send it first with no blocking.
                  auto busy = common::algorithm::filter( groups, [&message]( auto& group)
                  {
                     return ! communication::device::non::blocking::send( group.queue, message);
                  });

                  // Block for the busy ones, if any
                  algorithm::for_each( busy, [&message]( auto& group)
                  {
                     communication::device::blocking::send( group.queue, message);
                  });
               }

               namespace pending::lookups
               {
                  //! check if there are pending lookups that can be replied 
                  void check( State& state)
                  {
                     Trace trace{ "queue::manager::handle::local::pending::lookups::check"};
                     log::line( verbose::log, "pending: ", state.pending.lookups);

                     auto lookup_replied = [&state]( auto& lookup)
                     {
                        if( auto queue = state.queue( lookup.name))
                        {
                           auto reply = common::message::reverse::type( lookup);
                           reply.name = lookup.name;
                           reply.queue = queue->queue;
                           reply.process = queue->process;
                           reply.order = queue->order;

                           return predicate::boolean( communication::device::blocking::optional::send( lookup.process.ipc, reply));
                        }
                        return false;
                     };

                     algorithm::trim( state.pending.lookups, algorithm::remove_if( state.pending.lookups, lookup_replied));
                  }

                  void discard( State& state)
                  {
                     Trace trace{ "queue::manager::handle::local::pending::lookups::discard"};

                     auto pending = std::exchange( state.pending.lookups, {});
                     log::line( verbose::log, "pending: ", pending);

                     auto discard_lookup = []( auto& lookup)
                     {
                        auto reply = common::message::reverse::type( lookup);
                        reply.name = lookup.name;
                        communication::device::blocking::optional::send( lookup.process.ipc, reply);
                     };

                     algorithm::for_each( pending, discard_lookup);
                  }


               } // pending::lookups


               namespace discovery
               {
                  auto send( std::vector< std::string> queues, const strong::correlation::id& correlation = strong::correlation::id::emplace( uuid::make()))
                  {
                     Trace trace{ "queue::manager::handle::local::discovery::send"};

                     casual::domain::message::discovery::external::Request request{ common::process::handle()};
                     request.correlation = correlation;
                     request.content.queues = std::move( queues);

                     log::line( verbose::log, "request: ", request);

                     return casual::domain::discovery::external::request( request);
                  }
               }
            } // <unnamed>
         } // local

         namespace process
         {
            void exit( const common::process::lifetime::Exit& exit)
            {
               Trace trace{ "handle::process::exit"};
               common::log::line( verbose::log, "exit: ", exit);

               // one of our own children has died, we send the event.
               // we'll later receive the event from domain-manager, since
               // we listen to process-exit-events
               common::event::send( common::message::event::process::Exit{ exit});
            }
         } // process


         namespace local
         {
            namespace
            {
               namespace event
               {
                  namespace process
                  {
                     auto exit( State& state)
                     {
                        return [&state]( const common::message::event::process::Exit& event)
                        {
                           Trace trace{ "queue::manager::handle::local::event::process::exit"};
                           common::log::line( verbose::log, "event: ", event);
                           state.remove( event.state.pid);
                        };
                     }
                  } // process

                  namespace discoverable
                  {
                     auto available( State& state)
                     {
                        return [&state]( const common::message::event::discoverable::Avaliable& event)
                        {
                           Trace trace{ "queue::manager::handle::local::event::discoverable::available"};
                           common::log::line( verbose::log, "event: ", event);

                           auto queues = algorithm::transform( state.pending.lookups, []( auto& pending){ return pending.name;});

                           algorithm::trim( queues, algorithm::unique( algorithm::sort( queues)));

                           if( ! queues.empty())
                              local::discovery::send( std::move( queues));
                        };
                     }
                     
                  } // discoverable
                  
               } // event

               namespace shutdown
               {
                  auto request( State& state)
                  {
                     return [&state]( common::message::shutdown::Request& message)
                     {
                        Trace trace{ "queue::manager::handle::local::shutdown::request"};
                        common::log::line( verbose::log, "message: ", message);

                        state.runlevel = decltype( state.runlevel())::shutdown;

                        local::pending::lookups::discard( state);

                        auto shutdown = [&]( auto& entity)
                        {
                           if( entity.process.ipc)
                           {
                              communication::device::blocking::optional::send( 
                                 entity.process.ipc,
                                 common::message::shutdown::Request{ common::process::handle()});
                           }
                           else
                              signal::send( entity.process.pid, code::signal::terminate);
                        };

                        algorithm::for_each( state.forward.groups, shutdown);
                        algorithm::for_each( state.groups, shutdown);
                     };
                  }

               } // shutdown

               namespace lookup
               {
                  auto request( State& state)
                  {
                     return [&state]( queue::ipc::message::lookup::Request& message)
                     {
                        Trace trace{ "handle::local::lookup::request"};
                        common::log::line( verbose::log, "message: ", message);

                        auto reply = common::message::reverse::type( message);
                        reply.name = message.name;

                        auto send_reply = common::execute::scope( [&]()
                        {
                           common::log::line( verbose::log, "reply.execution: ", reply.execution);
                           communication::device::blocking::optional::send( message.process.ipc, reply);
                        });

                        if( auto queue = state.queue( message.name))
                        {
                           log::line( log, "queue provider found: ", *queue);
                           reply.queue = queue->queue;
                           reply.process = queue->process;
                           reply.order = queue->order;
                        }
                        else
                        {
                           // We didn't find the queue, let's try to ask our neighbors.

                           common::log::line( log, "queue not found - ", message.name);

                           if( local::discovery::send( { message.name}, message.correlation)
                              || message.context == decltype( message.context)::wait)
                           {
                              // either we've sent a discovery or the lookup want's to wait (possible for ever).
                              state.pending.lookups.push_back( std::move( message));

                              log::line( log, "pending request added: " , state.pending);

                              // We don't send the 'absent reply'.
                              send_reply.release();
                           }
                        }
                     };
                  }

                  namespace discard
                  {
                     auto request( State& state)
                     {
                        return [&state]( const queue::ipc::message::lookup::discard::Request& message)
                        {
                           Trace trace{ "handle::lookup::discard::Request"};
                           common::log::line( verbose::log, "message: ", message);

                           auto reply = message::reverse::type( message);

                           if( auto found = algorithm::find( state.pending.lookups, message.correlation))
                           {
                              reply.state = decltype( reply.state)::discarded;
                              state.pending.lookups.erase( std::begin( found));
                           }
                           else
                              reply.state = decltype( reply.state)::replied;

                           communication::device::blocking::optional::send( message.process.ipc, reply);
                        };
                     }
                  } // discard

               } // lookup

               namespace group
               {
                  auto connect( State& state)
                  {
                     return [&state]( queue::ipc::message::group::Connect& message)
                     {
                        Trace trace{ "queue::manager::handle::local::group::connect"};
                        log::line( verbose::log, "message: ", message);

                        if( auto found = common::algorithm::find( state.groups, message.process.pid))
                        {
                           found->state = decltype( found->state())::connected;
                           found->process = message.process;

                           // send configuration
                           queue::ipc::message::group::configuration::update::Request request{ common::process::handle()};
                           request.model = found->configuration;
                           communication::device::blocking::send( message.process.ipc, request);
                        }
                        else
                           common::log::line( common::log::category::error, "failed to correlate queue group - ", message.process.pid);
                     };
                  }

                  namespace configuration::update
                  {
                     auto reply( State& state)
                     {
                        return [&state]( queue::ipc::message::group::configuration::update::Reply&& message)
                        {
                           Trace trace{ "queue::manager::handle::local::group::configuration::update::reply"};
                           log::line( verbose::log, "message: ", message);

                           state.update( std::move( message));

                           pending::lookups::check( state);
                        };
                     }
                  } // configuration::update

               } // group

               namespace forward
               {
                  auto connect( State& state)
                  {
                     return [&state]( queue::ipc::message::forward::group::Connect& message)
                     {
                        Trace trace{ "queue::manager::handle::local::forward::connect"};
                        log::line( verbose::log, "message: ", message);

                        if( auto found = common::algorithm::find( state.forward.groups, message.process.pid))
                        {
                           log::line( verbose::log, "found: ", *found);

                           found->state = decltype( found->state())::connected;
                           found->process = message.process;

                           // send configuration
                           queue::ipc::message::forward::group::configuration::update::Request request{ common::process::handle()};
                           request.model = found->configuration;
                           communication::device::blocking::send( message.process.ipc, request);
                        }
                        else
                           common::log::line( common::log::category::error, "failed to correlate forward group - ", message.process.pid);
                     };
                  }

                  namespace configuration::update
                  {
                     auto reply( State& state)
                     {
                        return [&state]( queue::ipc::message::forward::group::configuration::update::Reply&& message)
                        {
                           Trace trace{ "queue::manager::handle::local::forward::configuration::update::reply"};
                           log::line( verbose::log, "message: ", message);

                           if( auto found = common::algorithm::find( state.forward.groups, message.process.pid))
                              found->state = decltype( found->state())::running;
                           else
                              common::log::line( common::log::category::error, "failed to correlate forward group - ", message.process.pid);
                        };
                     }
                  } // configuration::update
                  
               } // forward

               auto advertise( State& state)
               {
                  return [&state]( queue::ipc::message::Advertise& message)
                  {
                     Trace trace{ "queue::manager::handle::local::concurrent::advertise"};
                     log::line( verbose::log, "message: ", message);

                     state.update( message);

                     pending::lookups::check( state);
                  }; 
               }

               namespace domain
               {
                  namespace discover
                  {
                     auto request( State& state)
                     {
                        return [&state]( casual::domain::message::discovery::Request&& message)
                        {
                           Trace trace{ "queue::manager::handle::local::domain::discover::request"};
                           common::log::line( verbose::log, "message: ", message);

                           auto reply = common::message::reverse::type( message);

                           reply.process = common::process::handle();
                           reply.domain = common::domain::identity();

                           auto is_relevant = [&state, directive = message.directive]( auto& name)
                           {
                              if( auto found = algorithm::find( state.queues, name))
                              {
                                 if( directive == decltype( directive)::forward)
                                    return ! found->second.empty();

                                 return algorithm::any_of( found->second, []( auto& instance){ return ! instance.remote();});
                              }
                              return false;
                           };


                           for( auto& queue : algorithm::filter( message.content.queues, is_relevant))
                              reply.content.queues.emplace_back( std::move( queue));

                           common::log::line( verbose::log, "reply: ", reply);

                           communication::device::blocking::send( message.process.ipc, reply);
                        };
                     }

                     auto reply( State& state)
                     {
                        return [&state]( casual::domain::message::discovery::external::Reply& message)
                        {
                           Trace trace{ "queue::manager::handle::local::domain::discover::reply"};
                           common::log::line( verbose::log, "message: ", message);

                           pending::lookups::check( state);

                           // we need to reply to the caller that instigated the discovery, if the 
                           // context is direct  (and the lookup did not find what the caller wants, via check::pending::lookups)
                           if( auto found = algorithm::find( state.pending.lookups, message.correlation); found && found->context == decltype( found->context)::direct)
                           {
                              auto pending = algorithm::container::extract( state.pending.lookups, std::begin( found));
                              auto reply = common::message::reverse::type( pending);
                              communication::device::blocking::optional::send( pending.process.ipc, reply);
                           }
                           
                        };
                     }
                  } // discover
               } // domain 
               namespace configuration
               {
                  auto request( State& state)
                  {
                     return [&state]( casual::configuration::message::Request& message)
                     {
                        Trace trace{ "handle::domain::handle::local::configuration::request"};
                        common::log::line( verbose::log, "message: ", message);

                        auto reply = common::message::reverse::type( message);

                        reply.model.queue = transform::configuration( state);

                        communication::device::blocking::optional::send( message.process.ipc, reply);
                     };

                  }

               } // configuration
            } // <unnamed>
         } // local

         namespace comply
         {
            void configuration( State& state, casual::configuration::model::queue::Model model)
            {
               Trace trace{ "queue::manager::handle::comply::configuration"};
               log::line( verbose::log, "model: ", model);

               // TODO maintainence: runtime configuration

               state.note = model.note;

               state.groups = algorithm::transform( model.groups, []( auto& config){ return state::Group{ std::move( config)};});
               state.forward.groups = algorithm::transform( model.forward.groups, []( auto& config){ return state::forward::Group{ std::move( config)};});

               auto spawn = []( auto& entity)
               {
                  entity.process.pid = common::process::spawn( state::entity::path( entity), {});
                  entity.state = decltype( entity.state())::spawned;
               };

               algorithm::for_each( state.groups, spawn);
               algorithm::for_each( state.forward.groups, spawn);
               
            }
         } // comply

         void abort( State& state)
         {
            Trace trace{ "queue::manager::handle::abort"};
            log::line( verbose::log, "state: ", state);

            state.runlevel = decltype( state.runlevel())::error;

            local::pending::lookups::discard( state);

            auto terminate = [&]( auto& entity)
            {
               signal::send( entity.process.pid, code::signal::terminate);
            };

            algorithm::for_each( state.forward.groups, terminate);
            algorithm::for_each( state.groups, terminate);

         }
      } // handle


      handle::dispatch_type handlers( State& state)
      {
         return common::message::dispatch::handler( ipc::device(),
            common::message::handle::defaults( ipc::device()),
            
            handle::local::group::connect( state),
            handle::local::group::configuration::update::reply( state),

            handle::local::forward::connect( state),
            handle::local::forward::configuration::update::reply( state),

            handle::local::configuration::request( state),
            
            handle::local::lookup::request( state),
            handle::local::lookup::discard::request( state),
            
            handle::local::advertise( state),
            
            handle::local::domain::discover::request( state),
            handle::local::domain::discover::reply( state),

            handle::local::shutdown::request( state),
            common::event::listener( 
               handle::local::event::process::exit( state),
               handle::local::event::discoverable::available( state)
            ),

            common::server::handle::admin::Call{
               manager::admin::services( state)}
         );
      }

   } // queue::manager
} // casual

