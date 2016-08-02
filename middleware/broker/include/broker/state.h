//!
//! casual
//!

#ifndef CASUAL_BROKER_STATE_H_
#define CASUAL_BROKER_STATE_H_



#include "common/message/domain.h"
#include "common/message/server.h"
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


         struct Instance
         {
            enum class State : char
            {
               idle,
               busy,
            };

            Instance() = default;
            Instance( common::process::Handle process) : process( std::move( process)) {}

            common::process::Handle process;
            std::size_t invoked = 0;

            void lock( const common::platform::time_point& when);
            void unlock( const common::platform::time_point& when);


            inline bool idle() const { return m_state != State::busy;}

            inline State state() const { return m_state;}

            inline const common::platform::time_point& last() const { return m_last;}

            inline friend bool operator == ( const Instance& lhs, const Instance& rhs) { return lhs.process == rhs.process;}
            inline friend bool operator < ( const Instance& lhs, const Instance& rhs) { return lhs.process.pid < rhs.process.pid;}


         private:
            State m_state = State::idle;

            common::platform::time_point m_last = common::platform::time_point::min();

         };



         namespace service
         {
            struct Instance
            {
               using State = state::Instance::State;

               std::reference_wrapper< state::Instance> instance;
               std::size_t invoked = 0;

               Instance( state::Instance& instance, std::size_t hops) : instance{ instance}, m_hops{ hops} {}


               inline bool idle() const { return instance.get().idle();}

               void lock( const common::platform::time_point& when);
               void unlock( const common::platform::time_point& when);

               inline State state() const { return instance.get().state();}
               inline std::size_t hops() const { return m_hops;}
               inline const common::platform::time_point& last() const { return instance.get().last();}

               inline const common::process::Handle& process() const { return instance.get().process;}

               friend inline bool operator == ( const Instance& lhs, common::platform::pid::type rhs) { return lhs.process().pid == rhs;}
               friend inline bool operator < ( const Instance& lhs, const Instance& rhs) { return lhs.hops() < rhs.hops();}

            private:
               std::size_t m_hops = 0;
            };
         } // service



         struct Service
         {


            Service( common::message::service::call::Service information) : information( std::move( information)) {}
            Service() {}

            using instances_type = std::vector< service::Instance>;

            using range_type = common::range::traits< instances_type>::type;

            struct
            {
               range_type local;
               range_type remote;

            } instances;

            common::message::service::call::Service information;
            std::size_t lookedup = 0;

            //!
            //! @return an idle instance or nullptr if no one is found.
            //!
            service::Instance* idle();

            void add( state::Instance& instance, std::size_t hops);

            void remove( common::platform::pid::type instance);

            bool has( common::platform::pid::type instance);


            friend bool operator == ( const Service& lhs, const Service& rhs) { return lhs.information.name == rhs.information.name;}
            friend bool operator < ( const Service& lhs, const Service& rhs) { return lhs.information.name < rhs.information.name;}

            friend std::ostream& operator << ( std::ostream& out, const Service& service);

         private:

            void partition_instances();

            instances_type m_instances;
         };
      } // state





      struct State
      {


         State() = default;

         State( State&&) = default;
         State& operator = (State&&) = default;

         State( const State&) = delete;


         using instance_mapping_type = std::unordered_map< common::platform::pid::type, state::Instance>;
         using service_mapping_type = std::unordered_map< std::string, state::Service>;

         instance_mapping_type instances;
         service_mapping_type services;


         struct
         {
            std::deque< common::message::service::lookup::Request> requests;
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
         state::Instance& instance( common::platform::pid::type pid);

         void remove_process( common::platform::pid::type pid);

         void add( common::message::service::Advertise& message);
         void remove( const common::message::service::Unadvertise& message);


         state::Service* find_service( const std::string& name);



         void connect_broker( std::vector< common::message::service::advertise::Service> services);



      private:
      };


   } // broker


} // casual

#endif // STATE_H_
