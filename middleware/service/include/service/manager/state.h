//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "common/message/domain.h"
#include "common/message/gateway.h"
#include "common/message/service.h"
#include "common/message/pending.h"

#include "common/metric.h"
#include "common/server/service.h"
#include "common/communication/ipc.h"
#include "common/event/dispatch.h"

#include <vector>
#include <string>
#include <unordered_map>
#include <map>
#include <list>
#include <deque>

namespace casual
{
   namespace service
   {
      namespace manager
      {
         using size_type = platform::size::type;

         namespace internal
         {
            template< typename T>
            struct Id
            {
               using id_type = size_type;

               id_type id = nextId();

            private:
               inline static id_type nextId()
               {
                  static id_type id = 10;
                  return id++;
               }
            };

         } // internal


         namespace state
         {

            struct Service;

            namespace instance
            {
               struct base_instance
               {
                  base_instance() = default;
                  base_instance( common::process::Handle process) : process( std::move( process)) {}

                  common::process::Handle process;

                  inline friend bool operator == ( const base_instance& lhs, const base_instance& rhs) { return lhs.process == rhs.process;}
                  inline friend bool operator < ( const base_instance& lhs, const base_instance& rhs) { return lhs.process.pid < rhs.process.pid;}
                  inline friend bool operator == ( const base_instance& lhs, common::strong::process::id rhs) { return lhs.process.pid == rhs;}

                  CASUAL_LOG_SERIALIZE({
                     CASUAL_SERIALIZE( process);
                  })
               };


               struct Sequential : base_instance
               {
                  enum class State : short
                  {
                     idle,
                     busy,
                  };

                  using base_instance::base_instance;

                  void reserve( 
                     state::Service* service,
                     const common::process::Handle& caller,
                     const common::Uuid& correlation);

                  state::Service* unreserve( const common::message::event::service::Metric& metric);

                  //! discards the reservation
                  void discard();

                  State state() const;
                  inline bool idle() const { return m_service == nullptr;}

                  //! Return true if this instance exposes the service
                  bool service( const std::string& name) const;

                  //! removes this instance from all services
                  void deactivate();

                  void add( state::Service& service);
                  void remove( const std::string& service);

                  //! caller and correlation of last reserve
                  //! @{
                  inline const common::process::Handle& caller() const { return m_caller;}
                  inline const common::Uuid& correlation() const { return m_correlation;}
                  //! @}

                  friend std::ostream& operator << ( std::ostream& out, State value);

                  CASUAL_LOG_SERIALIZE(
                  {
                     base_instance::serialize( archive);   
                     CASUAL_SERIALIZE_NAME( m_service, "service");
                     CASUAL_SERIALIZE_NAME( m_caller, "caller");
                     CASUAL_SERIALIZE_NAME( m_correlation, "correlation");
                     CASUAL_SERIALIZE_NAME( m_services, "services");
                  })

               private:
                  state::Service* m_service = nullptr;
                  common::process::Handle m_caller;
                  common::Uuid m_correlation;

                  // all associated services
                  std::vector< state::Service*> m_services;
               };

               struct Concurrent : base_instance
               {
                  using base_instance::base_instance;

                  size_type order;

                  friend bool operator < ( const Concurrent& lhs, const Concurrent& rhs);

                  CASUAL_LOG_SERIALIZE(
                  {
                     base_instance::serialize( archive);   
                     CASUAL_SERIALIZE( order);
                  })
               };

            } // instance


            namespace service
            {
               using Metric = common::Metric;

               struct Pending
               {
                  Pending( common::message::service::lookup::Request request, platform::time::point::type when)
                   : request{ std::move( request)}, when{ when} {}

                  common::message::service::lookup::Request request;
                  platform::time::point::type when;

                  CASUAL_LOG_SERIALIZE(
                  { 
                     CASUAL_SERIALIZE( request);
                     CASUAL_SERIALIZE( when);
                  })
               };


               namespace instance
               {

                  using local_base = std::reference_wrapper< state::instance::Sequential>;

                  //! Just a helper to simplify usage of the reference
                  struct Sequential : local_base
                  {
                     using local_base::local_base;

                     inline bool idle() const { return get().idle();}

                     inline void reserve(                      
                        state::Service* service,
                        const common::process::Handle& caller,
                        const common::Uuid& correlation)
                     {
                        get().reserve( service, caller, correlation);
                     }

                     inline const common::process::Handle& process() const { return get().process;}
                     inline auto state() const { return get().state();}

                     inline friend bool operator == ( const Sequential& lhs, common::strong::process::id rhs) { return lhs.process().pid == rhs;}

                     CASUAL_LOG_SERIALIZE(
                        get().serialize( archive);
                     )
                  };

                  using remote_base = std::reference_wrapper< state::instance::Concurrent>;

                  //! Just a helper to simplify usage of the reference
                  struct Concurrent : remote_base
                  {
                     Concurrent( state::instance::Concurrent& instance, size_type hops) : remote_base{ instance}, m_hops{ hops} {}

                     inline const common::process::Handle& process() const { return get().process;}

                     inline size_type hops() const { return m_hops;}

                     inline friend bool operator == ( const Concurrent& lhs, common::strong::process::id rhs) { return lhs.process().pid == rhs;}
                     friend bool operator < ( const Concurrent& lhs, const Concurrent& rhs);

                     CASUAL_LOG_SERIALIZE(
                        get().serialize( archive);
                        CASUAL_SERIALIZE_NAME( m_hops, "hops");
                     )
                     

                  private:
                     size_type m_hops = 0;
                  };
               } // instance


               struct Advertised
               {
                  inline Advertised( common::message::service::call::Service information)
                     : information( std::move( information)) {}

                  struct Instances
                  {
                     template< typename T>
                     using instances_type = std::vector< T>;

                     instances_type< state::service::instance::Sequential> sequential;
                     instances_type< state::service::instance::Concurrent> concurrent;

                     inline bool empty() const { return sequential.empty() && concurrent.empty();}

                     inline void partition() { common::algorithm::stable_sort( concurrent);}

                     CASUAL_LOG_SERIALIZE(
                        CASUAL_SERIALIZE( sequential);
                        CASUAL_SERIALIZE( concurrent);
                     )
                  };

                  // state
                  Instances instances;
                  common::message::service::call::Service information;

                  void remove( common::strong::process::id instance);
                  state::instance::Sequential& sequential( common::strong::process::id instance);

                  CASUAL_LOG_SERIALIZE(
                  { 
                     CASUAL_SERIALIZE( information);
                     CASUAL_SERIALIZE( instances);
                  })
               };

            } // service

            struct Service : service::Advertised
            {
               using service::Advertised::Advertised;

               struct Metric
               {
                  service::Metric invoked;

                  //! Keeps track of the pending metrics for this service
                  service::Metric pending;

                  platform::time::point::type last = platform::time::point::limit::zero();

                  // remote invocations
                  size_type remote = 0;

                  inline void reset() { *this = Metric{};}
                  void update( const common::message::event::service::Metric& metric);

                  CASUAL_LOG_SERIALIZE(
                  { 
                     CASUAL_SERIALIZE( invoked);
                     CASUAL_SERIALIZE( pending);
                     CASUAL_SERIALIZE( last);
                     CASUAL_SERIALIZE( remote);
                  })                  
               };

               void add( state::instance::Sequential& instance);
               void add( state::instance::Concurrent& instance, size_type hops);

               Metric metric;

               //! @return a reserved instance or 'null-handle' if no one is found.
               common::process::Handle reserve( 
                  const common::process::Handle& caller, 
                  const common::Uuid& correlation);

               CASUAL_LOG_SERIALIZE(
                  service::Advertised::serialize( archive);
                  CASUAL_SERIALIZE( metric);
               )

            };
         } // state



         struct State
         {

            State( common::message::domain::configuration::service::Manager configuration);

            State( State&&) = default;
            State& operator = (State&&) = default;

            State( const State&) = delete;

            //! holds the total known services, including routes
            std::unordered_map< std::string, state::Service> services;

            struct
            {
               template< typename T>
               using instance_mapping_type = std::unordered_map< common::strong::process::id, T>;

               instance_mapping_type< state::instance::Sequential> sequential;
               instance_mapping_type< state::instance::Concurrent> concurrent;

            } instances;


            struct
            {
               std::deque< state::service::Pending> requests;
            } pending;

            common::event::dispatch::Collection< common::message::event::service::Calls> events;

            struct Metric 
            {
               using metric_type = common::message::event::service::Metric;

               void add( metric_type metric);
               void add( std::vector< metric_type> metrics);

               inline auto& message() const noexcept { return m_message;}
               inline void clear() { m_message.metrics.clear();}

               inline auto size() const noexcept { return m_message.metrics.size();}
               inline explicit operator bool() const noexcept { return ! m_message.metrics.empty();}

            private:
               common::message::event::service::Calls m_message;
            } metric;        

            common::process::Handle forward;

            platform::time::unit default_timeout = platform::time::unit::zero();

            //! holds all the routes, for services that has routes
            std::map< std::string, std::vector< std::string>> routes;

            state::Service& service( const std::string& name);
            
            //! removes the instance (deduced from `pid`) and remove the instance from all services 
            void remove( common::strong::process::id pid);

            //! removes the instance (deduced from `pid`) from all services, (keeps the instance though)
            void deactivate( common::strong::process::id pid);

            //! adds or "updates" service
            //! @{ 
            void update( common::message::service::Advertise& message);
            void update( common::message::service::concurrent::Advertise& message);
            //! @}

            //! Resets metrics for the provided services, if empty all metrics are reseted.
            //! @param services
            //!
            //! @return the services that was reset.
            std::vector< std::string> metric_reset( std::vector< std::string> services);

            //! find a service from name
            //!
            //! @param name of the service wanted
            //! @return pointer to service, nullptr if not found
            state::Service* find_service( const std::string& name);

            //! @returns a sequential (local) instances that has the `pid`, or nullptr if absent.
            state::instance::Sequential& sequential( common::strong::process::id pid);

            void connect_manager( std::vector< common::server::Service> services);

            
            CASUAL_LOG_SERIALIZE(
            {
               CASUAL_SERIALIZE( routes);
               CASUAL_SERIALIZE( services);
            }) 

         };

      } // manager
   } // service
} // casual


