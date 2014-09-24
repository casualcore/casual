//!
//! manager_state.h
//!
//! Created on: Aug 13, 2013
//!     Author: Lazan
//!

#ifndef MANAGER_STATE_H_
#define MANAGER_STATE_H_

#include "common/platform.h"
#include "common/message/transaction.h"
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
               base_message& operator = ( base_message&&) = default;

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



            //!
            //! Used to rank the return codes from the resources, the lower the enum value (higher up),
            //! the more severe...
            //!
            enum class Result
            {
               cXA_HEURHAZ,
               cXA_HEURMIX,
               cXA_HEURCOM,
               cXA_HEURRB,
               cXAER_RMFAIL,
               cXAER_RMERR,
               cXA_RBINTEGRITY,
               cXA_RBCOMMFAIL,
               cXA_RBROLLBACK,
               cXA_RBOTHER,
               cXA_RBDEADLOCK,
               cXAER_PROTO,
               cXA_RBPROTO,
               cXA_RBTIMEOUT,
               cXA_RBTRANSIENT,
               cXAER_INVAL,
               cXA_NOMIGRATE,
               cXAER_OUTSIDE,
               cXAER_NOTA,
               cXAER_ASYNC,
               cXA_RETRY,
               cXAER_DUPID,
               cXA_OK,      //! Went as expected
               cXA_RDONLY,  //! Went "better" than expected
            };

            Resource( id_type id) : id( id) {}
            Resource( Resource&&) noexcept = default;
            Resource& operator = ( Resource&&) noexcept = default;

            id_type id;
            State state = State::cInvolved;
            Result result = Result::cXA_OK;

            static Result convert( int value);
            static int convert( Result value);

            void setResult( int value);


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

            struct result
            {
               struct Filter
               {
                  Filter( Result result) : m_result( result) {}

                  bool operator () ( const Resource& value) const
                  {
                     return value.result == m_result;
                  }
               private:
                  Result m_result;
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


            friend bool operator < ( const Resource& lhs, const Resource& rhs) { return lhs.id < rhs.id; }
            friend bool operator == ( const Resource& lhs, const Resource& rhs) { return lhs.id == rhs.id; }
            friend std::ostream& operator << ( std::ostream& out, const Resource& value) { return out << value.id; }

         };


         typedef common::message::server::Id id_type;


         Transaction() = default;
         Transaction( Transaction&&) = default;
         Transaction& operator = ( Transaction&&) = default;

         Transaction( id_type owner, common::transaction::ID xid) : owner( owner), xid( xid) {}


         id_type owner;
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

         //!
         //! @return the most severe result from the resources
         //!
         Resource::Result results() const;

         friend std::ostream& operator << ( std::ostream& out, const Transaction& value)
         {
            return out << "{xid: " << value.xid << " owner: " << value.owner << " resources: " << common::range::make( value.resources) << "}";
         }
      };




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
         std::vector< state::pending::Reply> persistentReplies;

         //!
         //! Resource request, that will be processed as soon as possible,
         //! that is, as soon as corresponding resources is done/idle
         //!
         std::vector< state::pending::Request> pendingRequests;

         //!
         //! Resource request, that will be processed after an atomic
         //! write to the log. If corresponding resources is busy, for some
         //! requests, these will be moved to pendingRequests
         //!
         std::vector< state::pending::Request> persistentRequests;


         transaction::Log log;


         //!
         //! @return number of total instances
         //!
         std::size_t instances() const;

         std::vector< common::platform::pid_type> processes() const;

         void removeProcess(  common::platform::pid_type pid);

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
                  return common::range::any_of( resource.instances, Running{});
               }
            };



         } // filter


         namespace find
         {
            template< typename R>
            auto resource( R&& range, common::platform::resource::id_type id) -> decltype( common::range::make( range))
            {
               return common::range::sorted::bound(
                  range,
                  state::resource::Proxy{ id});
            }

            template< typename R, typename M>
            auto instance( R&& resources, const M& message) -> decltype( common::range::make( std::begin( resources)->instances))
            {
               auto resourceRange = resource(
                     resources,
                     message.resource);

               if( resourceRange.empty())
                  return decltype( common::range::make( std::begin( resources)->instances))();

               return common::range::find_if(
                     resourceRange->instances,
                     filter::Instance{ message.id.pid});
            }

            namespace idle
            {
               template< typename R>
               auto instance( R&& resources, common::platform::resource::id_type id) -> decltype( common::range::make( std::begin( resources)->instances))
               {
                  auto range = resource(
                        resources,
                        id);

                  if( range.empty())
                     return decltype( common::range::make( std::begin( resources)->instances))();

                  return common::range::find_if(
                     common::range::make( range->instances),
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

         void configure( State& state, const common::message::transaction::manager::Configuration& configuration);



      } // state



   } // transaction
} // casual

#endif // MANAGER_STATE_H_
