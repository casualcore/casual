//!
//! state.cpp
//!
//! Created on: Sep 13, 2014
//!     Author: Lazan
//!

#include "broker/state.h"

#include "common/internal/log.h"
#include "common/internal/trace.h"
#include "common/exception.h"

#include "common/ipc.h"
#include "common/process.h"

namespace casual
{

   namespace broker
   {
      using namespace common;


      void state::Server::Instance::remove( const state::Service& service)
      {
         auto found = range::find( services, service);

         if( found)
         {
            services.erase( found.first);
         }
      }

      void state::Service::remove( const Server::Instance& instance)
      {
         auto found = range::find( instances, instance);

         if( found)
         {
            instances.erase( found.first);
         }

      }


      bool operator == ( const state::Group& lhs, const state::Group& rhs) { return lhs.id == rhs.id;}

      bool operator < ( const state::Group& lhs, const state::Group& rhs)
      {
         if( lhs.dependencies.empty() && ! lhs.dependencies.empty()) { return true;}
         if( ! lhs.dependencies.empty() && lhs.dependencies.empty()) { return false;}

         if( common::range::find( lhs.dependencies, rhs.id)) { return true;}

         return false;
      }


      void state::Executable::remove( pid_type instance)
      {
         auto found = range::find( instances, instance);

         if( found)
         {
            instances.erase( found.first);
         }

      }


      state::Group& State::getGroup( state::Group::id_type id)
      {
         auto found = common::range::find( groups, id);

         if( ! found)
         {
            throw state::exception::Missing{ "group id: " + std::to_string( id) + " is not known"};
         }
         return *found;
      }


      state::Service& State::getService( const std::string& name)
      {
         auto found = common::range::find( services, name);

         if( ! found)
         {
            throw state::exception::Missing{ "service '" + name + "' is not known"};
         }
         return found->second;
      }

      void State::addServices( state::Server::pid_type pid, std::vector< state::Service> services)
      {
         auto& instance = getInstance( pid);

         for( auto&& s : services)
         {
            //
            // If we dont't have the service, it will be added
            //
            auto& service = this->services.emplace( s.information.name, std::move( s)).first->second;

            service.instances.push_back( instance);
            instance.services.push_back( service);
         }
      }

      void State::removeServices( state::Server::pid_type pid, std::vector< state::Service> services)
      {
         auto& instance = getInstance( pid);

         for( auto&& s : services)
         {
            instance.remove( s);

            auto current = common::range::find( this->services, s.information.name);

            if( current)
            {
               this->services.erase( current.first);
            }
         }
      }

      state::Server::Instance& State::getInstance( state::Server::pid_type pid)
      {
         auto found = common::range::find( instances, pid);

         if( ! found)
         {
            throw state::exception::Missing{ "instance with pid: " + std::to_string( pid) + " is not known"};
         }

         return found->second;
      }

      state::Server::Instance& State::addInstance( state::Server::Instance instance)
      {
         return instances.emplace( instance.pid, std::move( instance)).first->second;
      }



      void State::removeInstance( state::Server::pid_type pid)
      {

         auto found = common::range::find( instances, pid);

         if( found)
         {
            auto& instance = found->second;

            for( auto& s : instance.services)
            {
               auto& service = getService( s.get().information.name);

               service.remove( instance);
            }
         }

         instances.erase( found.first);
      }


      state::Server& State::getServer( state::Server::id_type id)
      {
         auto found = common::range::find( servers, id);

         if( ! found)
         {
            throw state::exception::Missing{ "server  id: " + std::to_string( id) + " is not known"};
         }
         return found->second;
      }



      void State::instance( state::Server& server, std::size_t instances)
      {
         if( instances > server.instances.size())
         {
            boot( server, instances - server.instances.size());
         }
         else
         {
            shutdown( server, server.instances.size() - instances);
         }
      }

      void State::instance( state::Server::id_type id, std::size_t instances)
      {
         auto found = common::range::find( servers, id);

         if( found)
         {
            instance( found->second, instances);
         }
      }


      void State::boot( state::Server& server, std::size_t instances)
      {
         while( instances-- != 0)
         {
            auto pid = common::process::spawn( server.path, server.arguments);

            state::Server::Instance instance;
            instance.pid = pid;
            instance.server = server.id;
            instance.alterState( state::Server::Instance::State::prospect);

            server.instances.push_back( pid);

            this->instances.emplace( pid, std::move( instance));
            //result.push_back( std::move( instance));
         }

      }

      void State::shutdown( state::Server& server, std::size_t instances)
      {
         assert( server.instances.size() >= instances);

         auto range = common::range::make( server.instances);

         range.last = range.last - instances;


         for( auto& pid : range)
         {
            auto&& instance = this->instances.at( pid);
            common::process::terminate( instance.pid);

            instance.alterState( state::Server::Instance::State::shutdown);
         }
      }

      namespace policy
      {

         void Broker::apply()
         {
            try
            {
               throw;
            }
            catch( const exception::signal::child::Terminate& exception)
            {
               auto terminated = process::lifetime::ended();
               for( auto& death : terminated)
               {

                  switch( death.why)
                  {
                     case process::lifetime::Exit::Why::core:
                        log::error << "process crashed " << death.string() << std::endl;

                        break;
                     default:
                        log::information << "proccess died: " << death.string() << std::endl;
                        break;
                  }

                  //action::remove::instance( death.pid, m_state);
               }
            }
         }



         void Broker::clean( platform::pid_type pid)
         {
            auto findIter = m_state.instances.find( pid);

            if( findIter != std::end( m_state.instances))
            {
               trace::Exit remove
               { "remove ipc" };
               ipc::remove( findIter->second.queue_id);

            }

         }
      } // policy
   } // broker

} // casual
