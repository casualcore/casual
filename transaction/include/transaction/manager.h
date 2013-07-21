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
      class Manager
      {
      public:


         Manager( const std::vector< std::string>& arguments);

         void start();

      private:

         struct Pending
         {
            typedef common::ipc::message::Transport::queue_key_type queue_key_type;

            queue_key_type target;
            common::message::transaction::Reply reply;

         };

         common::ipc::receive::Queue& m_receiveQueue;
         database m_database;

         std::vector< Pending> m_pendingReplies;

         static std::string databaseFileName();

      };

   } // transaction
} // casual



#endif /* MONITOR_H_ */
