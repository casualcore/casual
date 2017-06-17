//!
//! casual
//!

#ifndef CASUAL_SERVICE_MANAGER_STATE_H_
#define CASUAL_SERVICE_MANAGER_STATE_H_



#include "common/message/domain.h"
#include "common/message/server.h"
#include "common/message/gateway.h"
#include "common/message/service.h"
#include "common/message/pending.h"

#include "common/server/service.h"


#include "common/communication/ipc.h"

#include "common/event/dispatch.h"

#include "common/exception.h"

#include "sf/log.h"

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

         namespace internal
         {
            template< typename T>
            struct Id
            {
               using id_type = std::size_t;

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
               struct Missing : public common::exception::base
               {
                  using common::exception::base::base;
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
               };



               struct Local : base_instance
               {
                  enum class State : char
                  {
                     idle,
                     busy,
                     exiting,
                  };

                  using base_instance::base_instance;

                  void reserve( const common::platform::time::point::type& when, state::Service* service);
                  state::Service* unreserve( const common::platform::time::point::type& now);


                  inline const common::platform::time::point::type& last() const { return m_last;}

                  void exiting();

                  State state() const;
                  inline bool idle() const { return m_service == nullptr;}

                  //!
                  //! Return true if this instance exposes the service
                  //!
                  bool service( const std::string& name) const;


                  void add( const state::Service& service);
                  void remove( const std::string& service);


               private:
                  common::platform::time::point::type m_last = common::platform::time::point::type::min();
                  state::Service* m_service = nullptr;

                  using service_view = std::reference_wrapper< const std::string>;
                  std::vector< service_view> m_services;
               };

               struct Remote : base_instance
               {
                  using base_instance::base_instance;

                  std::size_t order;

                  friend bool operator < ( const Remote& lhs, const Remote& rhs);
               };

            } // instance


            namespace service
            {
               struct Metric
               {
                  inline std::size_t count() const { return m_count;}
                  inline std::chrono::microseconds total() const { return m_total;}

                  void add( const std::chrono::microseconds& duration);

                  template< typename R, typename P>
                  void add( const std::chrono::duration< R, P>& duration)
                  {
                     add( std::chrono::duration_cast< std::chrono::microseconds>( duration));
                  }

                  void reset();

               private:
                  std::size_t m_count = 0;
                  std::chrono::microseconds m_total = std::chrono::microseconds::zero();

               };

               struct Pending
               {
                  Pending( common::message::service::lookup::Request&& request, const common::platform::time::point::type& when)
                   : request{ std::move( request)}, when{ when} {}

                  common::message::service::lookup::Request request;
                  common::platform::time::point::type when;
               };


               namespace instance
               {

                  using local_base = std::reference_wrapper< state::instance::Local>;

                  //!
                  //! Just a helper to simplify usage of the reference
                  //!
                  struct Local : local_base
                  {
                     using local_base::local_base;

                     inline bool idle() const { return get().idle();}

                     inline void reserve( const common::platform::time::point::type& when, state::Service* service)
                     {
                        get().reserve( when, service);
                     }

                     inline const common::process::Handle& process() const { return get().process;}
                     inline auto state() const { return get().state();}


                     inline friend bool operator == ( const Local& lhs, common::platform::pid::type rhs) { return lhs.process().pid == rhs;}
                  };

                  using remote_base = std::reference_wrapper< state::instance::Remote>;

                  //!
                  //! Just a helper to simplify usage of the reference
                  //!
                  struct Remote : remote_base
                  {
                     Remote( state::instance::Remote& instance, std::size_t hops) : remote_base{ instance}, m_hops{ hops} {}

                     inline const common::process::Handle& process() const { return get().process;}

                     inline std::size_t hops() const { return m_hops;}

                     inline friend bool operator == ( const Remote& lhs, common::platform::pid::type rhs) { return lhs.process().pid == rhs;}
                     friend bool operator < ( const Remote& lhs, const Remote& rhs);

                  private:
                     std::size_t m_hops = 0;
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
                  instances_type< state::service::instance::Local> local;
                  instances_type< state::service::instance::Remote> remote;

                  inline bool empty() const { return local.empty() && remote.empty();}

                  //!
                  //! @return true if any of the instances is active (not exiting).
                  //!
                  bool active() const;

               } instances;

               common::message::service::call::Service information;
               service::Metric metric;

               //!
               //! Keeps track of the pending metrics for this service
               //!
               service::Metric pending;

               //!
               //! Resets the metrics
               //!
               void metric_reset();

               //!
               //! @return a reserved instance or 'null-handle' if no one is found.
               //!
               common::process::Handle reserve( const common::platform::time::point::type& now);


               void add( state::instance::Local& instance);
               void add( state::instance::Remote& instance, std::size_t hops);

               void remove( common::platform::pid::type instance);

               state::instance::Local& local( common::platform::pid::type instance);


               friend bool operator == ( const Service& lhs, const Service& rhs) { return lhs.information.name == rhs.information.name;}
               friend bool operator < ( const Service& lhs, const Service& rhs) { return lhs.information.name < rhs.information.name;}

               friend std::ostream& operator << ( std::ostream& out, const Service& service);

               inline std::size_t remote_invocations() const { return m_remote_invocations;}
               inline const common::platform::time::point::type& last() const { return m_last;}


            private:

               friend struct state::instance::Local;
               void unreserve( const common::platform::time::point::type& now, const common::platform::time::point::type& then);



               void partition_remote_instances();

               common::platform::time::point::type m_last = common::platform::time::point::type::min();
               std::size_t m_remote_invocations = 0;
            };
         } // state





         struct State
         {


            State() = default;

            State( State&&) = default;
            State& operator = (State&&) = default;

            State( const State&) = delete;


            template< typename T>
            using instance_mapping_type = std::unordered_map< common::platform::pid::type, T>;
            using service_mapping_type = std::unordered_map< std::string, state::Service>;

            service_mapping_type services;

            struct
            {
               instance_mapping_type< state::instance::Local> local;
               instance_mapping_type< state::instance::Remote> remote;

            } instances;


            struct
            {
               std::deque< state::service::Pending> requests;
               std::vector< common::message::pending::Message> replies;
            } pending;


            common::event::dispatch::Collection<
               common::message::event::service::Call> events;

            std::vector< common::platform::ipc::id::type> subscribers() const;


            common::process::Handle forward;

            std::chrono::microseconds default_timeout = std::chrono::microseconds::zero();


            state::Service& service( const std::string& name);

            void remove_process( common::platform::pid::type pid);
            void prepare_shutdown( common::platform::pid::type pid);


            void update( common::message::service::Advertise& message);
            void update( common::message::gateway::domain::Advertise& mesage);


            //!
            //! Resets metrics for the provided services, if empty all metrics are reseted.
            //! @param services
            //!
            //! @return the services that was reseted.
            //!
            std::vector< std::string> metric_reset( std::vector< std::string> services);


            //!
            //! find a service from name
            //!
            //! @param name of the service wanted
            //! @return pointer to service, nullptr if not found
            //!
            state::Service* find_service( const std::string& name);

            state::instance::Local& local( common::platform::pid::type pid);

            void connect_manager( std::vector< common::server::Service> services);




         };

      } // manager

   } // service


} // casual

#endif // STATE_H_
