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
#include "common/algorithm.h"
#include "common/marshal.h"

#include "config/xa_switch.h"

#include "transaction/manager/log.h"



#include <map>
#include <deque>
#include <vector>

namespace casual
{

   namespace transaction
   {
      namespace state
      {
         namespace resource
         {
            struct Proxy
            {
               using id_type = common::platform::resource::id_type;

               struct Instance
               {
                  Instance( id_type id) : id( id) {}
                  Instance( id_type id, const common::message::server::Id& server) : id{ id}, server{ server} {}

                  enum class State
                  {
                     absent,
                     started,
                     idle,
                     busy,
                     startupError,
                     shutdown
                  };

                  id_type id;
                  common::message::server::Id server;
                  State state = State::absent;



                  struct order
                  {
                     struct Id
                     {
                        bool operator () ( const Instance& lhs, const Instance& rhs) const
                        {
                           return lhs.id < rhs.id;
                        }
                     };

                     struct Pid
                     {
                        bool operator () ( const Instance& lhs, const Instance& rhs) const
                        {
                           return lhs.server.pid < rhs.server.pid;
                        }
                     };
                  };



                  bool operator < ( const Instance& rhs) const
                  {
                     order::Id order;
                     if( order( *this, rhs))
                        return true;
                     if(order( rhs, *this))
                        return false;
                     return order::Pid{}( *this, rhs);
                  }

               };

               id_type id = 0;

               std::string key;
               std::string openinfo;
               std::string closeinfo;
               std::size_t concurency = 0;
            };

         } // resource
      } // state



      namespace action
      {

         namespace pending
         {
            struct Reply
            {
               typedef common::platform::queue_id_type queue_id_type;

               template< typename M>
               Reply( queue_id_type target, M&& information) : target( target)
               {
                  common::marshal::output::Binary archive;
                  archive << information;

                  auto type = common::message::type( information);
                  message = common::ipc::message::Complete( type, archive.release());
               }

               Reply( Reply&&) = default;


               queue_id_type target;
               common::ipc::message::Complete message;
            };
         } // pending

      } // action




      struct Transaction
      {
         struct Resource
         {
            using id_type = common::platform::resource::id_type;

            enum class State
            {
               cInvolved,
               cPrepareRequested,
               cPrepareFailed,
               cPrepared,
               cCommitRequested,
               cCommitFailed,
               cCommitted,
               cRollbackRequested,
               cRollbackFailed,
               cRollbacked,
               cNotInvolved,
            };

            Resource( id_type id) : id( id) {}

            id_type id;
            State state = State::cInvolved;
         };

         enum class Task
         {
            invalid,
            logAndReplyBegin,
            waitForCommitOrRollback,
            waitForResourcesPrepare,
            waitForResourcesCommit,
            waitForResourcesRollback,

         };


         typedef common::message::server::Id id_type;


         Transaction() = default;
         Transaction( Transaction&&) = default;


         id_type owner;
         common::transaction::ID xid;
         std::vector< Resource> resources;
         Task task = Task::invalid;

         Resource::State state() const
         {
            Resource::State result = Resource::State::cNotInvolved;

            for( auto& resource : resources)
            {
               if( result > resource.state)
                  result = resource.state;
            }
            return result;
         }
      };

      inline bool operator < ( const Transaction::Resource& lhs, const Transaction::Resource& rhs) { return lhs.id < rhs.id; }
      inline bool operator == ( const Transaction::Resource& lhs, const Transaction::Resource& rhs) { return lhs.id == rhs.id; }
      inline std::ostream& operator << ( std::ostream& out, const Transaction::Resource& value) { return out << value.id; }

      namespace find
      {
         struct Transaction
         {
            Transaction( const common::transaction::ID& xid) : m_xid( xid) {}
            bool operator () ( const transaction::Transaction& value) const
            {
               return value.xid == m_xid;
            }

            struct Resource
            {
               using id_type = transaction::Transaction::Resource::id_type;

               Resource( id_type id) : m_id( id) {}
               bool operator () ( const transaction::Transaction::Resource& value) const
               {
                  return value.id == m_id;
               }
            private:
               id_type m_id;
            };

         private:
            const common::transaction::ID& m_xid;
         };

      } // find

      namespace transform
      {
         struct Transaction
         {
            transaction::Transaction operator () ( const common::message::transaction::begin::Request& message) const
            {
               transaction::Transaction result;

               result.owner = message.id;
               result.xid = message.xid;
               result.task = transaction::Transaction::Task::logAndReplyBegin;

               return result;
            }

         };

         namespace pending
         {
            template< typename M>
            action::pending::Reply reply( const transaction::Transaction& transaction, common::platform::queue_id_type queue)
            {
               M message;
               message.id.queue_id = common::ipc::receive::id();
               message.xid = transaction.xid;
               message.state = 0; // TODO:

               return action::pending::Reply( queue, message);
            }
         } // pending


      } // transform


      struct State
      {
         State( const std::string& database);


         //typedef instances_type;

         std::map< std::string, config::xa::Switch> xaConfig;

         std::vector< state::resource::Proxy> resources;
         std::vector< state::resource::Proxy::Instance> instances;


         std::vector< Transaction> transactions;

         //!
         //! Replies that will be sent after an atomic write to the log
         //!
         std::vector< action::pending::Reply> pendingReplies;


         transaction::Log log;

      };

      namespace state
      {


         namespace filter
         {

            struct Idle
            {
               //!
               //! @return true if instance is idle
               //!
               bool operator () ( const resource::Proxy::Instance& instance) const
               {
                  return instance.state == resource::Proxy::Instance::State::idle;
               }
            };

            struct Running
            {

               //!
               //! @return true if instance is running
               //!
               bool operator () ( const resource::Proxy::Instance& instance) const;

               //!
               //! @return true if at least one instance in resource-proxy is running
               //!
               template< typename Iter>
               bool operator () ( const common::Range< Iter>& resource) const
               {
                  return std::any_of( resource.first, resource.last, Running{});
               }
            };



         } // filter


         namespace find
         {
            template< typename Iter, typename M>
            common::Range< Iter> instance( common::Range< Iter> range, const M& message)
            {
               auto resourceRange = common::range::sorted::bound(
                     range,
                     state::resource::Proxy::Instance{ message.resource},
                     state::resource::Proxy::Instance::order::Id{});

               return common::range::sorted::bound(
                     resourceRange,
                     state::resource::Proxy::Instance{ message.resource, message.id},
                     state::resource::Proxy::Instance::order::Pid{});
            }

            namespace idle
            {
               template< typename Iter>
               common::Range< Iter> instance( common::Range< Iter> range, common::platform::resource::id_type id)
               {
                  auto resourceRange = common::range::sorted::bound(
                        range,
                        state::resource::Proxy::Instance{ id},
                        state::resource::Proxy::Instance::order::Id{});

                  return common::range::find( resourceRange, filter::Idle{});
               }
            } // idle
         } // find



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
