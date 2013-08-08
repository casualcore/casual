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
            typedef common::platform::queue_id_type queue_id_type;

            queue_id_type target;
            common::message::transaction::reply::Generic reply;
         };
      } // pending

      namespace resource
      {
         struct Proxy
         {
            struct Instance
            {
               enum class State
               {
                  absent,
                  started,
                  startupError,
                  idle,
                  busy
               };

               common::message::server::Id id;
               State state = State::absent;
            };


            std::string key;
            std::string openinfo;
            std::string closeinfo;
            std::size_t instances;

            std::vector< Instance> servers;
         };

      } // resource

      namespace action
      {
         struct Base
         {
            virtual ~Base() = default;
            virtual void apply() = 0;
         };

         namespace when
         {
            struct Persistent
            {

            };

         } // when

      } // action


      namespace filter
      {
         struct Instance
         {


            Instance( common::platform::pid_type pid)
                  : pid( pid)
            {
            }

            bool operator () ( const resource::Proxy::Instance& instance) const
            {
               return instance.id.pid == pid;
            }

            common::platform::pid_type pid;
         };
      } // filter


      struct State
      {
         State( const std::string& db);

         std::vector< pending::Reply> pendingReplies;
         sql::database::Connection db;

         std::vector< resource::Proxy> resources;

         std::map< std::string, config::xa::Switch> resourceMapping;

         std::vector< std::reference_wrapper< resource::Proxy::Instance>> find( common::platform::pid_type pid);

         std::vector< common::platform::pid_type> spawned;

         std::vector< resource::Proxy>::iterator findResource( common::platform::pid_type pid)
         {
            return std::find_if( std::begin( resources), std::end( resources),
             [=]( const resource::Proxy& proxy)
             {
               return std::any_of( std::begin( proxy.servers), std::end( proxy.servers), filter::Instance( pid));
             }
            );


         }

      };





      void configureResurceProxies( State& state);


      class Manager
      {
      public:


         Manager( const std::vector< std::string>& arguments);

         ~Manager();

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
