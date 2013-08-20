//!
//! monitor.cpp
//!
//! Created on: Jul 15, 2013
//!     Author: Lazan
//!

#include "transaction/manager.h"
#include "transaction/manager_handle.h"
#include "transaction/manager_action.h"


#include "common/message.h"
#include "common/trace.h"
#include "common/queue.h"
#include "common/environment.h"
#include "common/message_dispatch.h"

#include "config/domain.h"

#include "sf/archive_maker.h"


#include <tx.h>

using namespace casual::common;

namespace casual
{
   namespace transaction
   {



      namespace local
      {
         namespace
         {
            void createTables( sql::database::Connection& db)
            {
               db.execute( R"( CREATE TABLE IF NOT EXISTS trans (
                     gtrid         BLOB,
                     bqual         BLOB,
                     pid           NUMBER,
                     state         NUMBER,
                     started       NUMBER,
                     PRIMARY KEY (gtrid, bqual)); )");
            }



            struct ScopedWrite : public state::Base
            {
               ScopedWrite( State& state) : Base( state)
               {
                  // TODO: error checking?
                  m_state.db.begin();
               }

               ~ScopedWrite()
               {
                  m_state.db.commit();
               }
            };



         } // <unnamed>
      } // local


      namespace action
      {
         namespace boot
         {
            struct Proxie : state::Base
            {
               using state::Base::Base;

               void operator () ( const std::shared_ptr< state::resource::Proxy>& proxy)
               {
                  for( auto index = proxy->concurency; index > 0; --index)
                  {
                     auto& info = m_state.xaConfig.at( proxy->key);

                     auto instance = std::make_shared< state::resource::Proxy::Instance>();

                     instance->id.pid = process::spawn(
                           info.server,
                           {
                                 "--tm-queue", std::to_string( ipc::getReceiveQueue().id()),
                                 "--rm-key", info.key,
                                 "--rm-openinfo", proxy->openinfo,
                                 "--rm-closeinfo", proxy->closeinfo
                           }
                        );

                     m_state.instances.emplace( instance->id.pid, instance);
                     instance->proxy = proxy;

                     instance->state = state::resource::Proxy::Instance::State::started;

                     proxy->instances.emplace_back( std::move( instance));
                  }
               }
            };
         } // boot
      } // action




      void startResurceProxies( State& state)
      {
         common::trace::Exit log( "transaction manager start resource proxies");


      }


      State::State( const std::string& database) : db( database) {}


      Manager::Manager( const Settings& settings) :
          m_receiveQueue( ipc::getReceiveQueue()),
          m_state( settings.database)
      {
         common::trace::Exit trace( "transaction manager startup");



         local::createTables( m_state.db);

         state::configurate( m_state);
      }

      Manager::~Manager()
      {
         try
         {
            common::trace::Exit temp( "terminate child processes");

            //
            // We need to terminate all children
            //
            /*
            for( auto& resource : m_state.resources)
            {
               for( auto& instances : resource.servers)
               {
                  logger::information << "terminate: " << instances.id.pid;
                  process::terminate( instances.id.pid);
               }
            }
             */

            for( auto death : process::lifetime::ended())
            {
               logger::information << "shutdown: " << death.string();
            }

         }
         catch( ...)
         {
            common::error::handler();
         }

      }

      void Manager::start()
      {
         common::Trace trace( "transaction::Manager::start");

         //
         // Start the rm-proxy-servers
         //
         {
            common::trace::Exit trace( "transaction manager start rm-proxy-servers");

            std::for_each(
               std::begin( m_state.resources),
               std::end( m_state.resources),
               action::boot::Proxie( m_state));
         }

         {
            common::trace::Exit trace( "transaction manager notify broker");

            //
            // Notify the broker about us...
            //
            message::transaction::Connect message;

            message.path = common::environment::file::executable();
            message.server.queue_id = m_receiveQueue.id();

            queue::blocking::Writer writer( ipc::getBrokerQueue());
            writer(message);
         }


         //
         // prepare message dispatch handlers...
         //

         message::dispatch::Handler handler;

         handler.add< handle::Begin>( m_state);
         handler.add< handle::Commit>( m_state);
         handler.add< handle::Rollback>( m_state);


         QueueBlockingReader queueReader( m_receiveQueue, m_state);

         while( true)
         {
            {
               local::ScopedWrite batchWrite( m_state);

               //
               // Blocking
               //
               auto marshal = queueReader.next();

               if( ! handler.dispatch( marshal))
               {
                  common::logger::error << "message_type: " << marshal.type() << " not recognized - action: discard";
               }

               //
               // Consume until the queue is empty or we've pending replies equal to transaction_batch
               //
               {

                  QueueNonBlockingReader nonBlocking( m_receiveQueue, m_state);


                  for( auto marshler = nonBlocking.next(); ! marshler.empty(); marshler = nonBlocking.next())
                  {

                     if( ! handler.dispatch( marshler.front()))
                     {
                        common::logger::error << "message_type: " << marshal.type() << " not recognized - action: discard";
                     }

                     if( m_state.pendingReplies.size() >=  common::platform::transaction_batch)
                     {
                        break;
                     }
                  }
               }

               std::for_each(
                  std::begin( m_state.pendingReplies),
                  std::end( m_state.pendingReplies),
                  action::Send< QueueBlockingWriter>{ m_state});

               m_state.pendingReplies.clear();

            }
         }
      }


   } // transaction
} // casual


