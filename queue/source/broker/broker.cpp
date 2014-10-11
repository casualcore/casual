//!
//! broker.cpp
//!
//! Created on: Jun 20, 2014
//!     Author: Lazan
//!

#include "queue/broker/broker.h"


#include "queue/broker/handle.h"
#include "queue/environment.h"
#include "queue/broker/admin/server.h"

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
                           result.name = std::stoul( value.retries);
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

                     queueGroup.id.pid = casual::common::process::spawn(
                        casual::common::environment::directory::casual() + "/bin/casual-queue-group",
                        { "--queuebase", group.queuebase });


                     broker::queue::blocking::Reader read{ common::ipc::receive::queue(), m_state};
                     common::message::queue::connect::Request request;
                     read( request);
                     queueGroup.id.queue_id = request.server.queue_id;

                     common::message::queue::connect::Reply reply;
                     reply.name = group.name;

                     common::range::transform( group.queues, reply.queues, transform::Queue{});


                     broker::queue::blocking::Writer write{ request.server.queue_id, m_state};
                     write( reply);

                     return queueGroup;
                  }

               };

               void startup( State& state, config::queue::Domain config)
               {
                  casual::common::range::transform( config.groups, state.groups, Startup( state));
               }

            } // <unnamed>
         } // local


         std::vector< common::platform::pid_type> State::processes() const
         {
            std::vector< common::platform::pid_type> result;

            for( auto& group : groups)
            {
               result.push_back( group.id.pid);
            }

            return result;
         }

         void State::removeProcess( common::platform::pid_type pid)
         {
            {
               auto found = common::range::find_if( groups, [=]( const Group& g){
                  return g.id.pid == pid;
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
                  return value.second.server.pid == pid;
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
                     if( reqeust.group.id.pid == pid && reqeust.state <= Correlation::State::pending)
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
            broker::handle::lookup::Request{ m_state},
            broker::handle::group::Involved{ m_state},
            broker::handle::transaction::commit::Request{ m_state},
            broker::handle::transaction::commit::Reply{ m_state},
            broker::handle::transaction::rollback::Request{ m_state},
            broker::handle::transaction::rollback::Reply{ m_state},
            broker::handle::peek::queue::Request{ m_state},
            common::callee::handle::basic_admin_call< broker::State>{ broker::admin::Server::services( *this), m_state},
            common::message::handle::ping( m_state),
         };

         broker::queue::blocking::Reader blockedRead( casual::common::ipc::receive::queue(), m_state);

         common::log::information << "casual-queue-broker is on-line" << std::endl;


         while( true)
         {
            handler.dispatch( blockedRead.next());
         }

      }

      const broker::State& Broker::state() const
      {
         return m_state;
      }

      std::vector< broker::Queues> Broker::queues( std::vector< std::string> groups)
      {
         common::trace::internal::Scope trace( "Broker::queues", common::log::internal::queue);

         if( groups.empty())
         {
            return getQueues( m_state.groups);
         }
         else
         {

            return getQueues( common::range::intersection(
               m_state.groups,
               common::range::unique( groups),
               common::compare::equal_to( std::mem_fn( &broker::State::Group::name), common::compare::value())));
         }
      }

      template< typename R>
      std::vector< broker::Queues> Broker::getQueues( R&& range)
      {
         common::trace::internal::Scope trace( "Broker::getQueues", common::log::internal::queue);

         std::vector< broker::Queues> result;

         for( auto&& group : range)
         {
            common::log::internal::queue << "request to: " << group.id << std::endl;

            broker::queue::blocking::Writer blockedWrite( group.id.queue_id, m_state);

            common::message::queue::information::queues::Request request;
            request.server = common::message::server::Id::current();

            blockedWrite( request);

            broker::Queues queues;
            queues.group = group;
            result.push_back( std::move( queues));
         }

         for( auto count = result.size(); count != 0; --count)
         {
            broker::queue::blocking::Reader blockedRead( casual::common::ipc::receive::queue(), m_state);

            common::message::queue::information::queues::Reply reply;
            blockedRead( reply);

            auto found = common::range::find_if( result,
                  common::compare::equal_to( std::mem_fn( &broker::Queues::groupId), common::bind::value( reply.server.pid)));

            if( found)
            {
               common::range::copy( reply.queues, std::back_inserter( found->queues));
            }
         }

         return result;

      }

   } // queue

} // casual
