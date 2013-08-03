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

#include "config/xa_switch.h"


#include "sql/database.h"


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

      namespace resource
      {
         struct Proxy
         {
            struct Instance
            {
               common::message::server::Id id;
               bool idle = true;
            };


            std::string key;
            std::string openinfo;
            std::string closeinfo;
            std::size_t instances;

            std::vector< Instance> servers;
         };

      } // resource

      struct State
      {
         State( const std::string& db);

         std::vector< pending::Reply> pendingReplies;
         sql::database::Connection db;

         std::vector< resource::Proxy> resources;

         std::map< std::string, config::xa::Switch> resourceMapping;
      };


      void configureResurceProxies( State& state);


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
