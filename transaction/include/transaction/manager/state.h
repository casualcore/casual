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

#include "config/xa_switch.h"

#include "transaction/manager/log.h"



#include <map>

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
               struct Instance
               {
                  Instance( std::size_t id) : id( id) {}
                  Instance( std::size_t id, const common::message::server::Id& server) : id{ id}, server{ server} {}

                  enum class State
                  {
                     absent,
                     started,
                     idle,
                     busy,
                     startupError,
                     shutdown
                  };

                  std::size_t id;
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

               std::size_t id = 0;

               std::string key;
               std::string openinfo;
               std::string closeinfo;
               std::size_t concurency = 0;

               //std::vector< Instance> instances;
            };

         } // resource
      } // state



      namespace action
      {
         struct Resource
         {
            enum class State
            {
               cInvolved,
               cPrepareRequested,
               cPrepareFailed,
               cPrepared,
               cCommitRequested,
               cCommitFailed,
               cCommitted,
               cNotInvolved,
            };

            Resource( std::size_t id) : id( id) {}

            bool operator < ( const Resource& rhs) const
            {
               return id < rhs.id;
            }

            bool operator == ( const Resource& rhs) const
            {
               return id == rhs.id;
            }


            std::size_t id;
            State state = State::cInvolved;
         };

         struct Task
         {

            typedef common::platform::queue_id_type queue_id_type;


            Task() = default;
            Task( Task&&) = default;


            queue_id_type target;
            common::transaction::ID xid;
            std::vector< Resource> resources;

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


            struct Find
            {
               Find( const common::transaction::ID& xid) : m_xid( xid) {}

               bool operator () ( const Task& value) const
               {
                  return value.xid == m_xid;
               }

            private:
               const common::transaction::ID& m_xid;
            };

         };

         namespace pending
         {
            struct Request
            {
               std::size_t resourceId;
               common::ipc::message::Complete message;


               struct Find
               {
                  Find( std::size_t resourceId) : m_resourceId( resourceId) {}

                  bool operator () ( const Request& value) const
                  {
                     return value.resourceId == m_resourceId;
                  }

               private:
                  std::size_t m_resourceId;
               };

            };

         } // pending

         namespace pending
         {
            struct Reply
            {
               typedef common::platform::queue_id_type queue_id_type;

               queue_id_type target;
               common::ipc::message::Complete message;
            };
         } // pending

      } // action

      struct State
      {
         State( const std::string& database);


         //typedef instances_type;

         std::map< std::string, config::xa::Switch> xaConfig;

         std::vector< state::resource::Proxy> resources;
         std::vector< state::resource::Proxy::Instance> instances;



         std::vector< action::Task> tasks;

         std::vector< action::pending::Request> pendingRequest;

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
            common::Range< Iter> instance( Iter first, Iter last, const M& message)
            {
               auto resourceRange = common::sorted::bound(
                     first,
                     last,
                     state::resource::Proxy::Instance{ message.resource},
                     state::resource::Proxy::Instance::order::Id{});

               return common::sorted::bound(
                     resourceRange.first,
                     resourceRange.last,
                     state::resource::Proxy::Instance{ message.resource, message.id},
                     state::resource::Proxy::Instance::order::Pid{});
            }

            namespace idle
            {
               template< typename Iter>
               common::Range< Iter> instance( Iter first, Iter last, std::size_t resourceId)
               {
                  auto resourceRange = common::sorted::bound(
                        first,
                        last,
                        state::resource::Proxy::Instance{ resourceId},
                        state::resource::Proxy::Instance::order::Id{});

                  auto instance = std::find_if( resourceRange.first, resourceRange.last, filter::Idle{});

                  if( instance != resourceRange.last)
                  {
                     return common::make_range( instance, instance + 1);
                  }
                  else
                  {
                     return common::make_range( instance, instance);
                  }
               }
            } // idle

            template< typename Iter>
            common::Range< Iter> task( Iter first, Iter last, const common::transaction::ID& xid)
            {
               first = std::find_if( first, last, action::Task::Find( xid));

               if( first != last)
               {
                  return common::make_range( first, first + 1);
               }
               return common::make_range( last, last);
            }




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
