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

               };

               Proxy() = default;
               Proxy( id_type id) : id( id) {}

               id_type id = 0;

               std::string key;
               std::string openinfo;
               std::string closeinfo;
               std::size_t concurency = 0;

               std::vector< Instance> instances;
            };

            inline bool operator < ( const Proxy& lhs, const Proxy& rhs)
            {
               return lhs.id < rhs.id;
            }


            namespace update
            {
               template< Proxy::Instance::State state>
               struct State
               {
                  void operator () ( Proxy::Instance& instance) const
                  {
                     instance.state = state;
                  }
               };
            } // update

         } // resource



         namespace pending
         {
            struct base_message
            {
               template< typename M>
               base_message( M&& information)
               {
                  common::marshal::output::Binary archive;
                  archive << information;

                  auto type = common::message::type( information);
                  message = common::ipc::message::Complete( type, archive.release());
               }

               base_message( base_message&&) = default;

               common::ipc::message::Complete message;
            };

            struct Reply : base_message
            {
               typedef common::platform::queue_id_type queue_id_type;

               template< typename M>
               Reply( queue_id_type target, M&& message) : base_message( std::forward< M>( message)), target( target) {}

               queue_id_type target;
            };

            struct Request : base_message
            {
               using id_type = common::platform::resource::id_type;

               using base_message::base_message;

               std::vector< id_type> resources;

            };

            namespace filter
            {
               struct Request
               {
                  using id_type = common::platform::resource::id_type;

                  Request( id_type id) : m_id( id) {}

                  bool operator () ( const pending::Request& value) const
                  {
                     return common::range::any_of(
                           common::range::make( value.resources), [=]( id_type id){ return id == m_id;});
                  }

                  id_type m_id;
               };
            } // filter
         } // pending
      } // state


      struct Transaction
      {
         struct Resource
         {
            using id_type = common::platform::resource::id_type;

            enum class State
            {
               cInvolved,
               cPrepareRequested,
               cPrepareReplied,
               cCommitRequested,
               cCommitReplied,
               cRollbackRequested,
               cRollbackReplied,
               cDone,
               cError,
               cNotInvolved,
            };

            Resource( id_type id) : id( id) {}

            id_type id;
            State state = State::cInvolved;
            int result = XA_OK;


            struct state
            {
               struct Update
               {
                  Update( State state) : m_state( state) {}

                  void operator () ( Resource& value) const
                  {
                     value.state = m_state;
                  }
               private:
                  State m_state;
               };

               struct Filter
               {
                  Filter( State state) : m_state( state) {}

                  bool operator () ( const Resource& value) const
                  {
                     return value.state == m_state;
                  }
               private:
                  State m_state;
               };
            };

            struct id
            {
               struct Filter
               {
                  Filter( id_type id) : m_id( id) {}

                  bool operator () ( const Resource& value) const
                  {
                     return value.id == m_id;
                  }
               private:
                  id_type m_id;
               };

            };



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

         std::vector< int> results() const
         {
            std::vector< int> result;

            for( auto& resource : resources)
            {
               result.push_back( resource.result);
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
         namespace resource
         {
            struct ID
            {
               template< typename T>
               common::platform::resource::id_type operator () ( const T& value) const
               {
                  return value.id;
               }
            };
         } // resource
      } // transform


      struct State
      {
         State( const std::string& database);


         //typedef instances_type;

         std::map< std::string, config::xa::Switch> xaConfig;

         std::vector< state::resource::Proxy> resources;

         std::vector< Transaction> transactions;

         //!
         //! Replies that will be sent after an atomic write to the log
         //!
         std::vector< state::pending::Reply> pendingReplies;

         std::vector< state::pending::Request> pendingRequests;


         transaction::Log log;

      };

      namespace state
      {


         namespace filter
         {
            struct Instance
            {
               Instance( common::platform::pid_type pid) : m_pid( pid) {}
               bool operator () ( const resource::Proxy::Instance& instance) const
               {
                  return instance.server.pid == m_pid;
               }
            private:
               common::platform::pid_type m_pid;

            };

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
               bool operator () ( const resource::Proxy& resource) const
               {
                  return std::any_of( std::begin( resource.instances), std::end( resource.instances), Running{});
               }
            };



         } // filter


         namespace find
         {
            template< typename Iter>
            common::Range< Iter> resource( common::Range< Iter> range, common::platform::resource::id_type id)
            {
               return common::range::sorted::bound(
                  range,
                  state::resource::Proxy{ id});
            }

            template< typename Iter, typename M>
            auto instance( common::Range< Iter> range, const M& message) -> decltype( common::range::make( range.first->instances))
            {
               auto resourceRange = resource(
                     range,
                     message.resource);

               if( resourceRange.empty())
                  return decltype( common::range::make( range.first->instances))();

               return common::range::find_if(
                     common::range::make( resourceRange.first->instances),
                     filter::Instance{ message.id.pid});
            }

            namespace idle
            {
               template< typename Iter>
               auto instance( common::Range< Iter> resources, common::platform::resource::id_type id) -> decltype( common::range::make( resources.first->instances))
               {
                  resources = resource(
                        resources,
                        id);

                  if( resources.empty())
                     return decltype( common::range::make( resources.first->instances))();

                  return common::range::find_if(
                     common::range::make( resources.first->instances),
                     filter::Idle{});
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
