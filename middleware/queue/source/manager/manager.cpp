//!
//! casual
//!

#include "queue/manager/manager.h"
#include "queue/manager/admin/server.h"
#include "queue/manager/handle.h"

#include "queue/common/log.h"


#include "common/message/dispatch.h"
#include "common/message/handle.h"
#include "common/algorithm.h"
#include "common/process.h"
#include "common/environment.h"
#include "common/exception/signal.h"
#include "common/exception/handle.h"
#include "common/event/send.h"

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
                     Trace trace( "queue::manager::local::transform::state");

                     State result;

                     result.group_executable = common::coalesce(
                        std::move( settings.group_executable),
                        casual::common::environment::directory::casual() + "/bin/casual-queue-group"
                     );

                     //
                     // We ask the domain manager for configuration
                     //
                     {
                        common::message::domain::configuration::Request request;
                        request.process = common::process::handle();

                        result.configuration = common::communication::ipc::call(
                           common::communication::ipc::domain::manager::device(), request).domain.queue;
                     }

                     return result;
                  }
               } // transform


               struct Spawn : manager::handle::Base
               {
                  using manager::handle::Base::Base;

                  State::Group operator () ( const common::message::domain::configuration::queue::Group& group)
                  {
                     Trace trace( "queue::manager::local::Spawn");

                     State::Group result;
                     result.name = group.name;
                     result.queuebase = group.queuebase;

                     try
                     {
                        result.process.pid = casual::common::process::spawn(
                           m_state.group_executable,
                           { "--queuebase", group.queuebase, "--name", group.name});
                     }
                     catch( const common::exception::base& exception)
                     {
                        auto message = common::string::compose( "failed to spawn queue group:  ", group.name, " - ", exception);
                        
                        common::log::line( common::log::category::error, message);
                        common::event::error::send( message, common::event::error::Severity::error);
                     }

                     return result;
                  }
               };



               void startup( State& state)
               {
                  Trace trace( "queue::manager::local::startup");
                  
                  casual::common::algorithm::transform( state.configuration.groups, state.groups, Spawn( state));

                  common::algorithm::trim( state.groups, common::algorithm::remove_if( state.groups, []( auto& g){
                     return ! g.process.pid;
                  }));
            

                  //
                  // Make sure all groups are up and running before we continue
                  //
                  {
                     auto handler = ipc::device().handler(
                        manager::handle::connect::Request{ state},
                        manager::handle::connect::Information{ state},
                        manager::handle::process::Exit{ state},
                        common::message::handle::Shutdown{}
                     );

                     const auto filter = handler.types();

                     while( ! common::algorithm::all_of( state.groups, std::mem_fn(&State::Group::connected)))
                     {
                        handler( ipc::device().blocking_next( filter));
                     }
                  }
               }

            } // <unnamed>
         } // local


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

      }

      Broker::~Broker()
      {
         Trace trace( "queue::Broker::~Broker");

         try
         {

            common::process::children::terminate(
                  std::bind( &manager::handle::process::Exit::apply, manager::handle::process::Exit{ m_state}, std::placeholders::_1),
                  m_state.processes());

            common::log::line( common::log::category::information, "casual-queue-manager is off-line");

         }
         catch( const common::exception::signal::Timeout& exception)
         {
            auto pids = m_state.processes();
            common::log::line( common::log::category::error, "failed to terminate groups - pids: ", pids);
         }
         catch( ...)
         {
            common::exception::handle();
         }

      }

      void Broker::start()
      {
         common::log::line( log, "queue manager start");

         manager::local::startup( m_state);

         auto handler = manager::handlers( m_state);

         //
         // Connect to domain
         //
         common::process::instance::connect( common::process::instance::identity::queue::manager());

         common::log::line( common::log::category::information, "casual-queue-manager is on-line");

         while( true)
         {
            handler( manager::ipc::device().blocking_next());
         }
      }
   } // queue

} // casual
