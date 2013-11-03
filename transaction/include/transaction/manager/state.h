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

#include "transaction/manager/log.h"



#include <map>

namespace casual
{

   namespace transaction
   {
      namespace state
      {

         namespace pending
         {
            template< typename Q>
            struct base_reply
            {
               virtual ~base_reply() {}
               virtual bool send( Q& queue) const = 0;
            };


            struct Reply
            {
               typedef common::platform::queue_id_type queue_id_type;

               queue_id_type target;
               //common::message::transaction::reply::Generic reply;
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
                     idle,
                     busy,
                     startupError,
                     shutdown
                  };

                  std::shared_ptr< Proxy> proxy;
                  common::message::server::Id id;
                  State state = State::absent;

               };

               std::size_t id = 0;

               std::string key;
               std::string openinfo;
               std::string closeinfo;
               std::size_t concurency = 0;

               std::vector< std::shared_ptr< Instance>> instances;
            };

         } // resource





         namespace pending
         {
            struct base_pending
            {
               XID xid;
               std::vector< std::size_t> involved;


            };

            struct Prepare : base_pending
            {



            };

            struct Commit : base_pending
            {

            };

            struct Rollback : base_pending
            {

            };

         } // pending

         struct Transaction
         {

            std::vector< std::size_t> resoursesInvolved;
         };

      } // state

      struct State
      {
         State( const std::string& database);

         using instances_mapping_type = std::map< common::platform::pid_type, std::shared_ptr< state::resource::Proxy::Instance>>;


         std::map< std::string, config::xa::Switch> xaConfig;

         std::vector< std::shared_ptr< state::resource::Proxy>> resources;
         instances_mapping_type instances;

         //!
         //! Replies that will be sent after an atomic write
         //!
         std::vector< state::pending::Reply> pendingReplies;

         std::map< common::transaction::ID, state::Transaction> transactions;


         transaction::Log log;

      };

      namespace state
      {
         namespace filter
         {
            struct Instance
            {
               Instance( common::platform::pid_type pid);

               bool operator () ( const resource::Proxy::Instance& instance) const;

            private:
               common::platform::pid_type pid;
            };


            struct Started
            {
               //!
               //! @return true if instance is started
               //!
               bool operator () ( const std::shared_ptr< resource::Proxy::Instance>& instance) const;

               //!
               //! @return true if at least one instance in resource-proxy is started
               //!
               bool operator () ( const std::shared_ptr< state::resource::Proxy>& proxy) const;
            };

            struct Running
            {
               //!
               //! @return true if instance is running
               //!
               bool operator () ( const std::shared_ptr< resource::Proxy::Instance>& instance) const;

               //!
               //! @return true if at least one instance in resource-proxy is running
               //!
               bool operator () ( const std::shared_ptr< state::resource::Proxy>& proxy) const;
            };

         } // filter

         //!
         //! Base that holds the state
         //!
         struct Base
         {
            Base( State& state) : m_state( state) {}

         protected:
            State& m_state;

         };

         void configure( State& state, const common::message::transaction::Configuration& configuration);

         namespace remove
         {
            void instance( common::platform::pid_type pid, State& state);
         } // remove


      } // state
   } // transaction
} // casual

#endif // MANAGER_STATE_H_
