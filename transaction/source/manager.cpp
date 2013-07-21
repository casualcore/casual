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

using namespace casual::common;

namespace casual
{
   namespace transaction
   {

      namespace handle
      {

         struct Begin
         {
            typedef message::transaction::Begin message_type;

            void dispatch( message_type& message)
            {

            }
         };

         struct Commit
         {
            typedef message::transaction::Commit message_type;

            void dispatch( message_type& message)
            {

            }
         };

         struct Rollback
         {
            typedef message::transaction::Rollback message_type;

            void dispatch( message_type& message)
            {

            }
         };


      }

      Manager::Manager(const std::vector<std::string>& arguments) :
          m_receiveQueue( ipc::getReceiveQueue()),
          m_database( databaseFileName())
      {
         //
         // TODO: Use a correct argument handler
         //
         const std::string name = ! arguments.empty() ? arguments.front() : std::string("");
         common::environment::setExecutablePath( name);

         common::Trace trace( "transaction::Manager::Manager");


         //
         // Notify the broker about us...
         //
         message::transaction::Connect message;

         message.path = name;
         message.server.queue_key = m_receiveQueue.getKey();
         message.server.pid = platform::getProcessId();

         queue::blocking::Writer writer( ipc::getBrokerQueue());
         writer(message);
      }

      void Manager::start()
      {
         common::Trace trace( "transaction::Manager::start");

         //
         // prepare message dispatch handlers...
         //

         message::dispatch::Handler handler;

         handler.add< handle::Begin>();
         handler.add< handle::Commit>();
         handler.add< handle::Rollback>();


         queue::blocking::Reader queueReader( m_receiveQueue);

         while( true)
         {
            auto marshal = queueReader.next();

            if( ! handler.dispatch( marshal))
            {
               common::logger::error << "message_type: " << marshal.type() << " not recognized - action: discard";
            }

         }

      }

      std::string Manager::databaseFileName()
      {
         return common::environment::getRootPath() + "/transaction-manager.db";
      }

   } // transaction
} // casual


