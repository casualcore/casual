//!
//! state.cpp
//!
//! Created on: Sep 13, 2014
//!     Author: Lazan
//!

#include "broker/state.h"
#include "broker/filter.h"

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


      namespace local
      {
         namespace
         {
            template< typename M, typename ID>
            auto get( M& map, ID&& id) -> decltype( map.at( id))
            {
               auto found = common::range::find( map, id);

               if( ! found)
               {
                  throw state::exception::Missing{ "id: " + std::to_string( id) + " is not known"};
               }
               return found->second;
            }


            void add( state::Executable& executable, const std::vector< state::Server::pid_type>& pids)
            {
               executable.instances.insert( std::end( executable.instances), std::begin( pids), std::end( pids));

               executable.instances = range::to_vector( range::unique( range::sort( executable.instances)));
            }

         } // <unnamed>
      } // local


      namespace state
      {

         void Server::Instance::remove( const state::Service& service)
         {
            auto found = range::find( services, service);

            if( found)
            {
               services.erase( found.first);
            }
         }

         void Service::remove( const Server::Instance& instance)
         {
            auto found = range::find( instances, instance);

            if( found)
            {
               instances.erase( found.first);
            }

         }


         bool operator == ( const Group& lhs, const Group& rhs) { return lhs.id == rhs.id;}

         bool operator < ( const Group& lhs, const Group& rhs)
         {
            if( lhs.dependencies.empty() && ! lhs.dependencies.empty()) { return true;}
            if( ! lhs.dependencies.empty() && lhs.dependencies.empty()) { return false;}

            if( common::range::find( rhs.dependencies, lhs.id)) { return true;}

            return false;
         }


         bool Executable::remove( pid_type instance)
         {
            auto found = range::find( instances, instance);

            if( found)
            {
               instances.erase( found.first);
               return true;
            }
            return false;
         }

      } // state


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
         return local::get( instances, pid);
      }

      const state::Server::Instance& State::getInstance( state::Server::pid_type pid) const
      {
         return local::get( instances, pid);
      }

      state::Server::Instance& State::add( state::Server::Instance instance)
      {
         return instances.emplace( instance.process.pid, std::move( instance)).first->second;
      }


      void State::removeProcess( state::Server::pid_type pid)
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

            auto& server = getServer( instance.server);

            server.remove( pid);

            instances.erase( found.first);
         }
         else
         {
            for( auto& executable : executables)
            {
               if( executable.second.remove( pid))
               {
                  return;
               }
            }
         }
      }


      void State::addInstances( state::Executable::id_type id, const std::vector< state::Server::pid_type>& pids)
      {
         try
         {
            auto& server = local::get( servers, id);

            for( auto&& pid : pids)
            {
               state::Server::Instance instance;
               instance.process.pid = pid;
               instance.server = server.id;
               instance.alterState( state::Server::Instance::State::prospect);

               instances.emplace( pid, std::move( instance));
            }

            local::add( server, pids);

         }
         catch( const state::exception::Missing&)
         {
            //
            // We try the regular executables
            //
            auto& executable = local::get( executables, id);

            local::add( executable, pids);

         }
      }

      state::Service& State::add( state::Service service)
      {
         return services.emplace( service.information.name, std::move( service)).first->second;
      }

      state::Server& State::add( state::Server server)
      {
         return servers.emplace( server.id, std::move( server)).first->second;
      }

      state::Executable& State::add( state::Executable executable)
      {
         return executables.emplace( executable.id, std::move( executable)).first->second;
      }

      state::Server& State::getServer( state::Server::id_type id)
      {
         return local::get( servers, id);
      }

      state::Executable& State::getExecutable( state::Executable::id_type id)
      {
         return local::get( executables, id);
      }


      std::vector< State::Batch> State::bootOrder()
      {
         std::vector< State::Batch> result;

         State::Batch normalized;

         for( auto& s : servers)
         {
            normalized.servers.emplace_back( s.second);
         }

         for( auto& e : executables)
         {
            normalized.executables.emplace_back( e.second);
         }


         range::stable_sort( groups);

         auto serverSet = range::make( normalized.servers);
         auto executableSet = range::make( normalized.executables);

         for( auto&& group : groups)
         {
            auto serverBatch = range::stable_partition( serverSet, filter::group::Id{ group.id});
            auto executableBatch = range::stable_partition( executableSet, filter::group::Id{ group.id});

            if( serverBatch || executableBatch)
            {
               result.push_back( { group.name, range::to_vector( serverBatch), range::to_vector( executableBatch)});
               serverSet = serverSet - serverBatch;
               executableSet = executableSet - executableBatch;
            }
         }

         if( serverSet || executableSet)
         {
            result.push_back( { "<no group>", range::to_vector( serverSet), range::to_vector( executableSet)});
         }

         return result;
      }

      std::size_t State::size() const
      {
         return instances.size();
      }


      std::vector< common::platform::pid_type> State::processes() const
      {
         std::vector< common::platform::pid_type> result;
         for( auto& exe : executables)
         {
            range::copy( exe.second.instances, std::back_inserter( result));
         }
         for( auto& server : servers)
         {
            range::copy( server.second.instances, std::back_inserter( result));
         }
         return result;
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
            instance.process.pid = pid;
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
            common::process::terminate( instance.process.pid);

            instance.alterState( state::Server::Instance::State::shutdown);
         }
      }


   } // broker

} // casual
