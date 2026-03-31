//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "domain/manager/state.h"
#include "domain/common.h"


#include "common/message/domain.h"
#include "common/algorithm/compare.h"
#include "common/algorithm/sorted.h"
#include "common/communication/instance.h"


#include <utility>

namespace casual
{
      
   using namespace std::string_view_literals;      

   using namespace common;
   namespace domain::manager
   {
      namespace local
      {
         namespace
         {
            namespace ipc
            {
               void remove( strong::ipc::id ipc)
               {
                  communication::ipc::remove( ipc);
               }
            } // ipc

         } // <unnamed>
      } // local

      namespace state
      {
         std::string_view description( Runlevel value)
         {
            switch( value)
            {
               case Runlevel::error: return "error";
               case Runlevel::running: return "running";
               case Runlevel::shutdown: return "shutdown";
               case Runlevel::startup: return "startup";
            }
            return "<unknown>";
         }

         namespace instance
         {
            std::string_view description( State value) noexcept
            {
               switch( value)
               {
                  case State::disabled: return "disabled";
                  case State::running: return "running";
                  case State::scale_out: return "scale_out";
                  case State::scale_in: return "scale_in";
                  case State::exit: return "exit";
                  case State::error: return "error";
               }
               return "<unknown>";
            }

            std::string_view description( Wanted value) noexcept
            {
               switch( value)
               {
                  case Wanted::disabled: return "disabled";
                  case Wanted::running: return "running";
                  case Wanted::lingered: return "lingered";
                  case Wanted::removed: return "removed";
               }
               return "<unknown>";
            }

            std::string_view description( Exit value) noexcept
            {
               switch( value)
               {
                  case Exit::none: return "none";
                  case Exit::exit: return "exit";
                  case Exit::error: return "error";
               }
               return "<unknown>";
            }

         } // instance


         bool Group::boot::Order::operator () ( const Group& lhs, const Group& rhs)
         {
            auto rhs_depend = algorithm::find( rhs.dependencies, lhs.id);
            auto lhs_depend = algorithm::find( lhs.dependencies, rhs.id);

            return rhs_depend && ! lhs_depend;
         }

         namespace local
         {
            namespace
            {
               namespace instance
               {
                  auto spawnable( auto& entity)
                  {
                     return algorithm::sorted::subrange( entity.instances, []( auto& instance)
                     {
                        return instance.state() == state::instance::State::scale_out;
                     });
                  }

                  auto shutdownable( auto& entity)
                  {
                     return algorithm::sorted::subrange( entity.instances, []( auto& instance)
                     {
                        return instance.state() == state::instance::State::scale_in;
                     });
                  }

                  void scale( auto& entity, platform::size::type count)
                  {
                     Trace trace{ "domain::manager::state::local::scale"};
                     log::debug( "instances: ", entity.instances);

                     auto set_scale_keep = [ enabled = entity.enabled]( auto& instance)
                     {
                        if( enabled) 
                           instance.wanted = state::instance::Wanted::running;
                        else
                           instance.wanted = state::instance::Wanted::disabled;
                     };

                     auto set_scale_removed = []( auto& instance)
                     {
                        instance.wanted = state::instance::Wanted::removed;
                     };

                     if( std::ssize( entity.instances) < count)
                     {
                        entity.instances.resize( count);
                        algorithm::for_each( entity.instances, set_scale_keep);
                        return;
                     }

                     // move all running to the front, if any                     
                     algorithm::stable::partition( entity.instances, []( auto& instance)
                     {
                        return instance.state() == state::instance::State::running;
                     });

                     auto keep = range::make( std::begin( entity.instances), count);
                     auto exit = range::make( std::end( keep), std::end( entity.instances));

                     algorithm::for_each( keep, set_scale_keep);
                     algorithm::for_each( exit, set_scale_removed);

                     // remove already removed (wasn't spawned to begin with)
                     {
                        algorithm::container::erase( entity.instances, algorithm::filter( exit, []( auto& instance)
                        { 
                           return algorithm::compare::any( instance.state(), state::instance::State::exit, state::instance::State::error);
                        }));
                     }

                     log::debug( "instances: ", entity.instances);
                  }

               } // instance

            } // <unnamed>
         } // local


         Executable::instances_range Executable::spawnable()
         {
            return local::instance::spawnable( *this);
         }

         Executable::const_instances_range Executable::spawnable() const
         {
            if( ! enabled)
               return {};

            return local::instance::spawnable( *this);
         }

         Executable::const_instances_range Executable::shutdownable() const
         {
            return local::instance::shutdownable( *this);
         }

         void Executable::scale( platform::size::type count)
         {
            local::instance::scale( *this, count);
         }

         void Executable::remove( strong::process::id pid, common::process::lifetime::exit::Reason reason, Runlevel runlevel)
         {
            Trace trace{ "domain::manager::state::Executable::remove"};

            if( auto found = algorithm::find( instances, pid))
            {
               log::debug( "found: ", *found);
               log::debug( "instances: ", instances);

               found->handle = {};

               found->exit = reason == common::process::lifetime::exit::Reason::exited ? instance::Exit::exit : instance::Exit::error;

               switch( found->wanted)
               {
                  case instance::Wanted::disabled:
                  case instance::Wanted::lingered:
                     break;
                  case instance::Wanted::removed:
                  {
                     algorithm::container::erase( instances, std::begin( found));
                     break;
                  }
                  case instance::Wanted::running:
                  {
                     // we only restart if we are in running runlevel
                     if( restart && runlevel == Runlevel::running)
                        initiated_restarts++;
                     else
                        found->wanted = instance::Wanted::lingered;

                     break;
                  }
               }
            }
         }

         bool operator == ( const Executable& lhs, common::strong::process::id rhs)
         {
            return predicate::boolean( algorithm::find( lhs.instances, rhs));
         }


         const Server::instance_type* Server::instance( common::strong::process::id pid) const
         {
            return algorithm::find_if( instances, [pid]( auto& p){ return p.handle.pid == pid;}).data();
         }

         common::process::Handle Server::remove( common::strong::process::id pid, common::process::lifetime::exit::Reason reason, Runlevel runlevel)
         {
            Trace trace{ "domain::manager::state::Server::remove"};

            if( auto found = algorithm::find( instances, pid))
            {
               log::debug( "found: ", *found);
               log::debug( "instances: ", instances);

               found->exit = reason == common::process::lifetime::exit::Reason::exited ? instance::Exit::exit : instance::Exit::error;

               switch( found->wanted)
               {
                  case instance::Wanted::disabled:
                  case instance::Wanted::lingered:
                     break;
                  case instance::Wanted::removed:
                     return algorithm::container::extract( instances, std::begin( found)).handle;
                  case instance::Wanted::running:
                  {
                     // we only restart if we are in running runlevel
                     if( restart && runlevel == Runlevel::running)
                        initiated_restarts++;
                     else
                        found->wanted = instance::Wanted::lingered;

                     break;
                  }
               }

               return std::exchange( found->handle, {});
            }

            return {};
         }

         bool Server::connect( const common::process::Handle& process)
         {
            if( auto found = algorithm::find( instances, process.pid))
            {
               found->handle = process;
               return true;
            }
            return false;
         }

         Server::instances_range Server::spawnable()
         {
            if( ! enabled)
               return {};

            return local::instance::spawnable( *this);
         }

         Server::const_instances_range Server::spawnable() const
         {
            return local::instance::spawnable( *this);
         }

         Server::const_instances_range Server::shutdownable() const
         {
            return local::instance::shutdownable( *this);
         }

         void Server::scale( platform::size::type count)
         {
            local::instance::scale( *this, count);
         }

         bool operator == ( const Server& lhs, common::strong::process::id rhs)
         {
            return lhs.instance( rhs) != nullptr;
         }

      } // state


      std::tuple< state::Server*, state::Executable*, std::optional< common::message::event::Error>> State::remove( common::strong::process::id pid, common::process::lifetime::exit::Reason reason)
      {
         Trace trace{ "domain::manager::State::remove pid"};

         // We remove from event listeners if one of them has died
         event.remove( pid);

         algorithm::container::erase( whitelisted, pid);

         // We remove from pending 
         algorithm::container::erase( pending.lookup, pid);

         // Remove from singletons
         auto is_singleton = [ pid]( auto& pair){ return pair.second == pid;};

         if( auto found = algorithm::find_if( singletons, is_singleton))
         {
            log::debug( "remove singleton: ", found->second);

            // Try to remove ipc-queue (no-op if it's removed already)
            local::ipc::remove( found->second.ipc);

            singletons.erase( std::begin( found));
         }

         algorithm::container::erase( configuration.stakeholders, pid);

         using result_type = std::tuple< state::Server*, state::Executable*, std::optional< common::message::event::Error>>;

         // Check if it's a server
         if( auto found = server( pid))
         {
            log::debug( "found: ", *found);

            // we know the instance exists...
            auto process = found->remove( pid, reason, runlevel());

            log::debug( "remove server instance: ", process);

            // Try to remove ipc-queue (no-op if it's removed already)
            local::ipc::remove( process.ipc);

            if( runlevel == state::Runlevel::running)
            {
               // if discovery dies, we want to return an error, so we can do a hard shutdown.
               auto vital_servers = std::array{ "casual-domain-discovery"sv};
               
               if( std::ranges::contains( vital_servers, found->alias))
               {
                  common::message::event::Error error{ common::process::handle()};
                  error.severity = decltype( error.severity)::fatal;
                  error.code = code::casual::fatal_terminate;
                  error.message = string::compose( "vital '", found->alias, "' has died, we can't continue - action: shutting down");

                  return result_type{ nullptr, nullptr, error};
               }

               if( found->restart)
                  return result_type{ found, nullptr, std::nullopt};
            }

            if( found->restart && runlevel == state::Runlevel::running)
               return result_type{ found, nullptr, std::nullopt};
         }

         // Find and remove from executable
         if( auto found = executable( pid))
         {
            log::debug( "found: ", *found);

            found->remove( pid, reason, runlevel());
            log::debug( "remove executable instance: ", pid);

            if( found->restart && runlevel == state::Runlevel::running)
               return result_type{ nullptr, found, std::nullopt};
         }

         // check if it's a grandchild
         if( auto found = algorithm::find( grandchildren, pid))
         {
            log::debug( "remove grandchild: ", *found);
            
            // Try to remove ipc-queue (no-op if it's removed already)
            local::ipc::remove( found->handle.ipc);

            grandchildren.erase( std::begin( found));
         }

         log::debug( "runlevel: ", runlevel);
         return result_type{};
      }


      namespace local
      {
         namespace
         {
            template< typename S>
            auto server( S& servers, common::strong::process::id pid) noexcept
            {
               return algorithm::find_if( servers, [pid]( const auto& s){
                  return s.instance( pid) != nullptr;
               }).data();
            }

            template< typename E>
            auto executable( E& executables, common::strong::process::id pid) noexcept
            {
               return algorithm::find_if( executables, [=]( const auto& e){
                  return ! algorithm::find( e.instances, pid).empty();
               }).data();
            }
            
         } // <unnamed>
      } // local

      state::Server* State::server( common::strong::process::id pid) noexcept
      {
         return local::server( servers, pid);
      }

      const state::Server* State::server( common::strong::process::id pid) const noexcept
      {
         return local::server( servers, pid);
      }

      state::Executable* State::executable( common::strong::process::id pid) noexcept
      {
         return local::executable( executables, pid);
      }

      const state::Executable* State::executable( common::strong::process::id pid) const noexcept
      {
         return local::executable( executables, pid);
      }

      state::Server* State::server( const std::string& alias) noexcept
      {
         if( auto found = algorithm::find( servers, alias))
            return found.data();
         return nullptr;
      }

      state::Executable* State::executable( const std::string& alias) noexcept
      {
         if( auto found = algorithm::find( executables, alias))
            return found.data();
         return nullptr;
      }

      state::Group& State::group( strong::group::id id)
      {
         return range::front( algorithm::find_if( groups, [=]( const auto& g){
            return g.id == id;
         }));
      }

      const state::Group& State::group( strong::group::id id) const
      {
         return range::front( algorithm::find_if( groups, [=]( const auto& g){
            return g.id == id;
         }));
      }

      namespace local
      {
         namespace
         {
            template< typename I, typename ID>
            auto entities( I& instances, ID id)
            {
               return algorithm::find_if( instances, [id]( auto& i)
               {
                  return i.id == id;
               });
            }

            template< typename I, typename ID>
            decltype( auto) assert_entities( I& instances, ID id)
            {
               auto found = local::entities( instances, id);
               assert( found);
               return range::front( found);
            }

            template< typename G>
            auto grandchild( G& grandchildren, common::strong::process::id pid) noexcept
            {
               return algorithm::find( grandchildren, pid).data();
            }

         } // <unnamed>
      } // local

      state::Server& State::entity( state::Server::id_type id)
      {

         return local::assert_entities( servers, id);
      }
      const state::Server& State::entity( state::Server::id_type id) const
      {
         return local::assert_entities( servers, id);
      }

      state::Executable& State::entity( state::Executable::id_type id)
      {
         return local::assert_entities( executables, id);
      }
      const state::Executable& State::entity( state::Executable::id_type id) const
      {
         return local::assert_entities( executables, id);
      }

      state::Server* State::find_entity( strong::server::id id)
      {
         if( auto found = local::entities( servers, id))
            return found.data();
         return nullptr;
      }

      state::Executable* State::find_entity( strong::executable::id id)
      {
         if( auto found = local::entities( executables, id))
            return found.data();
         return nullptr;         
      }

      state::Scalables State::scalables( std::vector< std::string> aliases)
      {
         state::Scalables result;

         for( auto& alias : algorithm::unique( algorithm::sort( aliases)))
         {
            if( auto found = State::server( alias); found && ! State::untouchable( found->id))
               result.servers.push_back( found->id);
            else if( auto found = State::executable( alias); found && ! State::untouchable( found->id))
               result.executables.push_back( found->id);
         }

         return result;
      }

      state::Grandchild* State::grandchild( common::strong::process::id pid) noexcept
      {
         return local::grandchild( grandchildren, pid);
      }

      const state::Grandchild* State::grandchild( common::strong::process::id pid) const noexcept
      {
         return local::grandchild( grandchildren, pid);
      }

      common::process::Handle State::singleton( const common::Uuid& id) const noexcept
      {
         if( auto found = algorithm::find( singletons, id))
            return found->second;
         return {};
      }
      common::process::Handle State::singleton( common::strong::process::id pid) const noexcept
      {
         auto is_singleton = [ pid]( auto& pair){ return pair.second == pid;};
         if( auto found = algorithm::find_if( singletons, is_singleton))
            return found->second;
         return {};
      }

      bool State::untouchable( common::strong::process::id id) const noexcept
      {
         return predicate::boolean( algorithm::find( whitelisted, id));
      }

      bool State::untouchable( strong::server::id id) const noexcept
      {
         return predicate::boolean( algorithm::find( whitelisted, entity( id)));
      }

      bool State::untouchable( strong::executable::id id) const noexcept
      {
         return predicate::boolean( algorithm::find( whitelisted, entity( id)));
      }



      std::tuple< std::vector< state::Server::id_type>, std::vector< state::Executable::id_type>> State::untouchables() const noexcept
      {
         auto filter_id = []( auto& entities, auto& whitelisted)
         {
            std::vector< decltype( range::front( entities).id)> result;

            for( const auto& entity : entities)
               if( algorithm::find( whitelisted, entity))
                  result.push_back( entity.id);

            return result;
         };

         return {
            filter_id( servers, whitelisted),
            filter_id( executables, whitelisted)
         };

      }

      bool State::execute()
      {
         return ! ( runlevel >= decltype( runlevel())::shutdown && tasks.empty());
      }


      std::vector< common::environment::Variable> State::variables( const std::vector< common::environment::Variable>& variables)
      {
         auto result = configuration.model.domain.environment.variables;
         algorithm::container::append( variables, result);
         return result;
      }


      void State::scale( const state::scale::Instances& instances)
      {
         Trace trace{ "domain::manager::State::scale"};
         log::debug( "instances: ", instances);

         auto scale_instances = [ this]( const auto& instance)
         {
            // can we scale it?
            if( untouchable( instance.id))
               return;

            if( auto found = find_entity( instance.id))
            {
               log::debug( "found: ", *found);
               found->scale( instance.instances);
            }
         };

         std::ranges::for_each( instances.servers, scale_instances);
         std::ranges::for_each( instances.executables, scale_instances);
      }

   } // domain::manager
} // casual
