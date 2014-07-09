//!
//! broker.cpp
//!
//! Created on: Jun 20, 2014
//!     Author: Lazan
//!

#include "queue/broker/broker.h"


#include "queue/broker/handle.h"
#include "queue/environment.h"

#include "common/message/dispatch.h"
#include "common/algorithm.h"
#include "common/ipc.h"
#include "common/process.h"
#include "common/environment.h"
#include "common/exception.h"


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

                  common::log::debug << "writing queue broker queue file: " << path << std::endl;

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


               struct Startup : broker::handle::Base
               {
                  using broker::handle::Base::Base;

                  State::Server operator () ( const config::queue::Group& group)
                  {
                     State::Server server;

                     server.id.pid = casual::common::process::spawn(
                        casual::common::environment::directory::casual() + "/casual-queue-server",
                        {
                          "--queuebase", group.queuebase
                        });


                     broker::queue::blocking::Reader read{ common::ipc::receive::queue(), m_state};
                     common::message::queue::connect::Request request;
                     read( request);

                     common::message::queue::connect::Reply reply;
                     reply.name = group.name;

                     for( auto&& queue : group.queues)
                     {
                        reply.queues.emplace_back( queue.name, queue.retries);
                     }

                     broker::queue::blocking::Writer write{ request.server.queue_id, m_state};
                     write( reply);

                     return server;
                  }

               };

               void startup( State& state, config::queue::Queues config)
               {
                  casual::common::range::transform( config.groups, state.servers, Startup( state));
               }

            } // <unnamed>
         } // local
      } // broker


      Broker::Broker( broker::Settings settings)
       : m_queueFilePath( environment::broker::queue::path())
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


      void Broker::start()
      {

         casual::common::message::dispatch::Handler handler;

         handler.add( broker::handle::lookup::Request{ m_state});

         broker::queue::blocking::Reader blockedRead( casual::common::ipc::receive::queue(), m_state);

         while( true)
         {
            handler.dispatch( blockedRead.next());
         }

      }

   } // queue

} // casual
