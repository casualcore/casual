//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "transaction/manager/log.h"

#include "casual/platform.h"
#include "common/message/transaction.h"
#include "common/algorithm.h"
#include "common/metric.h"
#include "common/functional.h"
#include "common/message/coordinate.h"
#include "common/state/machine.h"
#include "common/transaction/global.h"

#include "common/communication/select.h"
#include "common/communication/ipc/send.h"
#include "common/communication/ipc/pending.h"

#include "common/serialize/native/complete.h"

#include "configuration/model.h"

#include <map>
#include <deque>
#include <vector>


namespace casual
{
   namespace transaction::manager
   {
      struct State;

      namespace state
      {
         struct Metrics
         {
            common::Metric resource;
            common::Metric roundtrip;

            Metrics& operator += ( const Metrics& rhs);

            CASUAL_LOG_SERIALIZE(
               CASUAL_SERIALIZE( resource);
               CASUAL_SERIALIZE( roundtrip);
            )
         };

         namespace resource
         {
            namespace id
            {
               inline bool remote( common::strong::resource::id id) { return id < common::strong::resource::id{0};}
               inline bool local( common::strong::resource::id id) { return id > common::strong::resource::id{0};}

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
                  friend std::string_view description( State value) noexcept;

                  
                  void reserve();
                  //! used when requests has been pending.
                  void reserve( platform::time::point::type requested);
                  void unreserve( const common::message::Statistics& statistics);

                  common::strong::resource::id id;
                  common::process::Handle process;

                  void state( State state);
                  State state() const;

                  inline const Metrics& metrics() const noexcept { return m_metrics;}
                  inline const common::Metric& pending() const noexcept { return m_pending;}

                  inline friend bool operator == ( const Instance& lhs, common::strong::process::id rhs) { return lhs.process.pid == rhs;}
                  inline friend bool operator == ( const Instance& lhs, State rhs) { return lhs.state() == rhs;}

                  CASUAL_LOG_SERIALIZE(
                     CASUAL_SERIALIZE( id);
                     CASUAL_SERIALIZE( process);
                     CASUAL_SERIALIZE( m_state);
                     CASUAL_SERIALIZE( m_reserved);
                     CASUAL_SERIALIZE( m_metrics);
                     CASUAL_SERIALIZE( m_pending);
                  )

               private:
                  void general_reserve( platform::time::point::type now);

                  State m_state = State::absent;
                  platform::time::point::type m_reserved{};
                  Metrics m_metrics;
                  common::Metric m_pending;
               };

               inline Proxy( configuration::model::transaction::Resource configuration)
                  : configuration{ std::move( configuration)} {}

               common::strong::resource::id id = common::strong::resource::id::generate();

               std::vector< Instance> instances;
               configuration::model::transaction::Resource configuration;

               //! This 'counters' keep track of metrics for removed
               //! instances, so we can give a better view for the operator.
               Metrics metrics;
               common::Metric pending;


               //! @return true if all instances 'has connected'
               bool booted() const;

               bool remove_instance( common::strong::process::id pid);

               inline friend bool operator < ( const Proxy& lhs, const Proxy& rhs) { return lhs.id < rhs.id;}
               friend bool operator == ( const Proxy& lhs, common::strong::resource::id rhs) { return lhs.id == rhs; }
               friend bool operator == ( common::strong::resource::id lhs, const Proxy& rhs) { return lhs == rhs.id; }

               CASUAL_LOG_SERIALIZE(
                  CASUAL_SERIALIZE( id);
                  CASUAL_SERIALIZE( configuration);
                  CASUAL_SERIALIZE( instances);
                  CASUAL_SERIALIZE( metrics);
                  CASUAL_SERIALIZE( pending);
               )
            };

            namespace external
            {
               struct Proxy
               {
                  Proxy( common::process::Handle process, common::strong::resource::id id);

                  common::process::Handle process;

                  //! RM id
                  common::strong::resource::id id;

                  // TODO maintainence: get metrics for "external" resources
                  // Metrics metrics;

                  inline friend bool operator == ( const Proxy& lhs, const common::strong::ipc::id& rhs) { return lhs.process == rhs;}
                  inline friend bool operator == ( const Proxy& lhs, common::strong::resource::id rhs) { return lhs.id == rhs;}

                  CASUAL_LOG_SERIALIZE(
                     CASUAL_SERIALIZE( process);
                     CASUAL_SERIALIZE( id);
                  )
               };

               namespace proxy
               {
                  common::strong::resource::id id( State& state, const common::process::Handle& process);
               } // proxy
            } // external
         } // resource


         namespace transaction
         {
            namespace branch
            {
               struct Resource
               {
                  Resource( common::strong::resource::id id) : id{ id} {}

                  common::strong::resource::id id;
                  //! the 'code' is only used to include in the _admin state_, Hence, 
                  //! we keep this state only for the user.
                  common::code::xa code = common::code::xa::ok;

                  inline friend bool operator == ( const Resource& lhs, common::strong::resource::id rhs) { return lhs.id == rhs;}

                  CASUAL_LOG_SERIALIZE(
                     CASUAL_SERIALIZE( id);
                     CASUAL_SERIALIZE( code);
                  )
               };
            } // branch

            struct Branch
            {
               Branch( const common::transaction::ID& trid) : trid( trid) {}

               inline void involve( common::strong::resource::id id)
               {
                  if( ! common::algorithm::find( resources, id))
                     resources.emplace_back( id);
               }

               template< concepts::range R> 
               void involve( R&& range)
               {
                  common::algorithm::append_unique( range, resources);
               }

               inline void remove( common::strong::resource::id id) { common::algorithm::container::erase( resources, id);}

               void failed( common::strong::resource::id resource);

               common::transaction::ID trid;
               std::vector< branch::Resource> resources;

               inline friend bool operator == ( const Branch& lhs, const common::transaction::ID& rhs) { return lhs.trid == rhs;}

               CASUAL_LOG_SERIALIZE(
                  CASUAL_SERIALIZE( trid);
                  CASUAL_SERIALIZE( resources);
               )
            };

            enum struct Stage : std::int8_t 
            {
               involved,
               prepare,
               post_prepare, // done with prepare. Other domain in charge.
               commit,
               rollback
            };
            std::string_view description( Stage value) noexcept;
            
         } // transaction

         struct Transaction
         {
            Transaction( Transaction&&) = default;
            Transaction& operator = ( Transaction&&) = default;

            inline explicit Transaction( const common::transaction::ID& trid) 
               : global( common::transaction::id::range::global( trid))
            {
               branches.emplace_back( trid);
            }

            platform::size::type resource_count() const noexcept;
            
            //! removes all branches that has no resources associated. 
            //! used only when the prepare/commit/rollback starts
            void purge();

            //! remove all resources associated with branches that is in `replies`
            template< typename R>
            void purge( const R& replies)
            {
               for( auto& reply : replies)
                  if( auto found = common::algorithm::find( branches, reply.trid))
                     found->remove( reply.resource);

               purge();
            }

            void failed( common::strong::resource::id resource);

            common::state::Machine< transaction::Stage> stage;

            //! the global part of the distributed transaction id
            common::transaction::global::ID global;

            //! 1..* branches of this global transaction
            std::vector< transaction::Branch> branches;

            common::process::Handle owner;

            platform::time::point::type started;
            platform::time::point::type deadline;



            inline friend bool operator == ( const Transaction& lhs, const common::transaction::global::ID& rhs) { return lhs.global == rhs;}
            inline friend bool operator == ( const Transaction& lhs, common::transaction::id::range::type::global rhs) { return lhs.global == rhs;}
            
            inline friend bool operator == ( const Transaction& lhs, const common::transaction::ID& rhs) { return lhs.global == rhs;}

            CASUAL_LOG_SERIALIZE(
               CASUAL_SERIALIZE( stage);
               CASUAL_SERIALIZE( global);
               CASUAL_SERIALIZE( branches);
               CASUAL_SERIALIZE( started);
               CASUAL_SERIALIZE( deadline);
            )
         };

         namespace pending
         {
            //! type to help hold pending request to re
            struct Request
            {
               Request() = default;
               inline Request( common::communication::ipc::message::Complete complete)
                  : created{ platform::time::clock::type::now()}, complete{ std::move( complete)} 
               {}

               inline explicit operator bool() const noexcept { return common::predicate::boolean( complete);}

               platform::time::point::type created{};
               common::communication::ipc::message::Complete complete;

               inline const auto& correlation() const noexcept { return complete.correlation();}

               CASUAL_LOG_SERIALIZE(
                  CASUAL_SERIALIZE( created);
                  CASUAL_SERIALIZE( complete);
               )
            };
         } // pending

      } // state

      struct State
      {
         common::communication::select::Directive directive;
         common::communication::ipc::send::Coordinator multiplex{ directive};

         std::vector< state::Transaction> transactions;
         std::vector< state::resource::Proxy> resources;
         std::vector< state::resource::external::Proxy> externals;

         struct
         {
            template< typename Reply>
            using Coordinate = common::message::coordinate::fan::Out< Reply, common::strong::resource::id>;
            
            Coordinate< common::message::transaction::resource::prepare::Reply> prepare;   
            Coordinate< common::message::transaction::resource::commit::Reply> commit;
            Coordinate< common::message::transaction::resource::rollback::Reply> rollback;

            inline void failed( common::strong::resource::id id) { prepare.failed( id); commit.failed( id); rollback.failed( id);}

            CASUAL_LOG_SERIALIZE( 
               CASUAL_SERIALIZE( prepare);
               CASUAL_SERIALIZE( commit);
               CASUAL_SERIALIZE( rollback);
            )
         } coordinate;

         struct
         {
            //! Replies that will be sent after an atomic write to the log
            common::communication::ipc::pending::Holder replies;

            //! the persistent transaction log
            Log log;

            CASUAL_LOG_SERIALIZE(
               CASUAL_SERIALIZE( replies);
               CASUAL_SERIALIZE( log);
            )

         } persistent;

         struct
         {
            //! Resource request, that will be processed as soon as possible,
            //! that is, as soon as corresponding resources is done/idle
            common::communication::ipc::pending::basic_holder< common::strong::resource::id, state::pending::Request> requests;

            CASUAL_LOG_SERIALIZE(
               CASUAL_SERIALIZE( requests);
            )

         } pending;


         struct
         {
            configuration::model::system::Model configuration;

            CASUAL_LOG_SERIALIZE(
               CASUAL_SERIALIZE( configuration);
            )
         } system;
         

         struct 
         {
            std::map< std::string, std::vector< common::strong::resource::id>> configuration;

            CASUAL_LOG_SERIALIZE(
               CASUAL_SERIALIZE( configuration);
            )
         } alias;


         //! @return true if all resource proxies is booted
         bool booted() const;

         //! @return number of total instances
         platform::size::type instances() const;

         std::vector< common::strong::process::id> processes() const;

         state::resource::Proxy& get_resource( common::strong::resource::id rm);
         state::resource::Proxy& get_resource( const std::string& name);
         const state::resource::Proxy& get_resource( const std::string& name) const;
         state::resource::Proxy* find_resource( common::strong::resource::id rm);
         state::resource::Proxy* find_resource( const std::string& name);
         state::resource::Proxy::Instance& get_instance( common::strong::resource::id rm, common::strong::process::id pid);

         bool remove_instance( common::strong::process::id pid);

         //! @return a reserved instance, nullptr if all are busy.
         state::resource::Proxy::Instance* try_reserve( common::strong::resource::id rm);

         const state::resource::external::Proxy& get_external( common::strong::resource::id rm) const;
         const state::resource::external::Proxy* find_external( common::strong::resource::id rm) const noexcept;

         common::message::transaction::configuration::alias::Reply configuration(
            const common::message::transaction::configuration::alias::Request& request);

         configuration::model::transaction::Model configuration() const;

         CASUAL_LOG_SERIALIZE(
            CASUAL_SERIALIZE( transactions);
            CASUAL_SERIALIZE( resources);
            CASUAL_SERIALIZE( externals);
            CASUAL_SERIALIZE( persistent);
            CASUAL_SERIALIZE( system);
            CASUAL_SERIALIZE( alias);
         )
      };
      static_assert( std::is_move_assignable_v< State> && std::is_move_constructible_v< State>, "State needs to be movable");


   } // transaction::manager
} // casual


