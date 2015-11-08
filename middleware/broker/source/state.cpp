//!
//! state.cpp
//!
//! Created on: Sep 13, 2014
//!     Author: Lazan
//!

#include "broker/state.h"
#include "broker/filter.h"
#include "broker/transform.h"

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

         namespace local
         {
            namespace
            {
               std::ostream& base_print( std::ostream& out, const Executable& value)
               {
                  return out << "alias: " << value.alias
                        << ", id: " << value.id
                        << ", configured_instances: " << value.configured_instances
                        << ", instances: " << range::make( value.instances)
                        << ", memberships" << range::make( value.memberships)
                        << ", arguments: " << range::make( value.arguments)
                        << ", restart: " << std::boolalpha << value.restart
                        << ", path: " << value.path;
               }
            } // <unnamed>
         } // local

         std::ostream& operator << ( std::ostream& out, const Executable& value)
         {
            out << "{ ";
            local::base_print( out, value);
            out << "}";

            return out;
         }

         std::ostream& operator << ( std::ostream& out, const Server& value)
         {
            out << "{ ";
            local::base_print( out, value);
            out << ", restrictions: " << range::make( value.restrictions) << "}";

            return out;
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
               service.information.type = s.information.type;
               service.information.transaction = s.information.transaction;
            }

            service.instances.push_back( instance);
            range::trim( service.instances, range::unique( range::sort( service.instances)));;

            instance.services.push_back( service);
            range::trim( instance.services, range::unique( range::sort( instance.services)));;
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


      void State::process( common::process::lifetime::Exit death)
      {
         //
         // We try to send a event to our self, if it's not possible (queue is full) we put it in pending
         //

         message::dead::process::Event event{ death};

         queue::non_blocking::Send send{ *this};

         if( ! send( common::ipc::broker::id(), event))
         {
            pending.replies.emplace_back( event, common::ipc::broker::id());
         }
      }

      void State::remove_process( state::Server::pid_type pid)
      {
         Trace trace{ "broker::State::remove_process", log::internal::debug};


         log::internal::debug << "remove process pid: " << pid << std::endl;

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

            server.invoked += instance.invoked;


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

         //
         // Erase from singeltons, if any...
         //
         {
            auto found = range::find_if( singeltons, [=]( const decltype( singeltons)::value_type& v){
               return v.second.pid == pid;
            });

            if( found)
            {
               singeltons.erase( found.first);
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


      void State::connect_broker( std::vector< common::message::Service> services)
      {
         try
         {
            getInstance( common::process::id());
         }
         catch( const state::exception::Missing&)
         {
            {
               state::Server server;
               server.alias = "casual-broker";
               server.configured_instances = 1;
               server.path = common::process::path();
               server.instances.push_back( common::process::id());

               {
                  state::Server::Instance instance;
                  instance.process = common::process::handle();
                  instance.server = server.id;
                  instance.alterState( state::Server::Instance::State::idle);

                  add( std::move( instance));
               }

               add( std::move( server));
            }

            //
            // Add services
            //
            {
               std::vector< state::Service> brokerServices;

               common::range::transform( services, brokerServices, transform::Service{});

               addServices( common::process::id(), std::move( brokerServices));
            }
         }
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

         //
         // We need to go through the groups in reverse order so we get a match for an executable
         // for the group that is booted last (if the executable has several memberships).
         //
         // We just reverse groups, and then we reverse the result.
         //
         {
            for( auto&& group : range::make_reverse( groups))
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

            std::reverse( std::begin( result), std::end( result));
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
   } // broker

} // casual
