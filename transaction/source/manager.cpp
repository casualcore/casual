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


using namespace casual::common;

namespace casual
{
   namespace transaction
   {

      namespace local
      {


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
         message.serverId.queue_key = m_receiveQueue.getKey();
         message.serverId.pid = platform::getProcessId();

         queue::blocking::Writer writer( ipc::getBrokerQueue());
         writer(message);
      }

      void Manager::start()
      {
         common::Trace trace( "transaction::Manager::start");

      }

      std::string Manager::databaseFileName()
      {
         return common::environment::getRootPath() + "/transaction-manager.db";
      }

   } // transaction
} // casual


