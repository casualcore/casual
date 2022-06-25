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
#include "queue/common/ipc.h"


#include "domain/discovery/api.h"

#include "configuration/message.h"

#include "common/process.h"
#include "common/server/lifetime.h"
#include "common/server/handle/call.h"
#include "common/event/listen.h"
#include "common/event/send.h"
#include "common/algorithm/compare.h"
#include "common/message/handle.h"
#include "common/message/internal.h"
#include "common/communication/instance.h"

#include "common/code/raise.h"
#include "common/code/casual.h"

#include "casual/assert.h"





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

                           return predicate::boolean( ipc::flush::optional::send( lookup.process.ipc, reply));
                        }
                        return false;
                     };

                     algorithm::container::trim( state.pending.lookups, algorithm::remove_if( state.pending.lookups, lookup_replied));
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
                        ipc::flush::optional::send( lookup.process.ipc, reply);
                     };

                     algorithm::for_each( pending, discard_lookup);
                  }


               } // pending::lookups

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
                  namespace detail::dispatch::lookup
                  {
                     void absent_queue( State& state, queue::ipc::message::lookup::Request& message)
                     {
                        using Enum = decltype( message.context.semantic);

                        switch( message.context.semantic)
                        {
                           case Enum::direct:
                           {
                              auto reply = common::message::reverse::type( message);
                              reply.name = message.name;
                              ipc::flush::optional::send( message.process.ipc, reply);
                              return;
                           }
                           case Enum::wait:
                           {
                              // either we've sent a discovery or the lookup want's to wait (possible for ever).
                              state.pending.lookups.push_back( std::move( message));
                              return;
                           }
                        }
                        casual::terminate( "unknown value for message.context.semantic: ", cast::underlying( message.context.semantic));
                     }

                     void discovery( State& state, queue::ipc::message::lookup::Request& message)
                     {                     
                        Trace trace{ "queue::manager::handle::local::detail::dispatch::lookup::discovery"};
                        
                        casual::domain::discovery::request( {}, { message.name}, message.correlation);
                        state.pending.lookups.push_back( std::move( message));
                     }

                     void reply( State& state, const casual::queue::manager::state::Queue& queue, queue::ipc::message::lookup::Request& message)
                     {
                        auto reply = common::message::reverse::type( message);
                        reply.name = message.name;
                        reply.queue = queue.queue;
                        reply.process = queue.process;
                        reply.order = queue.order;
                        ipc::flush::optional::send( message.process.ipc, reply);
                     }

                     bool internal_only( State& state, queue::ipc::message::lookup::Request& message)
                     {
                        if( auto queue = state.local_queue( message.name))
                        {
                           dispatch::lookup::reply( state, *queue, message);
                           return true;
                        }

                        return false;
                     }

                     bool internal_external( State& state, queue::ipc::message::lookup::Request& message)
                     {
                        if( auto queue = state.queue( message.name))
                        {
                           dispatch::lookup::reply( state, *queue, message);
                           return true;
                        }

                        return false;
                     }

                  } // detail::dispatch::lookup

                  auto request( State& state)
                  {
                     return [&state]( queue::ipc::message::lookup::Request& message)
                     {
                        Trace trace{ "queue::manager::local::handle::lookup::request"};
                        common::log::line( verbose::log, "message: ", message);

                        using Enum = decltype( message.context.requester);

                        switch( message.context.requester)
                        {
                           case Enum::internal:
                              if( ! detail::dispatch::lookup::internal_only( state, message))
                                 detail::dispatch::lookup::discovery( state, message);
                              return;
                           case Enum::external:
                              if( ! detail::dispatch::lookup::internal_only( state, message))
                                 detail::dispatch::lookup::absent_queue( state, message);
                              return;
                           case Enum::external_discovery:
                              if( ! detail::dispatch::lookup::internal_external( state, message))
                                 detail::dispatch::lookup::absent_queue( state, message);
                              return;
                        }
                        casual::terminate( "unknown value for message.context.requester: ", cast::underlying( message.context.requester));
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

                           ipc::flush::optional::send( message.process.ipc, reply);
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
                           ipc::flush::optional::send( message.process.ipc, request);
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

               namespace domain::discover
               {
                  namespace internal
                  {
                     auto request( State& state)
                     {
                        return [&state]( casual::domain::message::discovery::internal::Request&& message)
                        {
                           Trace trace{ "queue::manager::handle::local::domain::discovery::internal::request"};
                           common::log::line( verbose::log, "message: ", message);

                           auto reply = common::message::reverse::type( message, common::process::handle());

                           auto is_local = [&state]( auto& name)
                           {
                              if( auto found = algorithm::find( state.queues, name))
                                 return algorithm::any_of( found->second, []( auto& instance){ return instance.local();});

                              return false;
                           };


                           for( auto& queue : algorithm::filter( message.content.queues, is_local))
                              reply.content.queues.emplace_back( std::move( queue));

                           common::log::line( verbose::log, "reply: ", reply);

                           ipc::flush::optional::send( message.process.ipc, reply);
                        };
                     }
                  } // internal

                  namespace api
                  {
                     auto reply( State& state)
                     {
                        return [&state]( casual::domain::message::discovery::api::Reply& message)
                        {
                           Trace trace{ "queue::manager::handle::local::domain::discovery::api::reply"};
                           common::log::line( verbose::log, "message: ", message);

                           pending::lookups::check( state);

                           // we need to reply to the caller that instigated the discovery, if the 
                           // context is direct  (and the lookup did not find what the caller wants, via check::pending::lookups)
                           if( auto found = algorithm::find( state.pending.lookups, message.correlation); found && found->context.semantic == decltype( found->context.semantic)::direct)
                           {
                              auto pending = algorithm::container::extract( state.pending.lookups, std::begin( found));
                              auto reply = common::message::reverse::type( pending);
                              ipc::flush::optional::send( pending.process.ipc, reply);
                           }
                           
                        };
                     }

                  } // api

                  namespace needs
                  { 
                     auto request( State& state)
                     {
                        return [&state]( casual::domain::message::discovery::needs::Request& message)
                        {
                           Trace trace{ "queue::manager::handle::local::domain::discover::needs::request"};
                           common::log::line( verbose::log, "message: ", message);

                           auto reply = common::message::reverse::type( message);

                           // add the pending _wait for ever_ requests
                           for( auto& pending : state.pending.lookups)
                              if( pending.context.semantic == decltype( pending.context.semantic)::wait)
                                 reply.content.queues.push_back( pending.name);
                           
                           algorithm::container::trim( reply.content.queues, algorithm::unique( algorithm::sort( reply.content.queues)));
                           common::log::line( verbose::log, "reply: ", reply);

                           ipc::flush::optional::send( message.process.ipc, reply);
                           
                        };
                     }
                  } // needs

                  namespace known
                  {
                     auto request( State& state)
                     {
                        return [&state]( casual::domain::message::discovery::known::Request& message)
                        {
                           Trace trace{ "queue::manager::handle::local::domain::discover::known::request"};
                           common::log::line( verbose::log, "message: ", message);

                           auto reply = common::message::reverse::type( message);


                           // all known "remote" queues
                           reply.content.queues = algorithm::accumulate( state.queues, std::move( reply.content.queues), []( auto result, auto& pair)
                           {
                              auto is_remote = []( auto& queue){ return queue.remote();};
                              if( algorithm::find_if( std::get< 1>( pair), is_remote))
                                 result.push_back( std::get< 0>( pair));

                              return result;
                           });

                           // all 'wait for ever'
                           for( auto& pending : state.pending.lookups)
                              if( pending.context.semantic == decltype( pending.context.semantic)::wait)
                                 reply.content.queues.push_back( pending.name);

                           algorithm::container::trim( reply.content.queues, algorithm::unique( algorithm::sort( reply.content.queues)));

                           common::log::line( verbose::log, "reply: ", reply);

                           ipc::flush::optional::send( message.process.ipc, reply);
                           
                        };
                     }
                     
                  } // known

               } // domain::discover

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

                        ipc::flush::optional::send( message.process.ipc, reply);
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
            common::message::internal::dump::state::handle( state),
            
            handle::local::group::connect( state),
            handle::local::group::configuration::update::reply( state),

            handle::local::forward::connect( state),
            handle::local::forward::configuration::update::reply( state),

            handle::local::configuration::request( state),
            
            handle::local::lookup::request( state),
            handle::local::lookup::discard::request( state),
            
            handle::local::advertise( state),
            
            handle::local::domain::discover::internal::request( state),
            handle::local::domain::discover::api::reply( state),
            handle::local::domain::discover::needs::request( state),
            handle::local::domain::discover::known::request( state),

            handle::local::shutdown::request( state),
            common::event::listener( handle::local::event::process::exit( state)),

            common::server::handle::admin::Call{
               manager::admin::services( state)}
         );
      }

   } // queue::manager
} // casual

