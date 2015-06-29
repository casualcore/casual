//!
//! state.cpp
//!
//! Created on: Sep 13, 2014
//!     Author: Lazan
//!

#include "broker/state.h"
#include "broker/filter.h"

#include "common/server/service.h"
#include "common/server/lifetime.h"
#include "common/internal/log.h"
#include "common/internal/trace.h"
#include "common/exception.h"
#include "common/algorithm.h"

#include "common/ipc.h"
#include "common/process.h"



#include <functional>

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

         std::ostream& operator << ( std::ostream& out, const Service& service)
         {
            return out << "{ name: " << service.information.name
                  << ", type: " << service.information.type
                  << ", transaction: " << service.information.transaction
                  << ", timeout: " << service.information.timeout.count()
                  << "}";
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

         //
         // Check if we should filter restricted services
         //
         {
            auto& server = getServer( instance.server);

            if( ! server.restrictions.empty())
            {
               auto intersection = std::get< 0>( common::range::intersection( services, server.restrictions,
                     []( const state::Service& s, const std::string& name)
                     {
                        return s.information.name == name;
                     }));
               //
               // Remove
               //
               range::trim( services, intersection);
            }
         }


         for( auto& s : services)
         {
            //
            // If we dont't have the service, it will be added
            //
            auto inserted = this->services.emplace( s.information.name, s);

            auto& service = inserted.first->second;

            common::log::internal::debug << "service: " << service << std::endl;

            if( inserted.second)
            {
               if( service.information.type == common::server::Service::Type::cCasualAdmin)
               {
                  service.information.timeout = std::chrono::microseconds::zero();
               }
               else
               {
                  service.information.timeout = standard.service.information.timeout;
               }
            }
            else
            {
               service.information.type = s.information.type;
               service.information.transaction = s.information.transaction;
            }

            service.instances.push_back( instance);
            range::sort( service.instances);
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
               current->second.remove( instance);
               //this->services.erase( current.first);
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

            //
            // Try to remove the ipc-resource
            //
            common::ipc::remove( instance.process);

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
               instance.alterState( state::Server::Instance::State::booted);

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
         auto inserted = services.emplace( service.information.name, std::move( service));

         if( inserted.second)
         {


         }

         return inserted.first->second;

         //return services.emplace( service.information.name, std::move( service)).first->second;
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

            auto serverPartition = range::stable_partition( serverSet, filter::group::Id{ group.id});
            auto executablePartition = range::stable_partition( executableSet, filter::group::Id{ group.id});

            auto serverBatch = std::get< 0>( serverPartition);
            auto executableBatch = std::get< 0>( executablePartition);

            if( serverBatch || executableBatch)
            {
               result.push_back( { group.name, range::to_vector( serverBatch), range::to_vector( executableBatch)});
               serverSet = std::get< 1>( serverPartition);
               executableSet = std::get< 1>( executablePartition);
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

      std::vector< common::platform::pid_type> State::instance( state::Server& server, std::size_t instances)
      {
         if( instances > server.instances.size())
         {
            return boot( server, instances - server.instances.size());
         }
         else
         {
            return shutdown( server, server.instances.size() - instances);
         }
      }

      std::vector< common::platform::pid_type> State::instance( state::Server::id_type id, std::size_t instances)
      {
         auto found = common::range::find( servers, id);

         if( found)
         {
            return instance( found->second, instances);
         }
         return {};
      }


      std::vector< common::platform::pid_type> State::boot( state::Server& server, std::size_t instances)
      {
         std::vector< common::platform::pid_type> pids( instances);

         for( auto& pid : pids)
         {
            pid = common::process::spawn( server.path, server.arguments);

            state::Server::Instance instance;
            instance.process.pid = pid;
            instance.server = server.id;
            instance.alterState( state::Server::Instance::State::booted);

            server.instances.push_back( pid);

            this->instances.emplace( pid, std::move( instance));
         }
         return pids;

      }

      std::vector< common::platform::pid_type> State::shutdown( state::Server& server, std::size_t instances)
      {
         assert( server.instances.size() >= instances);

         auto range = common::range::make( server.instances);

         range.first = range.last - instances;

         std::vector< common::process::Handle> servers;

         for( auto& pid : range)
         {
            //
            // make sure we don't try to shutdown our self...
            //
            if( pid != common::process::id())
            {
               servers.push_back( getInstance( pid).process);
            }
         }

         auto result = common::server::lifetime::soft::shutdown( servers, std::chrono::milliseconds( 500));

         for( auto& pid : result)
         {
            removeProcess( pid);
         }

         return result;
      }


   } // broker

} // casual
