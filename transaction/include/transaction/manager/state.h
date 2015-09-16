//!
//! manager_state.h
//!
//! Created on: Aug 13, 2013
//!     Author: Lazan
//!

#ifndef CASUAL_TRANSACTION_MANAGER_STATE_H_
#define CASUAL_TRANSACTION_MANAGER_STATE_H_

#include "common/platform.h"
#include "common/message/transaction.h"
#include "common/algorithm.h"
#include "common/marshal/binary.h"

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
         struct Statistics
         {
            Statistics();

            std::chrono::microseconds min;
            std::chrono::microseconds max;
            std::chrono::microseconds total;
            std::size_t invoked;

            void start( const common::platform::time_point& start);
            void end( const common::platform::time_point& end);

            void time( const common::platform::time_point& start, const common::platform::time_point& end);


            friend Statistics& operator += ( Statistics& lhs, const Statistics& rhs);

         private:
            common::platform::time_point m_start;
         };

         struct Stats
         {
            Statistics resource;
            Statistics roundtrip;

            friend Stats& operator += ( Stats& lhs, const Stats& rhs);
         };

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
                  common::process::Handle process;

                  Stats statistics;

                  void state( State state);
                  State state() const;

               private:
                  State m_state = State::absent;

               };

               Proxy() = default;
               Proxy( id_type id) : id( id) {}

               id_type id = 0;

               std::string key;
               std::string openinfo;
               std::string closeinfo;
               std::size_t concurency = 0;

               //!
               //! This 'counterä' keep track of statistics for removed
               //! instances, so we can give a better view for the operator.
               //!
               Stats statistics;

               std::vector< Instance> instances;


               friend bool operator < ( const Proxy& lhs, const Proxy& rhs)
               {
                  return lhs.id < rhs.id;
               }

               friend bool operator == ( const Proxy& lhs, id_type rhs) { return lhs.id == rhs; }
               friend bool operator == ( id_type lhs, const Proxy& rhs) { return lhs == rhs.id; }

               friend std::ostream& operator << ( std::ostream& out, const Proxy& value);
               friend std::ostream& operator << ( std::ostream& out, const Proxy::Instance& value);
               friend std::ostream& operator << ( std::ostream& out, const Proxy::Instance::State& value);

            };




            namespace update
            {
               template< Proxy::Instance::State state>
               struct State
               {
                  void operator () ( Proxy::Instance& instance) const
                  {
                     instance.state( state);
                  }
               };
            } // update

         } // resource



         namespace pending
         {
            struct base_message
            {
               template< typename M>
               base_message( M&& information) : message{ common::marshal::complete( std::forward< M>( information))}
               {
               }

               base_message( base_message&&) = default;
               base_message& operator = ( base_message&&) = default;

               common::ipc::message::Complete message;
               common::platform::time_point created;
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
                           value.resources, [=]( id_type id){ return id == m_id;});
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

            enum class Stage
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
               cXAER_ASYNC,
               cXA_RETRY,
               cXAER_DUPID,
               cXAER_NOTA,
               cXA_OK,      //! Went as expected
               cXA_RDONLY,  //! Went "better" than expected
            };

            Resource( id_type id) : id( id) {}
            Resource( Resource&&) noexcept = default;
            Resource& operator = ( Resource&&) noexcept = default;

            id_type id;
            Stage stage = Stage::cInvolved;
            Result result = Result::cXA_OK;

            static Result convert( int value);
            static int convert( Result value);

            void setResult( int value);


            struct update
            {
               struct Stage
               {
                  Stage( Resource::Stage stage) : m_stage( stage) {}

                  void operator () ( Resource& value) const
                  {
                     value.stage = m_stage;
                  }
               private:
                  Resource::Stage m_stage;
               };
            };

            struct filter
            {
               struct Stage
               {
                  Stage( Resource::Stage stage) : m_stage( stage) {}

                  bool operator () ( const Resource& value) const
                  {
                     return value.stage == m_stage;
                  }
               private:
                  Resource::Stage m_stage;
               };

               struct Result
               {
                  Result( Resource::Result result) : m_result( result) {}

                  bool operator () ( const Resource& value) const
                  {
                     return value.result == m_result;
                  }
               private:
                  Resource::Result m_result;
               };

               struct ID
               {
                  ID( id_type id) : m_id( id) {}

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


         Transaction() = default;
         Transaction( Transaction&&) = default;
         Transaction& operator = ( Transaction&&) = default;

         Transaction( common::transaction::ID trid) : trid( std::move( trid)) {}


         common::transaction::ID trid;
         std::vector< Resource> resources;

         //!
         //! Used to keep track of the origin for commit request.
         //!
         common::Uuid correlation;

         Resource::Stage stage() const;

         //!
         //! @return the most severe result from the resources
         //!
         Resource::Result results() const;

         friend std::ostream& operator << ( std::ostream& out, const Transaction& value);
      };




      namespace find
      {
         struct Transaction
         {
            Transaction( const common::transaction::ID& trid) : m_trid( trid) {}
            bool operator () ( const transaction::Transaction& value) const
            {
               return value.trid == m_trid;
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
            const common::transaction::ID& m_trid;
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


      class State
      {
      public:
         State( const std::string& database);


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
         //! @return true if there are pending stuff to do. We can't block
         //! if we got stuff to do...
         //!
         bool pending() const;


         //!
         //! @return number of total instances
         //!
         std::size_t instances() const;

         std::vector< common::platform::pid_type> processes() const;

         void process( common::process::lifetime::Exit death);

         state::resource::Proxy& get_resource( common::platform::resource::id_type rm);
         state::resource::Proxy::Instance& get_instance( common::platform::resource::id_type rm, common::platform::pid_type pid);

         using instance_range = decltype( common::range::make( std::declval< state::resource::Proxy>().instances.begin(), std::declval< state::resource::Proxy>().instances.end()));
         instance_range idle_instance( common::platform::resource::id_type rm);

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
                  return instance.process.pid == m_pid;
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
                  return instance.state() == resource::Proxy::Instance::State::idle;
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
