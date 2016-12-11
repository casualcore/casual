//!
//! casual
//!

#ifndef CASUAL_BROKER_STATE_H_
#define CASUAL_BROKER_STATE_H_



#include "common/message/domain.h"
#include "common/message/server.h"
#include "common/message/gateway.h"
#include "common/message/service.h"
#include "common/message/pending.h"

#include "common/communication/ipc.h"

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
   namespace broker
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
               std::size_t invoked = 0;

               inline const common::platform::time_point& last() const { return m_last;}

               inline friend bool operator == ( const base_instance& lhs, const base_instance& rhs) { return lhs.process == rhs.process;}
               inline friend bool operator < ( const base_instance& lhs, const base_instance& rhs) { return lhs.process.pid < rhs.process.pid;}

            protected:
               common::platform::time_point m_last = common::platform::time_point::min();
            };

            struct Local : base_instance
            {
               enum class State : char
               {
                  idle,
                  busy,
               };

               using base_instance::base_instance;

               void lock( const common::platform::time_point& when);
               void unlock( const common::platform::time_point& when);

               inline State state() const { return m_state;}
               inline bool idle() const { return m_state == State::idle;}

            private:
               State m_state = State::idle;
            };

            struct Remote : base_instance
            {
               using base_instance::base_instance;

               std::size_t order;

               void requested( const common::platform::time_point& when);

               friend bool operator < ( const Remote& lhs, const Remote& rhs);
            };

         } // instance


         namespace service
         {
            namespace pending
            {
               struct Metric
               {
                  inline std::size_t count() const { return m_count;}
                  inline std::chrono::microseconds total() const { return m_total;}

                  void add( const common::platform::time_point::duration& duration);
                  void reset();

               private:
                  std::size_t m_count = 0;
                  std::chrono::microseconds m_total = std::chrono::microseconds::zero();

               };
            } // pending
            struct Metric
            {

               inline std::size_t invoked() const { return m_invoked;}
               inline std::chrono::microseconds total() const { return m_total;}

               void begin( const common::platform::time_point& time);
               void end( const common::platform::time_point& time);

               void reset();

               inline const common::platform::time_point& used() const { return m_begin;}

               Metric& operator += ( const Metric& rhs);

            private:

               std::size_t m_invoked = 0;
               common::platform::time_point m_begin;

               std::chrono::microseconds m_total = std::chrono::microseconds::zero();
            };

            namespace instance
            {
               struct base_instance
               {
                  virtual void lock( const common::platform::time_point& when) = 0;
                  virtual const common::process::Handle& process() const = 0;

                  friend inline bool operator == ( const base_instance& lhs, common::platform::pid::type rhs) { return lhs.process().pid == rhs;}

               protected:
                  ~base_instance() = default;
               };

               struct Local final : std::reference_wrapper< state::instance::Local>, base_instance
               {
                  using base_type = std::reference_wrapper< state::instance::Local>;
                  using base_type::base_type;

                  inline bool idle() const { return get().idle();}

                  inline const common::process::Handle& process() const override { return get().process;}
                  void lock( const common::platform::time_point& when) override;
                  void unlock( const common::platform::time_point& when);


                  Metric metric;

               };

               struct Remote final : std::reference_wrapper< state::instance::Remote>, base_instance
               {
                  using base_type = std::reference_wrapper< state::instance::Remote>;

                  Remote( state::instance::Remote& instance, std::size_t hops) : base_type{ instance}, m_hops{ hops} {}

                  inline const common::process::Handle& process() const override { return get().process;}
                  void lock( const common::platform::time_point& when) override;

                  inline std::size_t hops() const { return m_hops;}

                  std::size_t invoked = 0;

                  friend bool operator < ( const Remote& lhs, const Remote& rhs);

               private:
                  std::size_t m_hops = 0;
               };

            } // instance


            struct Pending
            {
               Pending( common::message::service::lookup::Request::Request&& request, const common::platform::time_point& when)
                : request{ std::move( request)}, when{ when} {}

               common::message::service::lookup::Request request;
               common::platform::time_point when;
            };

         } // service



         struct Service
         {
            Service( common::message::service::call::Service information)
               : information( std::move( information)) {}

            Service() = default;

            template< typename T>
            using instances_type = std::vector< T>;

            struct
            {
               instances_type< service::instance::Local> local;
               instances_type< service::instance::Remote> remote;

               inline bool empty() const { return local.empty() && remote.empty();}

            } instances;

            common::message::service::call::Service information;
            service::Metric metric;

            //!
            //! Keeps track of the pending metrics for this service
            //!
            state::service::pending::Metric pending;

            //!
            //! Resets the metrics
            //!
            void metric_reset();

            //!
            //! @return an idle instance or nullptr if no one is found.
            //!
            service::instance::base_instance* idle();

            void add( state::instance::Local& instance);
            void add( state::instance::Remote& instance, std::size_t hops);

            void remove( common::platform::pid::type instance);

            service::instance::Local& local( common::platform::pid::type instance);


            friend bool operator == ( const Service& lhs, const Service& rhs) { return lhs.information.name == rhs.information.name;}
            friend bool operator < ( const Service& lhs, const Service& rhs) { return lhs.information.name < rhs.information.name;}

            friend std::ostream& operator << ( std::ostream& out, const Service& service);

         private:

            void partition_remote_instances();
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



         struct traffic_t
         {
            struct monitors_t
            {
               void add( common::process::Handle process);
               void remove( common::platform::pid::type pid);

               std::vector< common::platform::ipc::id::type> get() const;

            private:
               std::vector< common::process::Handle> processes;
            } monitors;

         } traffic;


         common::process::Handle forward;



         state::Service& service( const std::string& name);

         void remove_process( common::platform::pid::type pid);


         void update( common::message::service::Advertise& message);
         void update( common::message::gateway::domain::Advertise& mesage);


         //!
         //! find a service from name
         //!
         //! @param name of the service wanted
         //! @return pointer to service, nullptr if not found
         //!
         state::Service* find_service( const std::string& name);

         void connect_broker( std::vector< common::message::service::advertise::Service> services);
      };


   } // broker


} // casual

#endif // STATE_H_
