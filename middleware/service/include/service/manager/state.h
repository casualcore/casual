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

#include "casual/container.h"

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
      struct State;
      
      namespace state
      {
         template< typename Tag>
         struct id_policy
         {
            constexpr static platform::size::type initialize() noexcept { return -1;}
            constexpr static bool valid( platform::size::type value) noexcept { return value != -1;}
         };

         using ipc_range_type = common::range::const_type_t< std::vector< common::strong::ipc::id>>;

         namespace service
         {
            namespace id
            {
               struct Tag{};
               using type = common::strong::Type< platform::size::type, state::id_policy< Tag>>;
            } // id
            
         } // service

         namespace instance
         {
            namespace sequential
            {
               namespace id
               {
                  struct Tag{};
                  using type = common::strong::Type< platform::size::type, state::id_policy< Tag>>;
               } // id

               enum class State : short
               {
                  idle,
                  busy,
               };
               std::string_view description( State value) noexcept;

               
            } // sequential

            namespace concurrent
            {
               namespace id
               {
                  struct Tag{};
                  using type = common::strong::Type< platform::size::type, state::id_policy< Tag>>;
               } // id

               using Property = common::message::service::concurrent::advertise::service::Property;

            } // concurrent
            
            template< typename T>
            constexpr bool is_concurrent = std::same_as< concurrent::id::type, T>;


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
                  CASUAL_SERIALIZE( alias);
               )


            };

            struct Sequential : base_instance
            {
               using base_instance::base_instance;

               void reserve( 
                  service::id::type service,
                  const common::process::Handle& caller,
                  const common::strong::correlation::id& correlation);

               //! unreserve the instance, @return the service that was used
               service::id::type unreserve();

               //! discards the reservation
               void discard();

               sequential::State state() const;
               inline bool idle() const { return ! common::predicate::boolean( m_reserved_service);}

               //! Return true if this instance exposes the service
               bool service( service::id::type service) const;

               void add( service::id::type service);
               void remove( service::id::type service);

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
               inline service::id::type reserved_service() const noexcept { return m_reserved_service;}

               const auto& services() const noexcept { return m_services;}

               CASUAL_LOG_SERIALIZE(
                  base_instance::serialize( archive);
                  CASUAL_SERIALIZE( m_reserved_service);
                  CASUAL_SERIALIZE( m_caller);
                  CASUAL_SERIALIZE( m_services);
               )

            private:
               service::id::type m_reserved_service{};
               instance::Caller m_caller;
               // all associated services
               std::vector< service::id::type> m_services;
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
                     instance::sequential::id::type target;
                     service::id::type service;

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

                  deadline::Entry* find_entry( const common::strong::correlation::id& correlation);

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

            namespace instances
            {

               struct Concurrent
               {
                  instance::concurrent::id::type id;
                  platform::size::type order{};
                  platform::size::type hops{};

                  inline friend bool operator < ( const Concurrent& lhs, const Concurrent& rhs) { return std::tie( lhs.order, lhs.hops) < std::tie( rhs.order, rhs.hops);}
                  inline friend bool operator == ( const Concurrent& lhs, instance::concurrent::id::type rhs) { return lhs.id == rhs;}

                  CASUAL_LOG_SERIALIZE(
                     CASUAL_SERIALIZE( id);
                     CASUAL_SERIALIZE( order);
                     CASUAL_SERIALIZE( hops);
                  )
               };
               
            } // instances
          
            struct Instances
            {
               void add( state::instance::sequential::id::type instance);
               void add( state::instance::concurrent::id::type instance, platform::size::type order, state::instance::concurrent::Property property);

               //! @return true if there are no instances left after the removal
               bool remove( instance::sequential::id::type id);
               //! @return true if there are no instances left after the removal
               bool remove( instance::concurrent::id::type id);


               inline bool has_sequential() const noexcept { return ! m_sequential.empty();}
               inline bool has_concurrent() const noexcept { return ! m_concurrent.empty();}

               inline bool empty() const noexcept { return m_sequential.empty() && m_concurrent.empty();}
               inline explicit operator bool() const noexcept { return ! empty();}

               //! @returns the first concurrent instance that in the preferred range
               //!   Otherwise, the first instance, and rotate.
               instance::concurrent::id::type next_concurrent( std::span< instance::concurrent::id::type> preferred) noexcept;

               void update_prioritized();

               inline const auto& sequential() const noexcept { return m_sequential;}
               inline const auto& concurrent() const noexcept { return m_concurrent;}

               CASUAL_LOG_SERIALIZE(
                  CASUAL_SERIALIZE( m_sequential);
                  CASUAL_SERIALIZE( m_concurrent);
               )

            private:

               void prioritize() noexcept;

               std::vector< instance::sequential::id::type> m_sequential;
               std::vector< instances::Concurrent> m_concurrent;
               std::span< instances::Concurrent> m_prioritized_concurrent;
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
            service::Instances instances{};
            common::message::service::call::Service information;
            casual::configuration::model::service::Timeout timeout{};
            common::service::transaction::Type transaction = common::service::transaction::Type::automatic;
            std::optional< common::service::visibility::Type> visibility;
            std::string category{};
            
            inline bool has_sequential() const noexcept { return instances.has_sequential();}
            inline bool has_concurrent() const noexcept { return instances.has_concurrent();}
            inline bool has_instances() const noexcept { return ! instances.empty();}
            inline bool is_concurrent_only() const noexcept { return has_concurrent() && ! has_sequential();}

            bool is_discoverable() const noexcept;

            bool timeoutable() const noexcept;

            //! @returns the concurrent property of the service, "empty" property if not applicable.
            instance::concurrent::Property property() const noexcept;

            //! @returns and consumes associated caller to the correlation. 'empty' caller if not found.
            //inline auto consume( const common::strong::correlation::id& correlation) { return instances.consume( correlation);}

            //! equal to service name
            inline friend bool operator == ( const Service& lhs, const std::string& name) noexcept { return lhs.information.name == name;}

            CASUAL_LOG_SERIALIZE(
               CASUAL_SERIALIZE( information);
               CASUAL_SERIALIZE( instances);
               CASUAL_SERIALIZE( timeout);
               CASUAL_SERIALIZE( visibility);
               CASUAL_SERIALIZE( category);
               
            )
         };

         //! holder for service lookup, service metric and such
         struct Services
         {
            
            //! insert the serves (if not present), add service name as the lookup (service without routes)
            state::service::id::type insert( state::Service service);
            //! insert the serves (if not present), add routes to lookup for the service
            state::service::id::type insert( state::Service service, const std::vector< std::string>& routes);

            //! @attention slow - only use during configuration
            void erase( state::service::id::type id);
            //! remove all current lookup names to the `id` and add the service.name as a lookup
            //! @attention slow - only use during configuration
            void restore_origin_name( state::service::id::type id);
            //! replaces all lookup to the `id` 
            //! @attention slow - only use during configuration
            void replace_routes( state::service::id::type id, const std::vector< std::string>& routes);

            state::service::id::type lookup( const std::string& name) const;
            //! lookup the service by using the origin name the service was advertised as
            //! @attention slow - only use during configuration
            state::service::id::type lookup_origin( const std::string& name) const;

            //! @returns all names for the service
            std::vector< std::string> names( state::service::id::type id) const;

            inline state::Service& operator [] ( state::service::id::type id) & { return m_services[ id];}
            inline const state::Service& operator [] ( state::service::id::type id) const & { return m_services[ id];}

            service::Metric& metric( const std::string& service);
            inline auto& metrics() const { return m_metrics;}

            inline auto indexes() const { return m_services.indexes();}
            inline bool contains( state::service::id::type id) const { return m_services.contains( id);}

            inline void for_each( auto callback) const
            {
               for( auto& pair : m_lookup)
                  callback( pair.second, pair.first, m_services[ pair.second]);
            }

            inline void for_each( auto callback)
            {
               for( auto& pair : m_lookup)
                  callback( pair.second, pair.first, m_services[ pair.second]);
            }

            CASUAL_LOG_SERIALIZE(
               CASUAL_SERIALIZE( m_lookup);
               CASUAL_SERIALIZE( m_services);
               CASUAL_SERIALIZE( m_metrics);
            )

            friend struct manager::State;

         private:
            //! holds the name -> id mapping, for all exposed services, including routes
            //! several names can refer to the same service-id
            std::unordered_map< std::string, state::service::id::type> m_lookup;

            //! holds the actual services
            casual::container::Index< state::Service, state::service::id::type> m_services;

            std::unordered_map< std::string, service::Metric> m_metrics;
         };

         struct Instances
         {
            casual::container::lookup_index< instance::Sequential, instance::sequential::id::type, common::strong::ipc::id> sequential;
            casual::container::lookup_index< instance::Concurrent, instance::concurrent::id::type, common::strong::ipc::id> concurrent;

            void remove_service( instance::sequential::id::type instance_id, service::id::type service_id);
            inline void remove_service( instance::concurrent::id::type, service::id::type) { /*no op*/}

            CASUAL_LOG_SERIALIZE(
               CASUAL_SERIALIZE( sequential);
               CASUAL_SERIALIZE( concurrent);
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
         state::Services services;

         state::Instances instances;

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
            std::unordered_map< common::transaction::global::ID, std::vector< state::instance::concurrent::id::type>> associations;

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
         //! TODO this should be with correlation instead of instance
         std::vector< common::strong::process::id> timeout_instances;

         //! instances that is in "shutdown" mode. We just keep track of them here
         //! until they're gone to prevent advertising new services and such
         std::vector< state::instance::sequential::id::type> disabled;

         //! holds all the routes, for services that has routes
         std::map< std::string, std::vector< std::string>> routes;
         //! the same as above from the route to actual service.
         std::map< std::string, std::string> reverse_routes;

         //! holds all alias restrictions.
         casual::configuration::model::service::Restriction restriction;


         //! @returns true if we're ready to shutdown
         bool done() const noexcept;
         
         //! removes the instance (deduced from `pid`) and remove the instance from all services 
         //! @returns possible callers to that waits for reply from the removed instance (in practice 0..1)
         [[nodiscard]] std::vector< state::instance::Caller> remove( common::strong::process::id pid);
         //! @returns possible caller to the remove instance
         std::vector< state::instance::Caller> remove( common::strong::ipc::id ipc);

         //! Tries to reserve a sequential instance for the given `service`
         //! @return id of the instance, or 'nil-id' if no idle is found
         state::instance::sequential::id::type reserve_sequential( 
            state::service::id::type service,
            const common::process::Handle& caller, 
            const common::strong::correlation::id& correlation);
            
         //! @return a reserved instance for the given `service` 
         //!   or 'nil-id' if no one is found.
         state::instance::concurrent::id::type reserve_concurrent( 
            state::service::id::type service,
            std::span< state::instance::concurrent::id::type> preferred);

         void unreserve( state::instance::sequential::id::type instance, const common::message::event::service::Metric& metric);


         struct prepare_shutdown_result
         {
            std::vector< std::string> services;
            std::vector< state::instance::sequential::id::type> instances;
            std::vector< common::process::Handle> unknown;

            CASUAL_LOG_SERIALIZE(
               CASUAL_SERIALIZE( services);
               CASUAL_SERIALIZE( instances);
               CASUAL_SERIALIZE( unknown);
            )
         };

         //! removes and extract all instances (deduced from `pid`) from all services
         [[nodiscard]] prepare_shutdown_result prepare_shutdown( std::vector< common::process::Handle> processes);

         //! adds or "updates" service
         //! @returns pending request that has got services ready for reply
         //! @{ 
         [[nodiscard]] std::vector< state::service::pending::Lookup> update( common::message::service::Advertise&& message);
         [[nodiscard]] std::vector< state::service::pending::Lookup> update( common::message::service::concurrent::Advertise&& message);
         //! @}

         //! @returns the previously associated "instances" to the `gtrid`, if any.
         std::vector< state::instance::concurrent::id::type> disassociate( common::transaction::global::id::range gtrid);

         //! Resets metrics for the provided services, if empty all metrics are reset.
         //! @param services
         //!
         //! @return the services that was reset.
         std::vector< std::string> metric_reset( std::vector< std::string> services);

         void connect_manager( std::vector< common::server::Service> services);

         CASUAL_LOG_SERIALIZE(
            CASUAL_SERIALIZE( runlevel);
            CASUAL_SERIALIZE( directive);
            CASUAL_SERIALIZE( multiplex);
            CASUAL_SERIALIZE( services);
            CASUAL_SERIALIZE( instances);
            CASUAL_SERIALIZE( pending);
            CASUAL_SERIALIZE( transaction);
            CASUAL_SERIALIZE( events);
            CASUAL_SERIALIZE( metric);
            CASUAL_SERIALIZE( forward);
            CASUAL_SERIALIZE( timeout);
            CASUAL_SERIALIZE( routes);
            CASUAL_SERIALIZE( disabled);
            CASUAL_SERIALIZE( restriction);
            
         ) 

      };

   } // service::manager
} // casual


