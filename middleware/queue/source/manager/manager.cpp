//!
//! casual
//!

#include "queue/manager/manager.h"
#include "queue/manager/admin/server.h"
#include "queue/manager/handle.h"

#include "queue/common/log.h"
#include "queue/common/environment.h"


#include "common/message/dispatch.h"
#include "common/message/handle.h"
#include "common/algorithm.h"
#include "common/process.h"
#include "common/environment.h"
#include "common/exception/signal.h"
#include "common/exception/handle.h"

#include "configuration/message/transform.h"
#include "configuration/queue.h"

#include <fstream>


namespace casual
{

   namespace queue
   {
      namespace manager
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


               struct Startup : manager::handle::Base
               {
                  using manager::handle::Base::Base;

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

                     common::algorithm::transform( group.queues, reply.queues, transform::Queue{});

                     ipc::device().blocking_send( request.process.queue, reply);

                     return result;
                  }

               };

               void startup( State& state, common::message::domain::configuration::Domain&& config)
               {
                  casual::common::algorithm::transform( config.queue.groups, state.groups, Startup( state));

                  //
                  // Make sure all groups are up and running before we continue
                  //
                  {
                     auto handler = ipc::device().handler(
                        manager::handle::connect::Request{ state});

                     const auto filter = handler.types();

                     while( ! common::algorithm::all_of( state.groups, std::mem_fn(&State::Group::connected)))
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
               log << "queue manager start" << std::endl;

               auto handler = manager::handlers( state);

               common::log::category::information << "casual-queue-manager is on-line" << std::endl;


               while( true)
               {
                  handler( ipc::device().blocking_next());
               }

            }

         } // message

         std::vector< common::message::queue::information::queues::Reply> queues( State& state)
         {
            Trace trace( "manager::queues");

            auto send = [&]( const manager::State::Group& group)
               {
                  common::message::queue::information::queues::Request request;
                  request.process = common::process::handle();
                  return ipc::device().blocking_send( group.process.queue, request);
               };

            std::vector< common::Uuid> correlations;

            common::algorithm::transform( state.groups, correlations, send);


            auto receive = [&]( const common::Uuid& correlation)
               {
                  common::message::queue::information::queues::Reply reply;
                  ipc::device().blocking_receive( reply, correlation);
                  return reply;
               };

            std::vector< common::message::queue::information::queues::Reply> replies;

            common::algorithm::transform( correlations, replies, receive);

            return replies;
         }

         common::message::queue::information::messages::Reply messages( State& state, const std::string& queue)
         {
            common::message::queue::information::messages::Reply result;

            auto found = common::algorithm::find( state.queues, queue);

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

      } // manager





      Broker::Broker( manager::Settings settings) : m_state{ manager::local::transform::state( settings)}
      {
         Trace trace( "queue::Broker::Broker");


         //
         // Set environment variable so children can find us easy
         //
         common::environment::variable::process::set(
               common::environment::variable::name::ipc::queue::manager(),
               common::process::handle());


         {
            //
            // We ask the domain manager for configuration
            //

            common::message::domain::configuration::Request request;
            request.process = common::process::handle();

            manager::local::startup( m_state,
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
                  std::bind( &manager::handle::process::Exit::apply, manager::handle::process::Exit{ m_state}, std::placeholders::_1),
                  m_state.processes());

            common::log::category::information << "casual-queue-manager is off-line" << std::endl;

         }
         catch( const common::exception::signal::Timeout& exception)
         {
            auto pids = m_state.processes();
            common::log::category::error << "failed to terminate groups - pids: " << common::range::make( pids) << std::endl;
         }
         catch( ...)
         {
            common::exception::handle();
         }

      }

      void Broker::start()
      {
         log << "qeueue manager start" << std::endl;

         auto handler = manager::handlers( m_state);

         //
         // Connect to domain
         //
         common::process::instance::connect( common::process::instance::identity::queue::manager());

         common::log::category::information << "casual-queue-manager is on-line" << std::endl;


         while( true)
         {
            handler( manager::ipc::device().blocking_next());
         }
      }
   } // queue

} // casual
