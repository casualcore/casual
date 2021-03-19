//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "transaction/global.h"
#include "transaction/manager/log.h"

#include "casual/platform.h"
#include "common/message/transaction.h"
#include "common/message/pending.h"
#include "common/algorithm.h"
#include "common/metric.h"
#include "common/functional.h"

#include "common/serialize/native/complete.h"

#include "configuration/resource/property.h"
#include "configuration/model.h"

#include <map>
#include <deque>
#include <vector>


namespace casual
{
   namespace transaction
   {
      namespace manager
      {
         using size_type = platform::size::type;

         struct State;

         //! argument settings
         struct Settings
         {
            std::string log;
         };

         namespace state
         {
            struct Metrics
            {
               common::Metric resource;
               common::Metric roundtrip;

               platform::time::point::type requested{};

               Metrics& operator += ( const Metrics& rhs);

               CASUAL_LOG_SERIALIZE(
               { 
                  CASUAL_SERIALIZE( resource);
                  CASUAL_SERIALIZE( roundtrip);
                  CASUAL_SERIALIZE( requested);
               })
            };

            namespace resource
            {
               namespace id
               {
                  using type = common::strong::resource::id;

                  inline bool remote( type id) { return id < common::strong::resource::id{0};}
                  inline bool local( type id) { return id > common::strong::resource::id{0};}

               } // id


               struct Proxy
               {
                  struct Instance
                  {
                     enum class State : short
                     {
                        absent,
                        started,
                        idle,
                        busy,
                        error,
                        shutdown
                     };

                     friend std::ostream& operator << ( std::ostream& out, const State& value);


                     id::type id;
                     common::process::Handle process;

                     Metrics metrics;

                     void state( State state);
                     State state() const;

                     inline friend bool operator == ( const Instance& lhs, common::strong::process::id rhs) { return lhs.process.pid == rhs;}

                     CASUAL_LOG_SERIALIZE(
                     { 
                        CASUAL_SERIALIZE( id);
                        CASUAL_SERIALIZE( process);
                        CASUAL_SERIALIZE( metrics);
                        CASUAL_SERIALIZE_NAME( m_state, "state");
                     })

                  private:
                     State m_state = State::absent;

                  };

                  struct generate_id {};

                  using id_sequence = common::value::id::sequence< id::type>;

                  inline Proxy( generate_id) : id( id_sequence::next()) {}

                  id::type id;

                  std::string key;
                  std::string openinfo;
                  std::string closeinfo;
                  size_type concurency = 0;

                  //! This 'counter' keep track of metrics for removed
                  //! instances, so we can give a better view for the operator.
                  Metrics metrics;

                  std::vector< Instance> instances;

                  std::string name;
                  std::string note;

                  //! @return true if all instances 'has connected'
                  bool booted() const;

                  bool remove_instance( common::strong::process::id pid);

                  inline friend bool operator < ( const Proxy& lhs, const Proxy& rhs) { return lhs.id < rhs.id;}
                  friend bool operator == ( const Proxy& lhs, id::type rhs) { return lhs.id == rhs; }
                  friend bool operator == ( id::type lhs, const Proxy& rhs) { return lhs == rhs.id; }

                  CASUAL_LOG_SERIALIZE(
                  { 
                     CASUAL_SERIALIZE( id);
                     CASUAL_SERIALIZE( key);
                     CASUAL_SERIALIZE( openinfo);
                     CASUAL_SERIALIZE( closeinfo);
                     CASUAL_SERIALIZE( concurency);
                     CASUAL_SERIALIZE( metrics);
                     CASUAL_SERIALIZE( instances);
                     CASUAL_SERIALIZE( name);
                     CASUAL_SERIALIZE( note);
                  })
               };

               namespace external
               {
                  struct Proxy
                  {
                     Proxy( common::process::Handle  process, id::type id);

                     common::process::Handle process;

                     //! RM id
                     id::type id;

                     // TODO maintainance: get metrics for "external" resources
                     // Metrics metrics;

                     friend bool operator == ( const Proxy& lhs, const common::process::Handle& rhs);
                     inline friend bool operator == ( const Proxy& lhs, id::type rhs) { return lhs.id == rhs;}
                  };

                  namespace proxy
                  {
                     id::type id( State& state, const common::process::Handle& process);
                  } // proxy

               } // external
            } // resource

            namespace pending
            {
               struct Request
               {
                  template< typename M>
                  Request( resource::id::type resource, M&& message) 
                     : resource( resource),  message( common::serialize::native::complete< common::communication::ipc::message::Complete>( std::forward< M>( message))) {}

                  resource::id::type resource;
                  common::communication::ipc::message::Complete message;

                  inline friend bool operator == ( const Request& lhs, resource::id::type rhs) { return lhs.resource == rhs;}

                  CASUAL_LOG_SERIALIZE(
                     CASUAL_SERIALIZE( resource);
                     CASUAL_SERIALIZE( message);
                  )
               };

            } // pending
         } // state


         struct Transaction
         {
            //! Holds specific implementations for the current transaction. 
            //! That is, depending on where the task comes from and if there
            //! are some optimizations there are different semantics for 
            //! the transaction.
            struct Dispatch
            {
               common::unique_function< bool( State&, common::message::transaction::resource::prepare::Reply&, Transaction&)> prepare;
               common::unique_function< bool( State&, common::message::transaction::resource::commit::Reply&, Transaction&)> commit;
               common::unique_function< bool( State&, common::message::transaction::resource::rollback::Reply&, Transaction&)> rollback;

               inline explicit operator bool () { return static_cast< bool>( prepare);}
            };

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

               friend std::ostream& operator << ( std::ostream& out, Stage value);

               friend Stage operator + ( Stage lhs, Stage rhs) { return std::max( lhs, rhs);}

               //! Used to rank the return codes from the resources, the lower the enum value (higher up),
               //! the more severe...
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

               inline Resource( id_type id) : id( id) {}

               Resource( Resource&&) noexcept = default;
               Resource& operator = ( Resource&&) noexcept = default;

               id_type id;

               Stage stage = Stage::involved;
               Result result = Result::xa_OK;


               static Result convert( common::code::xa value);
               static common::code::xa convert( Result value);

               void set_result( common::code::xa value);

               //! @return true if there's nothing more to do, hence this resource can be removed
               //!    from the transaction
               bool done() const;

               struct update
               {
                  static inline auto stage( Resource::Stage stage)
                  {
                     return [stage]( Resource& value){ value.stage = stage;};
                  }
               };

               struct filter
               {
                  static inline auto stage( Resource::Stage stage)
                  {
                     return [stage]( const Resource& value){ return value.stage == stage;};
                  }

                  static inline auto result( Resource::Result result)
                  {
                     return [ result]( const Resource& value){ return value.result == result;};
                  }
               };

               inline friend bool operator < ( const Resource& lhs, const Resource& rhs) { return lhs.id < rhs.id; }
               inline friend bool operator == ( const Resource& lhs, const Resource& rhs) { return lhs.id == rhs.id; }
               inline friend bool operator == ( const Resource& lhs, id_type id) { return lhs.id == id; }

               CASUAL_LOG_SERIALIZE(
               { 
                  CASUAL_SERIALIZE( id);
                  CASUAL_SERIALIZE( stage);
                  CASUAL_SERIALIZE( result);
                  CASUAL_SERIALIZE_NAME( done(), "done");
               })
            };

            struct Branch
            {
               Branch( const common::transaction::ID& trid) : trid( trid) {}

               std::vector< common::strong::resource::id> involved() const;
               void involve( common::strong::resource::id resource);

               template< typename R> 
               auto involve( R&& range) -> std::enable_if_t< common::traits::is::iterable< R>::value>
               {
                  for( auto r : range)
                     involve( r);
               }

               //! @return the least progressed stage of all resources associated with this branch
               Resource::Stage stage() const;

               //! @return the most severe result from all the resources
               Resource::Result results() const;

               common::transaction::ID trid;
               std::vector< Resource> resources;

               inline friend bool operator == ( const Branch& lhs, const common::transaction::ID& rhs) { return lhs.trid == rhs;}

               CASUAL_LOG_SERIALIZE(
               { 
                  CASUAL_SERIALIZE( trid);
                  CASUAL_SERIALIZE( resources);
                  CASUAL_SERIALIZE_NAME( results(), "results");
                  CASUAL_SERIALIZE_NAME( stage(), "stage");
               })
            };


            Transaction() = default;
            Transaction( Transaction&&) = default;
            Transaction& operator = ( Transaction&&) = default;

            inline Transaction( const common::transaction::ID& trid) : global( trid)
            {
               branches.emplace_back( trid);
            }

            inline const common::process::Handle& owner() const { return global.trid.owner();}

            platform::size::type resource_count() const noexcept;

            //! the global part of the distributed transaction id
            global::ID global;

            //! 1..* branches of this global transaction
            std::vector< Branch> branches;

            //! Depending on context the transaction will have different
            //! handle-implementations.
            Dispatch implementation;

            platform::time::point::type started;
            platform::time::point::type deadline;

            //! Used to keep track of the origin for commit request.
            common::Uuid correlation;

            //! Indicate if the transaction is owned by a remote domain,
            //! and what RM id that domain act as.
            state::resource::id::type resource;
            
            //! @return the 
            Resource::Stage stage() const;

            //! @return the most severe result from all the resources
            //! in all branches
            common::code::xa results() const;

            inline friend bool operator == ( const Transaction& lhs, const global::ID& rhs) { return lhs.global == rhs;}

            CASUAL_LOG_SERIALIZE(
            { 
               CASUAL_SERIALIZE( global);
               CASUAL_SERIALIZE( branches);
               CASUAL_SERIALIZE( started);
               CASUAL_SERIALIZE( deadline);
               CASUAL_SERIALIZE( correlation);
               CASUAL_SERIALIZE( resource);
               
            })
         };


         struct State
         {
            State( manager::Settings settings, 
               configuration::Model configuration,
               std::vector< configuration::resource::Property> properties);

            State( State&&) = default;
            State& operator = ( State&&) = default;

            std::vector< Transaction> transactions;
            std::vector< state::resource::Proxy> resources;
            std::vector< state::resource::external::Proxy> externals;

            struct Persistent
            {
               inline Persistent( std::string database) : log{ std::move( database)} {}

               //! Replies that will be sent after an atomic write to the log
               std::vector< common::message::pending::Message> replies;

               //! the persistent transaction log
               Log log;

            } persistent;

            struct
            {
               //! Resource request, that will be processed as soon as possible,
               //! that is, as soon as corresponding resources is done/idle
               std::vector< state::pending::Request> requests;

            } pending;

            struct
            {
               std::map< std::string, configuration::resource::Property> properties;
            } resource;


            //! @return true if there are pending stuff to do. We can't block
            //! if we got stuff to do...
            bool outstanding() const;

            //! @return true if all resource proxies is booted
            bool booted() const;

            //! @return number of total instances
            size_type instances() const;

            std::vector< common::strong::process::id> processes() const;

            void operator () ( const common::process::lifetime::Exit& death);

            state::resource::Proxy& get_resource( state::resource::id::type rm);
            state::resource::Proxy& get_resource( const std::string& name);
            const state::resource::Proxy& get_resource( const std::string& name) const;
            state::resource::Proxy* find_resource( const std::string& name);
            state::resource::Proxy::Instance& get_instance( state::resource::id::type rm, common::strong::process::id pid);

            bool remove_instance( common::strong::process::id pid);


            using instance_range = common::range::type_t< std::vector< state::resource::Proxy::Instance>>;
            instance_range idle_instance( state::resource::id::type rm);

            const state::resource::external::Proxy& get_external( state::resource::id::type rm) const;

            common::message::transaction::configuration::alias::Reply configuration(
               const common::message::transaction::configuration::alias::Request& request);

         private:

            struct
            {
               std::map< std::string, std::vector< state::resource::id::type>> configuration;
            } m_alias;
            
         };

         namespace state
         {
            namespace filter
            {
               //! @return a functor that returns true if instance is idle
               inline auto idle()
               {
                  return []( const resource::Proxy::Instance& i){ return i.state() == resource::Proxy::Instance::State::idle;};
               }

               struct Running
               {
                  //! @return true if instance is running
                  inline bool operator () ( const resource::Proxy::Instance& instance) const
                  {
                     return instance.state() == resource::Proxy::Instance::State::idle
                        || instance.state() == resource::Proxy::Instance::State::busy;
                  }

                  //! @return true if at least one instance in resource-proxy is running
                  inline bool operator () ( const resource::Proxy& resource) const
                  {
                     return common::algorithm::any_of( resource.instances, Running{});
                  }
               };

            } // filter

            //! Base that holds the state
            struct Base
            {
               Base( State& state) : m_state( state) {}

            protected:
               State& m_state;

            };

         } // state

      } // manager
   } // transaction
} // casual


