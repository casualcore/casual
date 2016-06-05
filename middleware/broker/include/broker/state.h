//!
//! state.h
//!
//! Created on: Sep 13, 2014
//!     Author: Lazan
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
            enum class State
            {
               idle,
               busy,
            };

            inline void state( State state)
            {
               m_state = state;
               m_last = common::platform::clock_type::now();
            }

            inline State state() const { return m_state;}

            const common::platform::time_point& last() const { return m_last;}

            common::process::Handle process;
            std::size_t invoked = 0;


            friend bool operator == ( const Instance& lhs, const Instance& rhs) { return lhs.process == rhs.process;}
            friend bool operator < ( const Instance& lhs, const Instance& rhs) { return lhs.process.pid < rhs.process.pid;}

         private:
            common::platform::time_point m_last = common::platform::time_point::min();
            State m_state = State::idle;

         };

         struct Service
         {
            struct Instance
            {
               using State = state::Instance::State;

               Instance( state::Instance& instance) : instance{ instance} {}
               std::size_t invoked = 0;
               std::reference_wrapper< state::Instance> instance;

               bool idle() const;
               inline void state( State state) { instance.get().state( state);}
               inline const common::process::Handle& process() const  { return instance.get().process;}

               friend bool operator == ( const Instance& lhs, common::platform::pid::type rhs);
            };


            Service( common::message::Service information) : information( std::move( information)) {}
            Service() {}

            std::vector< Instance> instances;
            common::message::Service information;
            std::size_t lookedup = 0;

            //!
            //! @return an idle instance or nullptr if no one is found.
            //!
            Instance* idle();

            void add( state::Instance& instance);
            void remove( common::platform::pid::type instance);

            friend bool operator == ( const Service& lhs, const Service& rhs) { return lhs.information.name == rhs.information.name;}
            friend bool operator < ( const Service& lhs, const Service& rhs) { return lhs.information.name < rhs.information.name;}

            friend std::ostream& operator << ( std::ostream& out, const Service& service);
         };
      } // state





      struct State
      {


         State() = default;

         State( State&&) = default;
         State& operator = (State&&) = default;

         State( const State&) = delete;


         typedef std::unordered_map< common::platform::pid::type, state::Instance> instance_mapping_type;
         typedef std::unordered_map< std::string, state::Service> service_mapping_type;

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

         void add( common::process::Handle process, std::vector< common::message::Service> services);
         void remove( common::process::Handle process, const std::vector< common::message::Service>& services);

         state::Service* find_service(  const std::string& name);


         //!
         //! finds or adds an instance
         //!
         //! @param process
         //! @return the instance
         state::Instance& find_or_add( common::process::Handle process);

         state::Service& find_or_add( common::message::Service service);


         std::size_t size() const;

         void connect_broker( std::vector< common::message::Service> services);



      private:
      };


   } // broker


} // casual

#endif // STATE_H_
