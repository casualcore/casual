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
         namespace instance
         {
            std::string_view description( State value) noexcept
            {
               switch( value)
               {
                  case State::disabled: return "disabled";
                  case State::running: return "running";
                  case State::spawned : return "spawned";
                  case State::scale_out: return "scale_out";
                  case State::scale_in: return "scale_in";
                  case State::exit: return "exit";
                  case State::error: return "error";
               }
               return "<unknown>";
            }

            std::string_view description( Phase value) noexcept
            {
               switch( value)
               {
                  case Phase::disabled: return "disabled";
                  case Phase::running: return "running";
                  case Phase::exit: return "exit";
                  case Phase::error: return "error";
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
                     log::line( verbose::log, "instances: ", entity.instances);

                     auto set_scale_keep = [ enabled = entity.enabled]( auto& instance)
                     {
                        if( enabled) 
                           instance.wanted = state::instance::Phase::running;
                        else
                           instance.wanted = state::instance::Phase::disabled;
                     };

                     auto set_scale_exit = []( auto& instance)
                     {
                        instance.wanted = state::instance::Phase::exit;
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
                     algorithm::for_each( exit, set_scale_exit);

                     // remove already exit (wasn't spawned to begin with)
                     {
                        algorithm::container::erase( entity.instances, algorithm::filter( exit, []( auto& instance)
                        { 
                           return instance.state() == state::instance::State::exit;
                        }));
                     }

                     log::line( verbose::log, "instances: ", entity.instances);
                  }

               } // instance

            } // <unnamed>
         } // local

         instance::State Executable::instance_policy::state( common::strong::process::id pid, instance::Phase wanted)
         {
            switch( wanted)
            {
               case instance::Phase::running:
                  return pid ? instance::State::running : instance::State::scale_out;
               case instance::Phase::exit:
                  return pid ? instance::State::scale_in : instance::State::exit;
               case instance::Phase::disabled:
                  return pid ? instance::State::scale_in : instance::State::disabled;
               case instance::Phase::error:
                  return instance::State::error;
            }
            common::code::raise::error( common::code::casual::internal_unexpected_value, "wanted phase: ", wanted);
         }


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

         void Executable::remove( strong::process::id pid)
         {
            Trace trace{ "domain::manager::state::Executable::remove"};

            if( auto found = algorithm::find( instances, pid))
            {
               log::line( verbose::log, "found: ", *found);
               log::line( verbose::log, "instances: ", instances);

               found->handle = {};

               switch( found->wanted)
               {
                  case instance::Phase::disabled:
                  case instance::Phase::error:
                     break;
                  case instance::Phase::exit:
                  {
                     algorithm::container::erase( instances, std::begin( found));
                     break;
                  }
                  case instance::Phase::running:
                  {
                     if( restart)
                        initiated_restarts++;
                     else
                        found->wanted = instance::Phase::exit;

                     break;
                  }
               }
            }
         }

         bool operator == ( const Executable& lhs, common::strong::process::id rhs)
         {
            return predicate::boolean( algorithm::find( lhs.instances, rhs));
         }

         instance::State Server::instance_policy::state( const common::process::Handle& handle, instance::Phase wanted)
         {
            switch( wanted)
            {
               case instance::Phase::running:
                  return handle ? instance::State::running : handle.pid ? instance::State::spawned : instance::State::scale_out;
               case instance::Phase::exit:
                  return handle ? instance::State::scale_in : instance::State::exit;
               case instance::Phase::disabled:
                  return handle ? instance::State::scale_in : instance::State::disabled;
               case instance::Phase::error:
                  return instance::State::error;
            }
            common::code::raise::error( common::code::casual::internal_unexpected_value, "wanted phase: ", wanted);
         }

         const Server::instance_type* Server::instance( common::strong::process::id pid) const
         {
            return algorithm::find_if( instances, [pid]( auto& p){ return p.handle.pid == pid;}).data();
         }

         common::process::Handle Server::remove( common::strong::process::id pid)
         {
            Trace trace{ "domain::manager::state::Server::remove"};

            if( auto found = algorithm::find( instances, pid))
            {
               log::line( verbose::log, "found: ", *found);
               log::line( verbose::log, "instances: ", instances);

               switch( found->wanted)
               {
                  case instance::Phase::disabled:
                  case instance::Phase::error:
                     break;
                  case instance::Phase::exit:
                     return algorithm::container::extract( instances, std::begin( found)).handle;
                  case instance::Phase::running:
                  {
                     if( restart)
                        initiated_restarts++;
                     else
                        found->wanted = instance::Phase::exit;

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

      } // state


      std::tuple< state::Server*, state::Executable*> State::remove( common::strong::process::id pid)
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
            log::line( log, "remove singleton: ", found->second);

            // Try to remove ipc-queue (no-op if it's removed already)
            local::ipc::remove( found->second.ipc);

            singletons.erase( std::begin( found));
         }

         algorithm::container::erase( configuration.stakeholders, pid);

         using result_type = std::tuple< state::Server*, state::Executable*>;

         // Check if it's a server
         if( auto found = server( pid))
         {
            // we know the instance exists...
            auto process = found->remove( pid);
            
            log::line( log, "remove server instance: ", process);

            // Try to remove ipc-queue (no-op if it's removed already)
            local::ipc::remove( process.ipc);

            if( found->restart && runlevel == decltype( runlevel())::running)
               return result_type{ found, nullptr};
         }

         // Find and remove from executable
         if( auto found = executable( pid))
         {
            found->remove( pid);
            log::line( log, "remove executable instance: ", pid);

            if( found->restart && runlevel == decltype( runlevel())::running)
               return result_type{ nullptr, found};
         }

         // check if it's a grandchild
         if( auto found = algorithm::find( grandchildren, pid))
         {
            log::line( log, "remove grandchild: ", *found);
            
            // Try to remove ipc-queue (no-op if it's removed already)
            local::ipc::remove( found->handle.ipc);

            grandchildren.erase( std::begin( found));
         }

         log::line( log, "runlevel: ", runlevel);
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
               result.servers.push_back( *found);
            else if( auto found = State::executable( alias); found && ! State::untouchable( found->id))
               result.executables.push_back( *found);
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


   } // domain::manager
} // casual
