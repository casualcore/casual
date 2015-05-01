//!
//! state.h
//!
//! Created on: Sep 13, 2014
//!     Author: Lazan
//!

#ifndef CASUAL_BROKER_STATE_H_
#define CASUAL_BROKER_STATE_H_


#include "common/queue.h"

#include "common/message/server.h"

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
            struct Missing : public common::exception::Base
            {
               using common::exception::Base::Base;
            };

         }

         struct Group : internal::Id< Group>
         {
            Group() = default;
            Group( std::string name, std::string note = "") : name( std::move( name)), note( std::move( note)) {}

            struct Resource : internal::Id< Resource>
            {
               Resource() = default;
               //Resource( Resource&&) = default;

               Resource( std::size_t instances, const std::string& key, const std::string& openinfo, const std::string& closeinfo)
               : instances{ instances}, key{ key}, openinfo{ openinfo}, closeinfo{ closeinfo} {}


               std::size_t instances;
               std::string key;
               std::string openinfo;
               std::string closeinfo;

               CASUAL_CONST_CORRECT_SERIALIZE({
                  archive & CASUAL_MAKE_NVP( instances);
                  archive & CASUAL_MAKE_NVP( key);
                  archive & CASUAL_MAKE_NVP( openinfo);
                  archive & CASUAL_MAKE_NVP( closeinfo);
               })
            };


            std::string name;
            std::string note;

            std::vector< Resource> resources;
            std::vector< id_type> dependencies;

            CASUAL_CONST_CORRECT_SERIALIZE({
               archive & CASUAL_MAKE_NVP( id);
               archive & CASUAL_MAKE_NVP( name);
               archive & CASUAL_MAKE_NVP( note);
               archive & CASUAL_MAKE_NVP( resources);
               archive & CASUAL_MAKE_NVP( dependencies);
            })

            friend bool operator == ( const Group& lhs, const Group& rhs);
            friend bool operator == ( const Group& lhs, Group::id_type id) { return lhs.id == id;}
            friend bool operator == ( Group::id_type id, const Group& rhs) { return id == rhs.id;}

            friend bool operator < ( const Group& lhs, const Group& rhs);
         };


         struct Executable : internal::Id< Executable>
         {
            typedef common::platform::pid_type pid_type;

            std::string alias;
            std::string path;
            std::vector< std::string> arguments;
            std::string note;

            std::vector< pid_type> instances;

            std::vector< Group::id_type> memberships;

            struct Environment
            {
               std::vector< std::string> variables;
            } environment;


            std::size_t configuredInstances = 0;
            bool restart = false;

            bool remove( pid_type instance);

         };


         struct Service;

         struct Server : Executable
         {
            typedef common::platform::pid_type pid_type;

            struct Instance
            {

               enum class State
               {
                  booted = 1,
                  idle,
                  busy,
                  shutdown
               };

               void alterState( State state)
               {
                  this->state = state;
                  last = common::platform::clock_type::now();
               }


               common::process::Handle process;
               std::size_t invoked = 0;
               common::platform::time_point last;
               Server::id_type server = 0;
               std::vector< std::reference_wrapper< Service>> services;

               void remove( const Service& service);

               friend bool operator == ( const Instance& lhs, const Instance& rhs) { return lhs.process == rhs.process;}
               friend bool operator < ( const Instance& lhs, const Instance& rhs) { return lhs.process.pid < rhs.process.pid;}

            //private:
               State state = State::booted;
            };

            using Executable::Executable;


            //!
            //! if not empty, only these services are allowed to be publish
            //!
            std::vector< std::string> restrictions;
         };


         struct Service
         {
            Service( const std::string& name) : information( name) {}

            Service() {}

            common::message::Service information;
            std::size_t lookedup = 0;
            std::vector< std::reference_wrapper< Server::Instance>> instances;

            void remove( const Server::Instance& instance);

            friend bool operator == ( const Service& lhs, const Service& rhs) { return lhs.information.name == rhs.information.name;}

            friend std::ostream& operator << ( std::ostream& out, const Service& service);
         };
      } // state





      struct State
      {
         State() = default;

         State( State&&) = default;
         State& operator = (State&&) = default;

         State( const State&) = delete;


         typedef std::unordered_map< state::Server::id_type, state::Server> server_mapping_type;
         typedef std::unordered_map< state::Executable::id_type, state::Executable> executable_mapping_type;
         typedef std::unordered_map< state::Server::pid_type, state::Server::Instance> instance_mapping_type;
         typedef std::unordered_map< std::string, state::Service> service_mapping_type;
         typedef std::deque< common::message::service::name::lookup::Request> pending_requests_type;


         server_mapping_type servers;
         instance_mapping_type instances;
         service_mapping_type services;
         executable_mapping_type executables;
         std::vector< state::Group> groups;

         state::Group::id_type  casual_group_id = 0;


         pending_requests_type pending;

         struct Standard
         {
            state::Service service;

            std::vector< std::string> environment;

         } standard;



         struct traffic_t
         {
            std::vector< common::platform::queue_id_type> monitors;
         } traffic;

         common::platform::queue_id_type transactionManagerQueue = 0;


         state::Group& getGroup( state::Group::id_type id);

         state::Service& getService( const std::string& name);
         void addServices( state::Server::pid_type pid, std::vector< state::Service> services);
         void removeServices( state::Server::pid_type pid, std::vector< state::Service> services);

         state::Server::Instance& getInstance( state::Server::pid_type pid);
         const state::Server::Instance& getInstance( state::Server::pid_type pid) const;


         void removeProcess( state::Server::pid_type pid);


         void addInstances( state::Executable::id_type id, const std::vector< state::Server::pid_type>& pids);

         state::Server::Instance& add( state::Server::Instance instance);
         state::Service& add( state::Service service);
         state::Server& add( state::Server server);
         state::Executable& add( state::Executable executable);

         state::Server& getServer( state::Server::id_type id);

         state::Executable& getExecutable( state::Executable::id_type id);

         struct Batch
         {
            std::string group;
            std::vector< std::reference_wrapper< state::Server>> servers;
            std::vector< std::reference_wrapper< state::Executable>> executables;
         };

         std::vector< Batch> bootOrder();


         std::size_t size() const;

         std::vector< common::platform::pid_type> processes() const;

         std::vector< common::platform::pid_type> instance( state::Server::id_type id, std::size_t instance);
         std::vector< common::platform::pid_type> instance( state::Server& server, std::size_t instance);

      private:
         std::vector< common::platform::pid_type> boot( state::Server& server, std::size_t instances);
         std::vector< common::platform::pid_type> shutdown( state::Server& server, std::size_t instances);

      };


      namespace state
      {
         struct Base
         {
            Base( State& state) : m_state( state) {};

         protected:
            State& m_state;
         };

      } // state


      namespace queue
      {
         namespace blocking
         {
            using Reader = common::queue::blocking::remove::basic_reader< State>;
            using Writer = common::queue::blocking::remove::basic_writer< State>;

         } // blocking

         namespace non_blocking
         {
            using Reader = common::queue::non_blocking::remove::basic_reader< State>;
            using Writer = common::queue::non_blocking::remove::basic_writer< State>;
            using Send = common::queue::non_blocking::remove::basic_send< State>;

         } // non_blocking

      } // queue
   } // broker


} // casual

#endif // STATE_H_
