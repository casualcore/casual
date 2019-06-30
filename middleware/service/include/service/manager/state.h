//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once




#include "common/message/domain.h"
#include "common/message/server.h"
#include "common/message/gateway.h"
#include "common/message/service.h"
#include "common/message/pending.h"

#include "common/metric.h"

#include "common/server/service.h"


#include "common/communication/ipc.h"

#include "common/event/dispatch.h"

#include "common/exception/system.h"

#include "serviceframework/log.h"

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
         using size_type = common::platform::size::type;

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

            namespace exception
            {
               struct Missing : public common::exception::system::invalid::Argument
               {
                  using common::exception::system::invalid::Argument::Argument;
               };

            }

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

                  CASUAL_CONST_CORRECT_SERIALIZE_WRITE({
                     CASUAL_NAMED_VALUE( process);
                  })
               };



               struct Sequential : base_instance
               {
                  enum class State : short
                  {
                     idle,
                     busy,
                     exiting,
                  };

                  using base_instance::base_instance;

                  void reserve( 
                     const common::platform::time::point::type& when, 
                     state::Service* service,
                     const common::process::Handle& caller,
                     const common::Uuid& correlation);

                  state::Service* unreserve( const common::platform::time::point::type& now);

                  //! discards the reservation
                  void discard();


                  inline const common::platform::time::point::type& last() const { return m_last;}

                  void exiting();

                  State state() const;
                  inline bool idle() const { return m_service == nullptr;}

                  //! Return true if this instance exposes the service
                  bool service( const std::string& name) const;


                  void add( const state::Service& service);
                  void remove( const std::string& service);

                  //! caller and correlation of last reserve
                  //! @{
                  inline const common::process::Handle& caller() const { return m_caller;}
                  inline const common::Uuid& correlation() const { return m_correlation;}
                  //! @}

                  friend std::ostream& operator << ( std::ostream& out, State value);

                  CASUAL_CONST_CORRECT_SERIALIZE_WRITE(
                  {
                     base_instance::serialize( archive);   
                     CASUAL_NAMED_VALUE_NAME( m_last, "last");
                     CASUAL_NAMED_VALUE_NAME( m_service, "service");
                     CASUAL_NAMED_VALUE_NAME( m_caller, "caller");
                     CASUAL_NAMED_VALUE_NAME( m_correlation, "correlation");
                     CASUAL_NAMED_VALUE_NAME( m_services, "services");
                  })

               private:
                  common::platform::time::point::type m_last = common::platform::time::point::type::min();
                  state::Service* m_service = nullptr;
                  common::process::Handle m_caller;
                  common::Uuid m_correlation;

                  using service_view = std::reference_wrapper< const std::string>;
                  std::vector< service_view> m_services;
               };

               struct Concurrent : base_instance
               {
                  using base_instance::base_instance;

                  size_type order;

                  friend bool operator < ( const Concurrent& lhs, const Concurrent& rhs);

                  CASUAL_CONST_CORRECT_SERIALIZE_WRITE(
                  {
                     base_instance::serialize( archive);   
                     CASUAL_NAMED_VALUE( order);
                  })
               };

            } // instance


            namespace service
            {
               using Metric = common::Metric;

               struct Pending
               {
                  Pending( common::message::service::lookup::Request&& request, const common::platform::time::point::type& when)
                   : request{ std::move( request)}, when{ when} {}

                  common::message::service::lookup::Request request;
                  common::platform::time::point::type when;


                  CASUAL_CONST_CORRECT_SERIALIZE_WRITE(
                  { 
                     CASUAL_NAMED_VALUE( request);
                     CASUAL_NAMED_VALUE( when);
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
                        const common::platform::time::point::type& when, 
                        state::Service* service,
                        const common::process::Handle& caller,
                        const common::Uuid& correlation)
                     {
                        get().reserve( when, service, caller, correlation);
                     }

                     inline const common::process::Handle& process() const { return get().process;}
                     inline auto state() const { return get().state();}


                     inline friend bool operator == ( const Sequential& lhs, common::strong::process::id rhs) { return lhs.process().pid == rhs;}
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

                  private:
                     size_type m_hops = 0;
                  };
               } // instance
            } // service

            struct Service
            {
               Service( common::message::service::call::Service information)
                  : information( std::move( information)) {}

               Service() = default;

               template< typename T>
               using instances_type = std::vector< T>;

               struct Instances
               {
                  instances_type< state::service::instance::Sequential> sequential;
                  instances_type< state::service::instance::Concurrent> concurrent;

                  inline bool empty() const { return sequential.empty() && concurrent.empty();}

                  //! @return true if any of the instances is active (not exiting).
                  bool active() const;

                  CASUAL_CONST_CORRECT_SERIALIZE_WRITE(
                  { 
                     CASUAL_NAMED_VALUE( sequential);
                     CASUAL_NAMED_VALUE( concurrent);
                  })

               } instances;

               common::message::service::call::Service information;
               service::Metric metric;

               //! Keeps track of the pending metrics for this service
               service::Metric pending;

               //! Resets the metrics
               void metric_reset();

               //! @return a reserved instance or 'null-handle' if no one is found.
               common::process::Handle reserve( 
                  const common::platform::time::point::type& now, 
                  const common::process::Handle& caller, 
                  const common::Uuid& correlation);


               void add( state::instance::Sequential& instance);
               void add( state::instance::Concurrent& instance, size_type hops);

               void remove( common::strong::process::id instance);

               state::instance::Sequential& local( common::strong::process::id instance);


               friend bool operator == ( const Service& lhs, const Service& rhs) { return lhs.information.name == rhs.information.name;}
               friend bool operator < ( const Service& lhs, const Service& rhs) { return lhs.information.name < rhs.information.name;}


               inline size_type remote_invocations() const { return m_remote_invocations;}

               inline const common::platform::time::point::type& last() const { return m_last;}
               inline void last( common::platform::time::point::type now) { m_last = now;}


               CASUAL_CONST_CORRECT_SERIALIZE_WRITE(
               { 
                  CASUAL_NAMED_VALUE( instances);
                  CASUAL_NAMED_VALUE( information);
                  CASUAL_NAMED_VALUE( pending);
                  CASUAL_NAMED_VALUE_NAME( m_last, "last");
                  CASUAL_NAMED_VALUE_NAME( m_remote_invocations, "remote_invocations");
               })

            private:

               friend struct state::instance::Sequential;
               void unreserve( const common::platform::time::point::type& now, const common::platform::time::point::type& then);



               void partition_remote_instances();

               common::platform::time::point::type m_last = common::platform::time::point::type::min();
               size_type m_remote_invocations = 0;
            };
         } // state



         struct State
         {

            State() = default;

            State( State&&) = default;
            State& operator = (State&&) = default;

            State( const State&) = delete;


            template< typename T>
            using instance_mapping_type = std::unordered_map< common::strong::process::id, T>;
            using service_mapping_type = std::unordered_map< std::string, state::Service>;

            service_mapping_type services;

            struct
            {
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

            common::platform::time::unit default_timeout = common::platform::time::unit::zero();


            state::Service& service( const std::string& name);

            void remove_process( common::strong::process::id pid);
            void prepare_shutdown( common::strong::process::id pid);


            void update( common::message::service::Advertise& message);
            void update( common::message::service::concurrent::Advertise& message);

            //! Resets metrics for the provided services, if empty all metrics are reseted.
            //! @param services
            //!
            //! @return the services that was reseted.
            std::vector< std::string> metric_reset( std::vector< std::string> services);

            //! find a service from name
            //!
            //! @param name of the service wanted
            //! @return pointer to service, nullptr if not found
            state::Service* find_service( const std::string& name);

            state::instance::Sequential& local( common::strong::process::id pid);

            void connect_manager( std::vector< common::server::Service> services);
         };

      } // manager

   } // service


} // casual


