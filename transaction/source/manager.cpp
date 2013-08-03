//!
//! monitor.cpp
//!
//! Created on: Jul 15, 2013
//!     Author: Lazan
//!

#include "transaction/manager.h"


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

      namespace handle
      {

         struct Base
         {
            Base( State& state) : m_state( state) {}

         protected:
            State& m_state;

         };

         struct Begin : public Base
         {
            typedef message::transaction::Begin message_type;

            // TODO: using Base::Base;
            Begin( State& state) : Base( state) {}

            void dispatch( message_type& message)
            {
               long state = 0;
               auto started = std::chrono::time_point_cast<std::chrono::microseconds>(message.start).time_since_epoch().count();
               auto xid = common::transform::xid( message.xid);

               const std::string sql{ R"( INSERT INTO trans VALUES (?,?,?,?,?); )"};

               pending::Reply reply;
               reply.target = message.server.queue_id;

               m_state.db.execute( sql, std::get< 0>( xid), std::get< 1>( xid), message.server.pid, state, started);

               m_state.pendingReplies.push_back( std::move( reply));

            }
         };

         struct Commit : public Base
         {
            typedef message::transaction::Commit message_type;

            // TODO: using Base::Base;
            Commit( State& state) : Base( state) {}

            void dispatch( message_type& message)
            {

            }
         };

         struct Rollback : public Base
         {
            typedef message::transaction::Rollback message_type;

            // TODO: using Base::Base;
            Rollback( State& state) : Base( state) {}

            void dispatch( message_type& message)
            {

            }
         };

      } // handle

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


            struct ScopedTransaction : public handle::Base
            {
               ScopedTransaction( State& state) : Base( state)
               {
                  // TODO: error checking?
                  //m_state.db.begin();
               }

               ~ScopedTransaction()
               {
                  //m_state.db.commit();
               }
            };

            namespace filter
            {
               struct Resource
               {
                  bool operator () ( const config::domain::Group& value) const
                  {
                     return ! value.resource.key.empty();
                  }
               };
            } // filter

            namespace transform
            {

               struct Group
               {
                  transaction::resource::Proxy operator () ( const config::domain::Group& value) const
                  {
                     transaction::resource::Proxy result;

                     result.key = value.resource.key;
                     result.openinfo = value.resource.openinfo;
                     result.closeinfo = value.resource.closeinfo;
                     result.instances = std::stoul( value.resource.instances);

                     return result;
                  }
               };

            } // transform


         } // <unnamed>
      } // local


      void configureResurceProxies( State& state)
      {

         {
            common::trace::Exit log( "transaction manager xa-switch configuration");

            auto resources = config::xa::switches::get();

            for( auto& resource : resources)
            {
               if( ! state.resourceMapping.emplace( resource.key, std::move( resource)).second)
               {
                  throw exception::NotReallySureWhatToNameThisException( "multiple keys in resource config");
               }
            }
         }

         //
         // configure resources
         //
         {
            common::trace::Exit log( "transaction manager resource configuration");

            auto domain = config::domain::get();

            auto resourcesEnd = std::partition( std::begin( domain.groups), std::end( domain.groups), local::filter::Resource());

            std::transform(
               std::begin( domain.groups),
               resourcesEnd,
               std::back_inserter( state.resources),
               local::transform::Group());
         }

      }


      State::State( const std::string& database) : db( database) {}



      Manager::Manager(const std::vector<std::string>& arguments) :
          m_receiveQueue( ipc::getReceiveQueue()),
          m_state( databaseFileName())
      {
         common::trace::Exit trace( "transaction manager startup");

         //
         // TODO: Use a correct argument handler
         //
         const std::string name = ! arguments.empty() ? arguments.front() : std::string("");
         common::environment::file::executable( name);




         local::createTables( m_state.db);

         {
            common::trace::Exit trace( "transaction manager notify broker");

            //
            // Notify the broker about us...
            //
            message::transaction::Connect message;

            message.path = name;
            message.server.queue_id = m_receiveQueue.id();

            queue::blocking::Writer writer( ipc::getBrokerQueue());
            writer(message);
         }
      }

      void Manager::start()
      {
         common::Trace trace( "transaction::Manager::start");

         //
         // prepare message dispatch handlers...
         //

         message::dispatch::Handler handler;

         handler.add< handle::Begin>( m_state);
         handler.add< handle::Commit>( m_state);
         handler.add< handle::Rollback>( m_state);


         queue::blocking::Reader queueReader( m_receiveQueue);

         while( true)
         {
            {
               local::ScopedTransaction batchCommit( m_state);

               //
               // Blocking
               //
               auto marshal = queueReader.next();

               if( ! handler.dispatch( marshal))
               {
                  common::logger::error << "message_type: " << marshal.type() << " not recognized - action: discard";
               }

               //
               // Consume until the queue is empty or we've reach transaction_batch
               //
               {
                  auto count = common::platform::transaction_batch;

                  queue::non_blocking::Reader nonBlocking( m_receiveQueue);

                  for( auto marshler = nonBlocking.next(); ! marshler.empty(); marshler = nonBlocking.next())
                  {
                     if( ! handler.dispatch( marshler.front()))
                     {
                        common::logger::error << "message_type: " << marshal.type() << " not recognized - action: discard";
                     }

                     if( --count == 0)
                        break;
                  }
               }
            }

            //
            // Handle the pending replies
            //
            handlePending();
         }
      }

      namespace local
      {
         namespace
         {
            struct Send
            {
               bool operator () ( const pending::Reply& reply) const
               {
                  queue::ipc_wrapper< queue::non_blocking::Writer> writer( reply.target);
                  return writer( reply.reply);
               }

            };

            struct ReportError
            {
               void operator () ( const pending::Reply& reply) const
               {
                  logger::error << "failed to send transaction reply to queue: " << reply.target << " state" << reply.reply.state;
               }
            };

         }
      }

      void Manager::handlePending()
      {
         //
         // Try to send replies to callers
         //
         auto end = std::partition( std::begin( m_state.pendingReplies), std::end(  m_state.pendingReplies), local::Send());

         //
         // TODO: what should we do in case of failed replies?
         //
         std::for_each( std::begin(  m_state.pendingReplies), end, local::ReportError());

         m_state.pendingReplies.clear();
      }

      std::string Manager::databaseFileName()
      {
         return common::environment::directory::domain() + "/transaction-manager.db";
      }

   } // transaction
} // casual


