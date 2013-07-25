//!
//! monitor.h
//!
//! Created on: Jul 13, 2013
//!     Author: Lazan
//!

#ifndef MONITOR_H_
#define MONITOR_H_


#include "common/ipc.h"
#include "common/message.h"

#include "database/database.hpp"


//
// std
//
#include <string>

namespace casual
{
   namespace transaction
   {

      namespace pending
      {
         struct Reply
         {
            typedef common::platform::queue_key_type queue_key_type;

            queue_key_type target;
            common::message::transaction::Reply reply;
         };
      } // pending


      struct State
      {
         State( const std::string& db);

         std::vector< pending::Reply> pendingReplies;
         database db;
      };

      class Manager
      {
      public:


         Manager( const std::vector< std::string>& arguments);

         void start();

      private:



         void handlePending();

         common::ipc::receive::Queue& m_receiveQueue;
         State m_state;

         static std::string databaseFileName();

      };

   } // transaction
} // casual



#endif /* MONITOR_H_ */
