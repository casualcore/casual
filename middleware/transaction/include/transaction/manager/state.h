//!
//! casual
//!

#ifndef CASUAL_TRANSACTION_MANAGER_STATE_H_
#define CASUAL_TRANSACTION_MANAGER_STATE_H_

#include "common/platform.h"
#include "common/message/transaction.h"
#include "common/message/domain.h"
#include "common/algorithm.h"
#include "common/marshal/complete.h"

#include "transaction/manager/log.h"

#include "configuration/resource/property.h"



#include <map>
#include <deque>
#include <vector>


namespace casual
{

   namespace transaction
   {
      using size_type = common::platform::size::type;

      namespace handle
      {
         namespace implementation
         {
            struct Interface;
         } // implementation
      } // handle

      class State;

      namespace state
      {
         struct Statistics
         {
            Statistics();

            std::chrono::microseconds min;
            std::chrono::microseconds max;
            std::chrono::microseconds total;
            size_type invoked;

            void start( const common::platform::time::point::type& start);
            void end( const common::platform::time::point::type& end);

            void time( const common::platform::time::point::type& start, const common::platform::time::point::type& end);


            friend Statistics& operator += ( Statistics& lhs, const Statistics& rhs);

         private:
            common::platform::time::point::type m_start;
         };

         struct Stats
         {
            Statistics resource;
            Statistics roundtrip;

            friend Stats& operator += ( Stats& lhs, const Stats& rhs);
         };

         namespace resource
         {
            namespace id
            {
               using type = common::platform::resource::id::type;

               inline bool remote( type id) { return id < 0;}
               inline bool local( type id) { return id > 0;}

            } // id


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
                     error,
                     shutdown
                  };

                  id::type id;
                  common::process::Handle process;

                  Stats statistics;

                  void state( State state);
                  State state() const;

               private:
                  State m_state = State::absent;

               };

               struct generate_id {};

               Proxy() = default;
               inline Proxy( generate_id) : id( next_id()) {}

               id::type id = 0;

               std::string key;
               std::string openinfo;
               std::string closeinfo;
               size_type concurency = 0;

               //!
               //! This 'counter' keep track of statistics for removed
               //! instances, so we can give a better view for the operator.
               //!
               Stats statistics;

               std::vector< Instance> instances;

               std::string name;
               std::string note;

               //!
               //! @return true if all instances 'has connected'
               //!
               bool booted() const;

               bool remove_instance( common::platform::pid::type pid);


               friend bool operator < ( const Proxy& lhs, const Proxy& rhs)
               {
                  return lhs.id < rhs.id;
               }

               friend bool operator == ( const Proxy& lhs, id::type rhs) { return lhs.id == rhs; }
               friend bool operator == ( id::type lhs, const Proxy& rhs) { return lhs == rhs.id; }

               friend std::ostream& operator << ( std::ostream& out, const Proxy& value);
               friend std::ostream& operator << ( std::ostream& out, const Proxy::Instance& value);
               friend std::ostream& operator << ( std::ostream& out, const Proxy::Instance::State& value);

            private:

               inline static size_type next_id()
               {
                  static size_type id = 1;
                  return id++;
               }

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


            namespace external
            {
               struct Proxy
               {
                  Proxy( const common::process::Handle& process, id::type id);


                  common::process::Handle process;

                  //!
                  //! RM id
                  //!
                  id::type id;

                  friend bool operator == ( const Proxy& lhs, const common::process::Handle& rhs);
               };

               namespace proxy
               {
                  id::type id( State& state, const common::process::Handle& process);
               } // proxy

            } // external

         } // resource



         namespace pending
         {
            struct base_message
            {
               template< typename M>
               base_message( M&& information) : message{ common::marshal::complete( std::forward< M>( information))}
               {
               }

               base_message( base_message&&) noexcept = default;
               base_message& operator = ( base_message&&) noexcept = default;

               common::communication::message::Complete message;
               common::platform::time::point::type created;
            };

            struct Reply : base_message
            {
               using queue_id_type = common::platform::ipc::id;

               template< typename M>
               Reply( queue_id_type target, M&& message) : base_message( std::forward< M>( message)), target( target) {}

               queue_id_type target;
            };

            struct Request : base_message
            {
               using id_type = common::platform::resource::id::type;

               template< typename M>
               Request( id_type resource, M&& message) : base_message( std::forward< M>( message)), resource( resource) {}

               id_type resource;

            };

            namespace filter
            {
               struct Request
               {
                  using id_type = common::platform::resource::id::type;

                  Request( id_type id) : m_id( id) {}

                  bool operator () ( const pending::Request& value) const
                  {
                     return value.resource == m_id;
                  }

               private:
                  id_type m_id;
               };
            } // filter
         } // pending
      } // state


      struct Transaction
      {
         struct Resource
         {
            using id_type = state::resource::id::type;

            enum class Stage : std::uint16_t
            {
               involved,
               prepare_requested,
               prepare_replied,
               commit_requested,
               commit_replied,
               rollback_requested,
               rollback_replied,
               done,
               error,
               not_involved,
            };



            //!
            //! Used to rank the return codes from the resources, the lower the enum value (higher up),
            //! the more severe...
            //!
            enum class Result : int
            {
               xa_HEURHAZ,
               xa_HEURMIX,
               xa_HEURCOM,
               xa_HEURRB,
               xaer_RMFAIL,
               xaer_RMERR,
               xa_RBINTEGRITY,
               xa_RBCOMMFAIL,
               xa_RBROLLBACK,
               xa_RBOTHER,
               xa_RBDEADLOCK,
               xaer_PROTO,
               xa_RBPROTO,
               xa_RBTIMEOUT,
               xa_RBTRANSIENT,
               xaer_INVAL,
               xa_NOMIGRATE,
               xaer_OUTSIDE,
               xaer_ASYNC,
               xa_RETRY,
               xaer_DUPID,
               xaer_NOTA,  //! nothing to do?
               xa_OK,      //! Went as expected
               xa_RDONLY,  //! Went "better" than expected
            };

            Resource( id_type id) : id( id) {}
            Resource( Resource&&) noexcept = default;
            Resource& operator = ( Resource&&) noexcept = default;

            id_type id;
            Stage stage = Stage::involved;
            Result result = Result::xa_OK;

            static Result convert( common::code::xa value);
            static common::code::xa convert( Result value);

            void set_result( common::code::xa value);

            //!
            //! @return true if there's nothing more to do, hence this resource can be removed
            //!    from the transaction
            //!
            bool done() const;

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

         //!
         //! Depending on context the transaction will have different
         //! handle-implementations.
         //!
         const handle::implementation::Interface* implementation = nullptr;

         common::transaction::ID trid;
         std::vector< Resource> resources;

         common::platform::time::point::type started;
         common::platform::time::point::type deadline;


         //!
         //! Used to keep track of the origin for commit request.
         //!
         common::Uuid correlation;

         //!
         //! Indicate if the transaction is owned by a remote domain,
         //! and what RM id that domain act as.
         //!
         state::resource::id::type resource = 0;


         Resource::Stage stage() const;

         //!
         //! @return the most severe result from the resources
         //!
         common::code::xa results() const;

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
               common::platform::resource::id::type operator () ( const T& value) const
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

         State( const State&) = delete;
         State& operator = ( const State&) = delete;

         std::vector< Transaction> transactions;

         std::vector< state::resource::Proxy> resources;

         std::vector< state::resource::external::Proxy> externals;


         struct
         {
            //!
            //! Replies that will be sent after an atomic write to the log
            //!
            std::vector< state::pending::Reply> replies;

            //!
            //! Resource request, that will be processed after an atomic
            //! write to the log. If corresponding resources is busy, for some
            //! requests, these will be moved to pending.requests
            //!
            std::vector< state::pending::Request> requests;

         } persistent;



         struct
         {
            //!
            //! Resource request, that will be processed as soon as possible,
            //! that is, as soon as corresponding resources is done/idle
            //!
            std::vector< state::pending::Request> requests;

         } pending;







         //!
         //! the persistent transaction log
         //!
         transaction::Log log;


         std::map< std::string, configuration::resource::Property> resource_properties;


         //!
         //! @return true if there are pending stuff to do. We can't block
         //! if we got stuff to do...
         //!
         bool outstanding() const;



         //!
         //! @return true if all resource proxies is booted
         //!
         bool booted() const;


         //!
         //! @return number of total instances
         //!
         size_type instances() const;

         std::vector< common::platform::pid::type> processes() const;

         void operator () ( const common::process::lifetime::Exit& death);

         state::resource::Proxy& get_resource( state::resource::id::type rm);
         state::resource::Proxy::Instance& get_instance( state::resource::id::type rm, common::platform::pid::type pid);

         bool remove_instance( common::platform::pid::type pid);


         using instance_range = common::range::type_t< std::vector< state::resource::Proxy::Instance>>;
         instance_range idle_instance( state::resource::id::type rm);

         const state::resource::external::Proxy& get_external( state::resource::id::type rm) const;



      private:



      };

      namespace state
      {


         namespace filter
         {

            struct Instance
            {
               Instance( common::platform::pid::type pid) : m_pid( pid) {}
               bool operator () ( const resource::Proxy::Instance& instance) const
               {
                  return instance.process.pid == m_pid;
               }
            private:
               common::platform::pid::type m_pid;

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

         void configure( State& state, const common::message::domain::configuration::Reply& configuration);



      } // state



   } // transaction
} // casual

#endif // MANAGER_STATE_H_
