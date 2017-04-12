//!
//! casual
//!

#include "queue/broker/broker.h"
#include "queue/common/log.h"


#include "queue/broker/handle.h"
#include "queue/common/environment.h"
#include "queue/broker/admin/server.h"


#include "common/message/dispatch.h"
#include "common/message/handle.h"
#include "common/algorithm.h"
#include "common/process.h"
#include "common/environment.h"
#include "common/exception.h"

#include "configuration/message/transform.h"
#include "configuration/queue.h"

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
                     common::message::queue::Queue operator() ( const common::message::domain::configuration::queue::Queue& value) const
                     {
                        common::message::queue::Queue result;

                        result.name = value.name;
                        result.retries = value.retries;

                        return result;
                     }
                  };
               } // transform


               struct Startup : broker::handle::Base
               {
                  using broker::handle::Base::Base;

                  State::Group operator () ( const common::message::domain::configuration::queue::Group& group)
                  {
                     State::Group result;
                     result.name = group.name;
                     result.queuebase = group.queuebase;

                     result.process.pid = casual::common::process::spawn(
                        m_state.group_executable,
                        { "--queuebase", group.queuebase, "--name", group.name});

                     common::message::queue::connect::Request request;
                     ipc::device().blocking_receive( request);

                     result.process.queue = request.process.queue;

                     common::message::queue::connect::Reply reply;
                     reply.name = group.name;

                     common::range::transform( group.queues, reply.queues, transform::Queue{});

                     ipc::device().blocking_send( request.process.queue, reply);

                     return result;
                  }

               };

               void startup( State& state, common::message::domain::configuration::Domain&& config)
               {
                  casual::common::range::transform( config.queue.groups, state.groups, Startup( state));

                  //
                  // Make sure all groups are up and running before we continue
                  //
                  {
                     auto handler = ipc::device().handler(
                        broker::handle::connect::Request{ state});

                     const auto filter = handler.types();

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
               log << "qeueue broker start" << std::endl;

               auto handler = broker::handlers( state);

               common::log::category::information << "casual-queue-broker is on-line" << std::endl;


               while( true)
               {
                  handler( ipc::device().blocking_next());
               }

            }

         } // message

         std::vector< common::message::queue::information::queues::Reply> queues( State& state)
         {
            Trace trace( "broker::queues");

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

            if( found && ! found->second.empty())
            {
               common::message::queue::information::messages::Request request;
               request.process = common::process::handle();
               request.qid = found->second.front().queue;

               ipc::device().blocking_receive(
                     result,
                     ipc::device().blocking_send( found->second.front().process.queue, request));
            }

            return result;
         }

      } // broker





      Broker::Broker( broker::Settings settings) : m_state{ broker::local::transform::state( settings)}
      {
         Trace trace( "queue::Broker::Broker");

         //
         // Connect to domain
         //
         common::process::instance::connect( common::process::instance::identity::queue::broker());

         common::environment::variable::process::set(
               common::environment::variable::name::ipc::queue::broker(),
               common::process::handle());


         {
            //
            // We ask the domain manager for configuration
            //

            common::message::domain::configuration::Request request;
            request.process = common::process::handle();

            broker::local::startup( m_state,
                  common::communication::ipc::call(
                        common::communication::ipc::domain::manager::device(), request).domain);
         }

      }

      Broker::~Broker()
      {
         Trace trace( "queue::Broker::~Broker");

         try
         {

            common::process::children::terminate(
                  std::bind( &broker::handle::process::Exit::apply, broker::handle::process::Exit{ m_state}, std::placeholders::_1),
                  m_state.processes());

            common::log::category::information << "casual-queue-broker is off-line" << std::endl;

         }
         catch( const common::exception::signal::Timeout& exception)
         {
            auto pids = m_state.processes();
            common::log::category::error << "failed to terminate groups - pids: " << common::range::make( pids) << std::endl;
         }
         catch( ...)
         {
            common::error::handler();
         }

      }

      void Broker::start()
      {
         log << "qeueue broker start" << std::endl;

         broker::message::pump( m_state);
      }
   } // queue

} // casual
