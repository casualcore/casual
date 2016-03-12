//!
//! broker.cpp
//!
//! Created on: Jun 20, 2014
//!     Author: Lazan
//!

#include "queue/broker/broker.h"


#include "queue/broker/handle.h"
#include "queue/common/environment.h"
#include "queue/broker/admin/server.h"

#include "common/server/handle.h"
#include "common/message/dispatch.h"
#include "common/message/handle.h"
#include "common/algorithm.h"
#include "common/process.h"
#include "common/environment.h"
#include "common/exception.h"
#include "common/internal/log.h"

#include "config/queue.h"

#include <fstream>

namespace casual
{

   namespace queue
   {
      namespace broker
      {

         namespace local
         {
            namespace
            {

               namespace transform
               {

                  State state( const Settings& settings)
                  {
                     State result;

                     if( settings.group_executable.empty())
                     {
                        result.group_executable = casual::common::environment::directory::casual() + "/bin/casual-queue-group";
                     }
                     else
                     {
                        result.group_executable = std::move( settings.group_executable);
                     }

                     return result;
                  }

                  struct Queue
                  {
                     common::message::queue::Queue operator() ( const config::queue::Queue& value) const
                     {
                        common::message::queue::Queue result;

                        result.name = value.name;
                        if( ! value.retries.empty())
                        {
                           result.retries = std::stoul( value.retries);
                        }
                        return result;
                     }
                  };
               } // transform


               struct Startup : broker::handle::Base
               {
                  using broker::handle::Base::Base;

                  State::Group operator () ( const config::queue::Group& group)
                  {
                     State::Group queueGroup;
                     queueGroup.name = group.name;
                     queueGroup.queuebase = group.queuebase;

                     queueGroup.process.pid = casual::common::process::spawn(
                        m_state.group_executable,
                        { "--queuebase", group.queuebase, "--name", group.name});

                     common::message::queue::connect::Request request;
                     ipc::device().blocking_receive( request);

                     queueGroup.process.queue = request.process.queue;

                     common::message::queue::connect::Reply reply;
                     reply.name = group.name;

                     common::range::transform( group.queues, reply.queues, transform::Queue{});

                     ipc::device().blocking_send( request.process.queue, reply);

                     return queueGroup;
                  }

               };

               void startup( State& state, config::queue::Domain config)
               {
                  casual::common::range::transform( config.groups, state.groups, Startup( state));

                  //
                  // Make sure all groups are up and running before we continue
                  //
                  {
                     casual::common::message::dispatch::Handler handler{
                        broker::handle::connect::Request{ state}};

                     auto filter = handler.types();

                     while( ! common::range::all_of( state.groups, std::mem_fn(&State::Group::connected)))
                     {
                        handler( ipc::device().blocking_next( filter));
                     }

                  }
               }

            } // <unnamed>
         } // local




         namespace message
         {
            void pump( State& state)
            {
               common::log::internal::queue << "qeueue broker start" << std::endl;

               casual::common::message::dispatch::Handler handler{
                  broker::handle::process::Exit{ state},
                  broker::handle::connect::Request{ state},
                  broker::handle::shutdown::Request{ state},
                  broker::handle::lookup::Request{ state},
                  broker::handle::group::Involved{ state},
                  broker::handle::transaction::commit::Request{ state},
                  broker::handle::transaction::commit::Reply{ state},
                  broker::handle::transaction::rollback::Request{ state},
                  broker::handle::transaction::rollback::Reply{ state},
                  //broker::handle::peek::queue::Request{ m_state},
                  common::server::handle::basic_admin_call{
                     ipc::device().device(),
                     broker::admin::services( state),
                     environment::broker::identification(),
                     ipc::device().error_handler()},
                  common::message::handle::ping(),
               };

               common::log::information << "casual-queue-broker is on-line" << std::endl;


               while( true)
               {
                  handler( ipc::device().blocking_next());
               }

            }

         } // message

         std::vector< common::message::queue::information::queues::Reply> queues( State& state)
         {
            common::trace::internal::Scope trace( "broker::queues", common::log::internal::queue);

            auto send = [&]( const broker::State::Group& group)
               {
                  common::message::queue::information::queues::Request request;
                  request.process = common::process::handle();
                  return ipc::device().blocking_send( group.process.queue, request);
               };

            std::vector< common::Uuid> correlations;

            common::range::transform( state.groups, correlations, send);


            auto receive = [&]( const common::Uuid& correlation)
               {
                  common::message::queue::information::queues::Reply reply;
                  ipc::device().blocking_receive( reply, correlation);
                  return reply;
               };

            std::vector< common::message::queue::information::queues::Reply> replies;

            common::range::transform( correlations, replies, receive);

            return replies;
         }

         common::message::queue::information::messages::Reply messages( State& state, const std::string& queue)
         {
            common::message::queue::information::messages::Reply result;

            auto found = common::range::find( state.queues, queue);

            if( found)
            {
               common::message::queue::information::messages::Request request;
               request.process = common::process::handle();
               request.qid = found->second.queue;

               ipc::device().blocking_receive(
                     result,
                     ipc::device().blocking_send( found->second.process.queue, request));
            }

            return result;
         }

      } // broker





      Broker::Broker( broker::Settings settings) : m_state{ broker::local::transform::state( settings)}
      {
         //
         // Will throw if another queue-broker is up and running.
         //
         common::server::handle::connect::reply(
               common::server::connect( environment::broker::identification()));



         if( ! settings.configuration.empty())
         {
            broker::local::startup( m_state, config::queue::get( settings.configuration));
         }
         else
         {
            broker::local::startup( m_state, config::queue::get());
         }

      }

      Broker::~Broker()
      {
         try
         {

            common::process::children::terminate(
                  std::bind( &broker::handle::process::Exit::apply, broker::handle::process::Exit{ m_state}, std::placeholders::_1),
                  m_state.processes());

            common::log::information << "casual-queue-broker is off-line" << std::endl;

         }
         catch( const common::exception::signal::Timeout& exception)
         {
            auto pids = m_state.processes();
            common::log::error << "failed to terminate groups - pids: " << common::range::make( pids) << std::endl;
         }
         catch( ...)
         {
            common::error::handler();
         }

      }

      void Broker::start()
      {
         common::log::internal::queue << "qeueue broker start" << std::endl;

         broker::message::pump( m_state);
      }
   } // queue

} // casual
