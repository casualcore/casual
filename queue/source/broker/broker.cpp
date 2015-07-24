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
#include "common/ipc.h"
#include "common/process.h"
#include "common/environment.h"
#include "common/exception.h"
#include "common/internal/log.h"


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

               template< typename Q>
               void exportBrokerQueueKey( const Q& queue, const std::string& path)
               {
                  if( casual::common::file::exists( path))
                  {
                     //
                     // TODO: ping to see if there are another queue broker running
                     //
                     common::file::remove( path);
                  }

                  common::log::internal::queue << "writing queue broker queue file: " << path << std::endl;

                  std::ofstream brokerQueueFile( path);

                  if( brokerQueueFile)
                  {
                     brokerQueueFile << queue.id() << std::endl;
                     brokerQueueFile.close();
                  }
                  else
                  {
                     throw common::exception::NotReallySureWhatToNameThisException( "failed to write broker queue file: " + path);
                  }
               }

               namespace transform
               {
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
                        casual::common::environment::directory::casual() + "/bin/casual-queue-group",
                        { "--queuebase", group.queuebase, "--name", group.name});


                     broker::queue::blocking::Reader read{ common::ipc::receive::queue(), m_state};
                     common::message::queue::connect::Request request;
                     read( request);
                     queueGroup.process.queue = request.process.queue;

                     common::message::queue::connect::Reply reply;
                     reply.name = group.name;

                     common::range::transform( group.queues, reply.queues, transform::Queue{});


                     broker::queue::blocking::Writer write{ request.process.queue, m_state};
                     write( reply);

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

                     broker::queue::blocking::Reader read( casual::common::ipc::receive::queue(), state);

                     auto filter = handler.types();

                     while( ! common::range::all_of( state.groups, std::mem_fn(&State::Group::connected)))
                     {
                        handler( read.next( filter));
                     }

                  }
               }

            } // <unnamed>
         } // local


         std::vector< common::platform::pid_type> State::processes() const
         {
            std::vector< common::platform::pid_type> result;

            for( auto& group : groups)
            {
               result.push_back( group.process.pid);
            }

            return result;
         }

         void State::process( common::process::lifetime::Exit death)
         {
            {
               auto found = common::range::find_if( groups, [=]( const Group& g){
                  return g.process.pid == death.pid;
               });

               if( found)
               {
                  groups.erase( found.first);
               }
               else
               {
                  // error?
               }
            }

            //
            // Remove all queues for the group
            //
            {

               auto predicate = [=]( decltype( *queues.begin())& value){
                  return value.second.process.pid == death.pid;
               };

               auto range = common::range::make( queues);

               while( range)
               {
                  range = common::range::find_if( range, predicate);

                  if( range)
                  {
                     queues.erase( range.first);
                     range = common::range::make( queues);
                  }
               }
            }
            //
            // Invalidate xa-requests
            //
            {
               for( auto& corr : correlation)
               {
                  for( auto& reqeust : corr.second.requests)
                  {
                     if( reqeust.group.pid == death.pid && reqeust.state <= Correlation::State::pending)
                     {
                        reqeust.state = Correlation::State::error;
                     }
                  }
               }
            }
         }


      } // broker





      Broker::Broker( broker::Settings settings)
       : m_queueFilePath( common::process::singleton( environment::broker::queue::path()))
      {

         broker::local::exportBrokerQueueKey( casual::common::ipc::receive::queue(), m_queueFilePath);


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
            common::process::children::terminate( m_state);

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

         casual::common::message::dispatch::Handler handler{
            broker::handle::connect::Request{ m_state},
            broker::handle::shutdown::Request{ m_state},
            broker::handle::lookup::Request{ m_state},
            broker::handle::group::Involved{ m_state},
            broker::handle::transaction::commit::Request{ m_state},
            broker::handle::transaction::commit::Reply{ m_state},
            broker::handle::transaction::rollback::Request{ m_state},
            broker::handle::transaction::rollback::Reply{ m_state},
            //broker::handle::peek::queue::Request{ m_state},
            common::server::handle::basic_admin_call< broker::State>{
               broker::admin::Server::services( *this), m_state, common::process::instance::identity::queue::broker()},
            common::message::handle::ping( m_state),
         };

         broker::queue::blocking::Reader blockedRead( casual::common::ipc::receive::queue(), m_state);

         common::log::information << "casual-queue-broker is on-line" << std::endl;


         while( true)
         {
            handler( blockedRead.next());
         }

      }

      const broker::State& Broker::state() const
      {
         return m_state;
      }

      std::vector< common::message::queue::information::queues::Reply> Broker::queues()
      {
         common::trace::internal::Scope trace( "Broker::queues", common::log::internal::queue);

         std::vector< common::message::queue::information::queues::Reply> replies;

         common::queue::batch(
               broker::queue::blocking::Send{ m_state}, m_state.groups,
               broker::queue::blocking::Reader{ casual::common::ipc::receive::queue(), m_state}, replies,
                  []( const broker::State::Group& group)
                  {
                     common::message::queue::information::queues::Request request;
                     request.process = common::process::handle();
                     return std::make_tuple( group.process.queue, std::move( request));
                  });

         return replies;
      }

      common::message::queue::information::messages::Reply Broker::messages( const std::string& queue)
      {
         common::message::queue::information::messages::Reply result;

         auto found = common::range::find( m_state.queues, queue);

         if( found)
         {
            broker::queue::blocking::Send send{ m_state};

            common::message::queue::information::messages::Request request;
            request.process = common::process::handle();
            request.qid = found->second.queue;

            auto id = send( found->second.process.queue, request);


            broker::queue::blocking::Reader receive{ casual::common::ipc::receive::queue(), m_state};
            receive( result, id);
         }

         return result;
      }


   } // queue

} // casual
