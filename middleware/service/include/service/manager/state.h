//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/message/service.h"

#include "common/metric.h"
#include "common/server/service.h"
#include "common/communication/ipc.h"
#include "common/event/dispatch.h"
#include "common/state/machine.h"
#include "common/message/coordinate.h"
#include "common/communication/select.h"
#include "common/communication/ipc/send.h"

#include "configuration/model.h"

#include <vector>
#include <string>
#include <unordered_map>
#include <map>
#include <list>
#include <deque>
#include <optional>

namespace casual
{
   namespace service::manager
   {
      namespace state
      {
         using ipc_range_type = common::range::const_type_t< std::vector< common::strong::ipc::id>>;

         struct Service;

         namespace instance
         {
            struct Caller
            {
               common::process::Handle process;
               common::strong::correlation::id correlation;

               inline explicit operator bool() const noexcept { return common::predicate::boolean( process);}
               inline friend bool operator == ( const Caller& lhs, const common::strong::correlation::id& rhs) { return lhs.correlation == rhs;}

               CASUAL_LOG_SERIALIZE(
                  CASUAL_SERIALIZE( process);
                  CASUAL_SERIALIZE( correlation);
               )
            };

            struct base_instance
            {
               base_instance( const common::process::Handle& process) : process( process) {}

               common::process::Handle process;
               std::string alias;

               inline friend bool operator == ( const base_instance& lhs, common::process::compare_equal_to_handle auto rhs) { return lhs.process == rhs;}

               CASUAL_LOG_SERIALIZE(
                  CASUAL_SERIALIZE( process);
               )
            };

            namespace sequential
            {
               enum class State : short
               {
                  idle,
                  busy,
               };
               std::string_view description( State value) noexcept;
               
            } // sequential


            struct Sequential : base_instance
            {
               using base_instance::base_instance;

               void reserve( 
                  state::Service* service,
                  const common::process::Handle& caller,
                  const common::strong::correlation::id& correlation);

               void unreserve( const common::message::event::service::Metric& metric);

               //! discards the reservation
               void discard();

               sequential::State state() const;
               inline bool idle() const { return m_service == nullptr;}

               //! Return true if this instance exposes the service
               bool service( const std::string& name) const;

               //! removes this instance from all services
               //! @returns all associated service names that after the detach has no instances (this instance was the last/only)
               std::vector< std::string> detach();

               void add( state::Service& service);
               void remove( const std::string& service);
               //! removes service from associated 
               void remove( const state::Service* service, state::Service* replacement);

               //! @returns and consumes associated caller to the correlation. 'empty' caller if not found.
               inline instance::Caller consume( const common::strong::correlation::id& correlation)
               {
                  if( m_caller == correlation)
                     return std::exchange( m_caller, {});
                  return {};
               }

               //! caller of last reserve
               inline const auto& caller() const noexcept { return m_caller;}

               //! @returns the name of the reserved service if a reservation exists
               std::optional< std::string> reserved_service() const;

               CASUAL_LOG_SERIALIZE(
                  base_instance::serialize( archive);   
                  CASUAL_SERIALIZE_NAME( m_service, "service");
                  CASUAL_SERIALIZE_NAME( m_caller, "caller");
                  CASUAL_SERIALIZE_NAME( m_services, "services");
               )

            private:
               state::Service* m_service = nullptr;
               instance::Caller m_caller;
               // all associated services
               std::vector< state::Service*> m_services;
            };

            struct Concurrent : base_instance
            {      
               using base_instance::base_instance;

               platform::size::type order{};
               std::string description;
               
               friend bool operator < ( const Concurrent& lhs, const Concurrent& rhs);

               CASUAL_LOG_SERIALIZE(
                  base_instance::serialize( archive);
                  CASUAL_SERIALIZE( order);
                  CASUAL_SERIALIZE( description);
               )

            };

         } // instance


         namespace service
         {
            namespace pending
            {
               struct Lookup
               {
                  Lookup( common::message::service::lookup::Request request, platform::time::point::type when)
                  : request{ std::move( request)}, when{ when} {}

                  common::message::service::lookup::Request request;
                  platform::time::point::type when;

                  inline friend bool operator == ( const Lookup& lhs, const common::strong::correlation::id& rhs) { return lhs.request == rhs;}
                  inline friend bool operator == ( const Lookup& lhs, const std::string& service) { return lhs.request.requested == service;}

                  CASUAL_LOG_SERIALIZE( 
                     CASUAL_SERIALIZE( request);
                     CASUAL_SERIALIZE( when);
                  )
               };

               namespace deadline
               {
                  struct Entry
                  {   
                     platform::time::point::type when;
                     common::strong::correlation::id correlation;
                     //! The actual instance that the timeout refers to.
                     common::strong::process::id target;
                     state::Service* service = nullptr;

                     inline friend bool operator < ( const Entry& lhs, const Entry& rhs) { return lhs.when < rhs.when;}
                     inline friend bool operator == ( const Entry& lhs, const common::strong::correlation::id& rhs) { return lhs.correlation == rhs;}

                     CASUAL_LOG_SERIALIZE( 
                        CASUAL_SERIALIZE( when);
                        CASUAL_SERIALIZE( correlation);
                        CASUAL_SERIALIZE( target);
                        CASUAL_SERIALIZE( service);
                     )
                  };
                  
               } // deadline

               struct Deadline
               {      
                  std::optional< platform::time::point::type> add( deadline::Entry entry);
                  std::optional< platform::time::point::type> remove( const common::strong::correlation::id& correlation);
                  std::optional< platform::time::point::type> remove( const std::vector< common::strong::correlation::id>& correlations);

                  struct Expired
                  {
                     std::vector< deadline::Entry> entries;
                     std::optional< platform::time::point::type> deadline;

                     CASUAL_LOG_SERIALIZE(
                        CASUAL_SERIALIZE( entries);
                        CASUAL_SERIALIZE( deadline);
                     )
                  };

                  Expired expired( platform::time::point::type now = platform::time::point::type::clock::now());

                  CASUAL_LOG_SERIALIZE( 
                     CASUAL_SERIALIZE_NAME( m_entries, "entries");
                  )
                  
               private:
                  std::vector< deadline::Entry> m_entries;
               };
            } // pending

            namespace instance
            {

               using local_base = std::reference_wrapper< state::instance::Sequential>;

               //! Just a helper to simplify usage of the reference
               struct Sequential : local_base
               {
                  using local_base::local_base;

                  inline bool idle() const { return get().idle();}

                  inline auto& reserve(                      
                     state::Service* service,
                     const common::process::Handle& caller,
                     const common::strong::correlation::id& correlation)
                  {
                     get().reserve( service, caller, correlation);
                     return get().process;
                  }

                  inline const common::process::Handle& process() const { return get().process;}
                  inline auto state() const { return get().state();}

                  inline friend bool operator == ( const Sequential& lhs, common::process::compare_equal_to_handle auto rhs) { return lhs.process() == rhs;}

                  CASUAL_LOG_SERIALIZE(
                     get().serialize( archive);
                  )
               };

               using remote_base = std::reference_wrapper< state::instance::Concurrent>;

               //! Just a helper to simplify usage of the reference
               struct Concurrent : remote_base, common::Compare< Concurrent>
               {
                  using Property = common::message::service::concurrent::advertise::service::Property;

                  Concurrent( state::instance::Concurrent& instance, Property property) : remote_base{ instance}, property{ property} {}

                  inline const common::process::Handle& process() const { return get().process;}

                  inline friend bool operator == ( const Concurrent& lhs, common::process::compare_equal_to_handle auto rhs) { return lhs.process() == rhs;}
                  
                  //! for common::Compare. The configured services are 'worth more' than not configured. 
                  inline auto tie() const { return std::tie( property.type, get().order, property.hops);}

                  CASUAL_LOG_SERIALIZE(
                     get().serialize( archive);
                     CASUAL_SERIALIZE( property);
                  )

                  Property property;

               };
            } // instance


            struct Instances
            {
               void add( state::instance::Concurrent& instance, state::service::instance::Concurrent::Property property);

               void remove( const common::strong::ipc::id& ipc, const std::string& service);
               void remove( const common::strong::ipc::id& ipc);

               //! removes service from associated 
               void remove( const state::Service* service, state::Service* replacement);

               inline bool has_sequential() const noexcept { return ! m_sequential.empty();}
               inline bool has_concurrent() const noexcept { return ! m_concurrent.empty();}

               inline bool empty() const noexcept { return m_sequential.empty() && m_concurrent.empty();}
               inline explicit operator bool() const noexcept { return ! empty();}

               //! @returns and consumes associated caller to the correlation. 'empty' caller if not found.
               state::instance::Caller consume( const common::strong::correlation::id& correlation);

               //! @returns the first concurrent instance that in the preferred range
               //!   Otherwise, the first instance, and rotate.
               state::service::instance::Concurrent* next_concurrent( ipc_range_type preferred) noexcept;

               inline const auto& sequential() const noexcept { return m_sequential;}
               inline const auto& concurrent() const noexcept { return m_concurrent;}

               CASUAL_LOG_SERIALIZE(
                  CASUAL_SERIALIZE( m_sequential);
                  CASUAL_SERIALIZE( m_concurrent);
               )

               friend Service;

            private:

               void prioritize() noexcept;

               std::vector< state::service::instance::Sequential> m_sequential;
               std::vector< state::service::instance::Concurrent> m_concurrent;
               common::range::type_t< std::vector< state::service::instance::Concurrent>> m_prioritized_concurrent;
            };

            struct Metric
            {
               common::Metric invoked;

               //! Keeps track of the pending metrics for this service
               common::Metric pending;

               platform::time::point::type last = platform::time::point::limit::zero();

               // remote invocations
               platform::size::type remote = 0;

               inline void reset() { *this = Metric{};}
               void update( const common::message::event::service::Metric& metric);

               friend Metric& operator += ( Metric& lhs, const Metric& rhs);

               CASUAL_LOG_SERIALIZE(
                  CASUAL_SERIALIZE( invoked);
                  CASUAL_SERIALIZE( pending);
                  CASUAL_SERIALIZE( last);
                  CASUAL_SERIALIZE( remote);
               )
            };

         } // service

         struct Service
         {
            // state
            service::Instances instances;
            common::message::service::call::Service information;
            casual::configuration::model::service::Timeout timeout;
            service::Metric metric;
            std::optional< common::service::visibility::Type> visibility;

            void remove( const common::strong::ipc::id& ipc);

            void add( state::instance::Sequential& instance);
            void add( state::instance::Concurrent& instance, state::service::instance::Concurrent::Property property);

            common::process::Handle reserve_sequential( 
               const common::process::Handle& caller, 
               const common::strong::correlation::id& correlation);
            
            //! @return a reserved instance or 'null-handle' if no one is found.
            common::process::Handle reserve_concurrent( ipc_range_type preferred);

            inline bool has_sequential() const noexcept { return instances.has_sequential();}
            inline bool has_concurrent() const noexcept { return instances.has_concurrent();}
            inline bool has_instances() const noexcept { return ! instances.empty();}

            bool is_discoverable() const noexcept;

            bool timeoutable() const noexcept;

            //! @returns the concurrent property of the service, "empty" property if not applicable.
            service::instance::Concurrent::Property property() const noexcept;

            //! @returns and consumes associated caller to the correlation. 'empty' caller if not found.
            inline auto consume( const common::strong::correlation::id& correlation) { return instances.consume( correlation);}

            CASUAL_LOG_SERIALIZE(
               CASUAL_SERIALIZE( information);
               CASUAL_SERIALIZE( instances);
               CASUAL_SERIALIZE( timeout);
               CASUAL_SERIALIZE( metric);
               CASUAL_SERIALIZE( visibility);
            )
         };

         enum struct Runlevel : short
         {
            running,
            shutdown,
            error,
         };
         std::string_view description( Runlevel value);

      } // state

      struct State
      {
         State();

         common::state::Machine< state::Runlevel> runlevel;
         
         common::communication::select::Directive directive;
         common::communication::ipc::send::Coordinator multiplex{ directive};

         //! holds the total known services, including routes
         std::unordered_map< std::string, state::Service> services;

         struct
         {
            template< typename T>
            using instance_mapping_type = std::unordered_map< common::strong::ipc::id, T>;

            instance_mapping_type< state::instance::Sequential> sequential;
            instance_mapping_type< state::instance::Concurrent> concurrent;

            CASUAL_LOG_SERIALIZE(
               CASUAL_SERIALIZE( sequential);
               CASUAL_SERIALIZE( concurrent);
            )

         } instances;


         struct
         {
            state::service::pending::Deadline deadline;
            std::vector< state::service::pending::Lookup> lookups;
            common::message::coordinate::fan::Out< common::message::service::call::ACK, common::strong::process::id> shutdown;
            
            CASUAL_LOG_SERIALIZE(
               CASUAL_SERIALIZE( deadline);
               CASUAL_SERIALIZE( lookups);
               CASUAL_SERIALIZE( shutdown);
            )
         } pending;

         struct
         {
            std::unordered_map< common::transaction::global::ID, std::vector< common::strong::ipc::id>> associations;

            CASUAL_LOG_SERIALIZE(
               CASUAL_SERIALIZE( associations);
            )
         
         } transaction;

         common::event::dispatch::Collection< common::message::event::service::Calls> events;  

         struct Metric 
         {
            using metric_type = common::message::event::service::Metric;

            void add( metric_type metric);
            void add( std::vector< metric_type> metrics);

            inline auto extract() noexcept { return std::exchange( m_message, {});}

            inline auto size() const noexcept { return m_message.metrics.size();}
            inline explicit operator bool() const noexcept { return ! m_message.metrics.empty();}

            CASUAL_LOG_SERIALIZE(
               CASUAL_SERIALIZE_NAME( m_message, "message");
            )

         private:
            common::message::event::service::Calls m_message;
         } metric; 

         common::Process forward;

         // TODO merge this to a timeout { duration, instances} 
         casual::configuration::model::service::Timeout timeout;
         std::vector< common::strong::process::id> timeout_instances;

         //! holds all the routes, for services that has routes
         std::map< std::string, std::vector< std::string>> routes;
         //! the same as above from the route to actual service.
         std::map< std::string, std::string> reverse_routes;

         //! holds all alias restrictions.
         casual::configuration::model::service::Restriction restriction;


         //! @returns true if we're ready to shutdown
         bool done() const noexcept;


         //! find a service from name
         //! @param name of the service wanted
         //! @return pointer to service, nullptr if not found
         //! @{
         [[nodiscard]] state::Service* service( const std::string& name) noexcept;
         [[nodiscard]] const state::Service* service( const std::string& name) const noexcept;
         //! @}

         //! find service from an `origin` service name
         //! @param name of the service wanted
         //! @return pointer to service, nullptr if not found
         //[[nodiscard]] state::Service* origin_service( const std::string& origin);
         
         //! removes the instance (deduced from `pid`) and remove the instance from all services 
         void remove( common::strong::process::id pid);
         void remove( common::strong::ipc::id ipc);


         using prepare_shutdown_result = std::tuple< std::vector< std::string>, std::vector< state::instance::Sequential>, std::vector< common::process::Handle>>;
         //! removes and extract all instances (deduced from `pid`) from all services
         //! @returns a tuple of (origin) services with no instances - extracted sequential instances - unknown processes
         [[nodiscard]] prepare_shutdown_result prepare_shutdown( std::vector< common::process::Handle> processes);

         //! adds or "updates" service
         //! @returns pending request that has got services ready for reply
         //! @{ 
         [[nodiscard]] std::vector< state::service::pending::Lookup> update( common::message::service::Advertise&& message);
         [[nodiscard]] std::vector< state::service::pending::Lookup> update( common::message::service::concurrent::Advertise&& message);
         //! @}

         //! @returns the previously associated "instances" to the `gtrid`, if any.
         std::vector< common::strong::ipc::id> disassociate( common::transaction::global::id::range gtrid);

         //! Resets metrics for the provided services, if empty all metrics are reset.
         //! @param services
         //!
         //! @return the services that was reset.
         std::vector< std::string> metric_reset( std::vector< std::string> services);

         //! @returns a sequential (local) instances that has the `ipc`, or nullptr if absent.
         [[nodiscard]] state::instance::Sequential* sequential( common::strong::ipc::id ipc);

         void connect_manager( std::vector< common::server::Service> services);

         CASUAL_LOG_SERIALIZE(
            CASUAL_SERIALIZE( runlevel);
            CASUAL_SERIALIZE( directive);
            CASUAL_SERIALIZE( multiplex);
            CASUAL_SERIALIZE( services);
            CASUAL_SERIALIZE( pending);
            CASUAL_SERIALIZE( transaction);
            CASUAL_SERIALIZE( events);
            CASUAL_SERIALIZE( metric);
            CASUAL_SERIALIZE( forward);
            CASUAL_SERIALIZE( timeout);
            CASUAL_SERIALIZE( routes);
            CASUAL_SERIALIZE( restriction);
            
         ) 

      };

   } // service::manager
} // casual


