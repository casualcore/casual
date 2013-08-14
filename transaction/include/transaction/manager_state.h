//!
//! manager_state.h
//!
//! Created on: Aug 13, 2013
//!     Author: Lazan
//!

#ifndef MANAGER_STATE_H_
#define MANAGER_STATE_H_

#include "common/platform.h"
#include "common/message.h"

#include "config/xa_switch.h"

#include "sql/database.h"


#include <map>

namespace casual
{

   namespace transaction
   {
      namespace state
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

                  std::shared_ptr< Proxy> proxy;
                  common::message::server::Id id;
                  State state = State::absent;

               };


               std::string key;
               std::string openinfo;
               std::string closeinfo;
               std::size_t concurency;

               std::vector< std::shared_ptr< Instance>> instances;
            };

         } // resource

         namespace action
         {
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


      } // state

      struct State
      {
         State( const std::string& db);


         sql::database::Connection db;
         std::map< std::string, config::xa::Switch> xaConfig;

         std::vector< std::shared_ptr< state::resource::Proxy>> resources;
         std::map< common::platform::pid_type, std::shared_ptr< state::resource::Proxy::Instance>> instances;



         //!
         //! Replies that will be sent after an atomic write
         //!
         std::vector< state::pending::Reply> pendingReplies;


      };


      namespace state
      {
         struct Base
         {
            Base( State& state) : m_state( state) {}

         protected:
            State& m_state;

         };

         void configurate( State& state);

         namespace remove
         {
            void instance( common::platform::pid_type pid, State& state);
         } // remove



      } // state
   } // transaction
} // casual

#endif // MANAGER_STATE_H_
