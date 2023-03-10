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
                  if( communication::ipc::exists( ipc))
                     if( ! communication::ipc::remove( ipc))
                        log::line( log::category::error, "failed to remove ipc: ", communication::ipc::Address{ ipc});
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
                  case State::running: return "running";
                  case State::spawned : return "spawned";
                  case State::scale_out: return "scale-out";
                  case State::scale_in: return "scale-in";
                  case State::exit: return "exit";
                  case State::error: return "error";
               }
               return "unknown";
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
                  template< typename I>
                  auto spawnable( I& instances)
                  {
                     using state_type = typename I::value_type::state_type;

                     return algorithm::sorted::subrange( instances, []( auto& i){
                        return i.state == state_type::scale_out && ! common::process::id( i.handle);
                     });
                  }

                  template< typename I>
                  auto shutdownable( I& instances)
                  {
                     using state_type = typename I::value_type::state_type;

                     return algorithm::sorted::subrange( instances, []( auto& i){
                        return i.state == state_type::scale_in && common::process::id( i.handle);
                     });
                  }

                  template< typename I>
                  void scale( I& instances, platform::size::type count)
                  {
                     Trace trace{ "domain::manager::state::local::scale"};
                     log::line( verbose::log, "instances: ", instances);

                     using state_type = decltype( std::begin( instances)->state);

                     auto [ running, rest] = algorithm::stable::partition( instances, []( auto& instance)
                     {
                        return algorithm::compare::any( instance.state, state_type::running, state_type::scale_out);
                     });

                     // Do we scale in, or scale out?
                     if( running.size() < count)
                     {
                        count -= running.size();

                        // scale out
                        // check if we got any 'exit' to reuse
                        {
                           auto exit = algorithm::filter( rest, []( auto& instance)
                           {
                              return algorithm::compare::any( instance.state, state_type::exit, state_type::error);
                           });

                           count -= algorithm::for_each_n( exit, count, []( auto& instance)
                           {
                              instance.state = state_type::scale_out;
                           }).size();

                           algorithm::container::trim( instances, algorithm::remove_if( instances, []( auto& instance)
                           {
                              return algorithm::compare::any( instance.state, state_type::exit, state_type::error);
                           }));
                        }

                        // resize the source to get the rest, and rely on default ctor for instance_type.
                        instances.resize( instances.size() + count);
                     }
                     else
                     {
                        // scale in.
                        // We just advance the range, and scale_in the reminders.
                        algorithm::for_each( running.advance( count), []( auto& instance)
                        {
                           instance.state = state_type::scale_in;
                        });
                        
                        auto is_removable = []( auto& i)
                        {
                           return i.state == state_type::scale_in && ! process::id( i.handle);
                        };

                        // we can remove the backmarkers with invalid 'pids'
                        algorithm::container::trim( instances, algorithm::remove_if( instances, is_removable));
                     }

                     log::line( verbose::log, "instances: ", instances);
                  }

               } // instance
            } // <unnamed>
         } // local

         Executable::instances_range Executable::spawnable()
         {
            return local::instance::spawnable( instances);
         }

         Executable::const_instances_range Executable::spawnable() const
         {
            return local::instance::spawnable( instances);
         }

         Executable::const_instances_range Executable::shutdownable() const
         {
            return local::instance::shutdownable( instances);
         }

         void Executable::scale( platform::size::type count)
         {
            local::instance::scale( instances, count);
         }

         void Executable::remove( strong::process::id pid)
         {
            Trace trace{ "domain::manager::state::Executable::remove"};

            if( auto found = algorithm::find( instances, pid))
            {
               log::line( verbose::log, "found: ", *found);
               auto const state = found->state;

               if( state == state_type::scale_in)
               {
                  log::line( verbose::log, "remove: ", *found);
                  instances.erase( std::begin( found));
               }
               else
               {
                  found->handle = common::strong::process::id{};
                  found->state =  state == state_type::running && restart ? state_type::scale_out : state_type::exit;
                  log::line( verbose::log, "reset: ", *found);
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

         common::process::Handle Server::remove( common::strong::process::id pid)
         {
            Trace trace{ "domain::manager::state::Server::remove"};

            if( auto found = algorithm::find( instances, pid))
            {
               log::line( verbose::log, "found: ", *found);
               log::line( verbose::log, "instances: ", instances);

               if( found->state == state_type::scale_in)
                  return algorithm::container::extract( instances, std::begin( found)).handle;

               found->state = found->state == state_type::running && restart ? state_type::scale_out : state_type::exit;
               return std::exchange( found->handle, {});
            }

            return {};
         }

         bool Server::connect( const common::process::Handle& process)
         {
            if( auto found = algorithm::find( instances, process.pid))
            {
               found->handle = process;
               found->state = instance::State::running;
               return true;
            }
            return false;
         }


         Server::instances_range Server::spawnable()
         {
            return local::instance::spawnable( instances);
         }

         Server::const_instances_range Server::spawnable() const
         {
            return local::instance::spawnable( instances);
         }

         Server::const_instances_range Server::shutdownable() const
         {
            return local::instance::shutdownable( instances);
         }

         void Server::scale( platform::size::type count)
         {
            local::instance::scale( instances, count);
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

      state::Group& State::group( state::Group::id_type id)
      {
         return range::front( algorithm::find_if( groups, [=]( const auto& g){
            return g.id == id;
         }));
      }

      const state::Group& State::group( state::Group::id_type id) const
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
            decltype( auto) executable( I& instances, ID id)
            {
               return range::front( algorithm::find_if( instances, [id]( auto& i){
                  return i.id == id;
               }));
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
         return local::executable( servers, id);
      }
      const state::Server& State::entity( state::Server::id_type id) const
      {
         return local::executable( servers, id);
      }

      state::Executable& State::entity( state::Executable::id_type id)
      {
         return local::executable( executables, id);
      }
      const state::Executable& State::entity( state::Executable::id_type id) const
      {
         return local::executable( executables, id);
      }


      State::Runnables State::runnables( std::vector< std::string> aliases)
      {
         auto range = algorithm::unique( algorithm::sort( aliases));

         auto wanted_alias = [ &range]( auto& entity)
         {
            // partition all (0..1) instances to the end
            auto split = algorithm::partition( range, [&entity]( auto& alias)
            {
               return alias == entity.alias;
            });

            // we keep the non-matched.
            range = std::get< 1>( split);
            // if the 'removed' is not empty, we have a match.
            return ! std::get< 0>( split).empty();
         };

         Runnables runnables;

         // convert to reference wrappers
         algorithm::copy_if( servers, std::back_inserter( runnables.servers), wanted_alias);
         algorithm::copy_if( executables, std::back_inserter( runnables.executables), wanted_alias);
         
         return runnables;
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
         algorithm::append( variables, result);
         return result;
      }


   } // domain::manager
} // casual
