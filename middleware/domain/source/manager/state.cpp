//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include <utility>

#include "domain/manager/state.h"
#include "domain/manager/state/create.h"
#include "domain/common.h"


#include "common/message/domain.h"

namespace casual
{
   using namespace common;
   namespace domain
   {
      namespace manager
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
                     {
                        communication::ipc::remove( ipc);
                     }
                  }
               } // ipc

            } // <unnamed>
         } // local

         namespace state
         {
            namespace instance
            {
               std::ostream& operator << ( std::ostream& out, State value)
               {
                  switch( value)
                  {
                     case State::scale_out: return out << "scale-out";
                     case State::running: return out << "running";
                     case State::scale_in: return out << "scale-in";
                     case State::exit: return out << "exit";
                     case State::spawn_error: return out << "spawn-error";
                  }
                  return out << "unknown";
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
                     void scale( I& instances, size_type count)
                     {
                        Trace trace{ "domain::manager::state::local::scale"};

                        log::line( verbose::log, "instances: ", instances);

                        using state_type = typename I::value_type::state_type;

                        auto split = algorithm::stable_partition( instances, []( auto& i){
                           return algorithm::compare::any( i.state, state_type::running, state_type::scale_out);
                        });

                        auto running = std::get< 0>( split);

                        // Do we scale in, or scale out?
                        if( running.size() < count)
                        {
                           count -= running.size();

                           // scale out
                           // check if we got any 'exit' to reuse
                           {
                              auto exit = std::get< 0>( algorithm::partition( std::get< 1>( split), []( auto& i){
                                 return i.state == state_type::exit;
                              }));

                              count -= algorithm::for_each_n( exit, count, []( auto& i){
                                 i.state = state_type::scale_out;
                              }).size();
                           }

                           // resize to get the rest, and rely on default ctor for instance_type.
                           instances.resize( instances.size() + count);

                        }
                        else
                        {
                           // scale in.
                           // We just advance the range, and scale_in the reminders.
                           algorithm::for_each( running.advance( count), []( auto& i){
                              i.state = state_type::scale_in;
                           });
                        }

                        algorithm::stable_sort( instances);

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

            void Executable::scale( size_type count)
            {
               local::instance::scale( instances, count);
            }

            void Executable::remove( pid_type pid)
            {
               Trace trace{ "domain::manager::state::Executable::remove"};

               auto found = algorithm::find( instances, pid);

               if( found)
               {
                  auto const state = found->state;

                  if( state == state_type::scale_in)
                  {
                     log::line( verbose::log, "remove: ", *found);
                     instances.erase( std::begin( found));
                  }
                  else
                  {
                     log::line( verbose::log, "reset: ", *found);
                     found->handle = common::strong::process::id{};
                     found->state =  state == state_type::running && restart ? state_type::scale_out : state_type::exit;
                  }
               }
            }


            Server::instance_type Server::instance( common::strong::process::id pid) const
            {
               auto found = algorithm::find_if( instances, [pid]( auto& p){
                  return p.handle.pid == pid;
               });

               if( found)
               {
                  return *found;
               }
               return {};
            }

            Server::instance_type Server::remove( common::strong::process::id pid)
            {
               Trace trace{ "domain::manager::state::Server::remove"};

               auto found = algorithm::find( instances, pid);

               if( ! found)
               {
                  log::line( log, "failed to find server - pid: ", pid);
                  log::line( verbose::log, "instances: ", instances);
                  return {};
               }
 
               auto result = *found;

               auto const state = found->state;

               if( state == state_type::scale_in)
               {
                  log::line( verbose::log, "remove: ", *found);
                  instances.erase( std::begin( found));
               }
               else
               {
                  log::line( verbose::log, "reset: ", *found);
                  found->handle = common::process::Handle{};
                  found->state =  state == state_type::running && restart ? state_type::scale_out : state_type::exit;
               }
               
               return result;
            }

            bool Server::connect( common::process::Handle process)
            {
               auto found = algorithm::find_if( instances, [=]( auto& i){
                  return i.handle.pid == process.pid;
               });

               if( found)
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

            void Server::scale( size_type count)
            {
               local::instance::scale( instances, count);
            }



            bool operator == ( const Server& lhs, common::strong::process::id rhs)
            {
               return lhs.instance( rhs).handle.pid == rhs;
            }


            Batch::Batch( Group::id_type group) : group(std::move( group)) {}

            void Batch::log( std::ostream& out, const State& state) const
            {
               stream::write( out, "{ group: ", group);

               auto delimiter = [&out](){ out << ", ";};

               out << ", servers: [";
               algorithm::for_each_interleave( servers, [&out,&state]( auto id){
                  stream::write( out,  state.server( id));
               }, delimiter);

               out << "], executables: [";
               algorithm::for_each_interleave( executables, [&out,&state]( auto id){
                  stream::write( out, state.executable( id));
               }, delimiter);

               out << "]}";
            }



         } // state


         std::vector< state::Batch> State::bootorder()
         {
            Trace trace{ "domain::manager::State::bootorder"};

            return state::create::boot::order( servers, executables, groups);
         }

         std::vector< state::Batch> State::shutdownorder()
         {
            Trace trace{ "domain::manager::State::shutdownorder"};

            return algorithm::reverse( bootorder());
         }


         std::tuple< state::Server*, state::Executable*> State::remove( common::strong::process::id pid)
         {
            Trace trace{ "domain::manager::State::remove pid"};

            // We remove from event listeners if one of them has died
            event.remove( pid);

            // We remove from pending 
            {
               algorithm::trim( pending.lookup, algorithm::remove_if( pending.lookup, [pid]( auto& m)
               {
                  return m.process == pid;
               }));
            }

            // Remove from singletons
            {
               auto found = algorithm::find_if( singletons, [pid]( auto& v){
                  return v.second == pid;
               });

               if( found)
               {
                  log::line( log, "remove singleton: ", found->second);
                  singletons.erase( std::begin( found));
               }
            }

            using result_type = std::tuple< state::Server*, state::Executable*>;

            // Check if it's a server
            {
               auto found = server( pid);

               if( found)
               {
                  auto instance = found->remove( pid);
                  log::line( log, "remove server instance: ", instance);

                  // Try to remove ipc-queue (no-op if it's removed already)
                  local::ipc::remove( instance.handle.ipc);

                  if( found->restart && runlevel() == Runlevel::running)
                  {
                     return result_type{ found, nullptr};
                  }
               }
            }

            // Find and remove from executable
            {
               auto found = executable( pid);

               if( found)
               {
                  found->remove( pid);
                  log::line( log, "remove executable instance: ", pid);

                  if( found->restart && runlevel() == Runlevel::running)
                  {
                     return result_type{ nullptr, found};
                  }
               }
            }

            // check if it's a grandchild
            {
               auto found = algorithm::find_if( grandchildren, [=]( const auto& v){
                  return v.pid == pid;
               });

               if( found)
               {
                  log::line( log, "remove grandchild: ", *found);
                  grandchildren.erase( std::begin( found));
               }
            }

            return result_type{};
         }


         std::vector< common::environment::Variable> State::variables( const state::Process& process)
         {
            auto result = casual::configuration::environment::transform( casual::configuration::environment::fetch( environment));

            algorithm::append( process.environment.variables, result);

            return result;
         }

         namespace local
         {
            namespace
            {
               template< typename S>
               auto server( S& servers, common::strong::process::id pid) noexcept
               {
                  return algorithm::find_if( servers, [pid]( const auto& s){
                     return s.instance( pid).handle.pid == pid;
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
            return algorithm::find_if( executables, [=]( const auto& e){
               return ! algorithm::find( e.instances, pid).empty();
            }).data();
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
            } // <unnamed>
         } // local

         state::Server& State::server( state::Server::id_type id)
         {
            return local::executable( servers, id);
         }

         const state::Server& State::server( state::Server::id_type id) const
         {
            return local::executable( servers, id);
         }

         state::Executable& State::executable( state::Executable::id_type id)
         {
            return local::executable( executables, id);
         }
         const state::Executable& State::executable( state::Executable::id_type id) const
         {
            return local::executable( executables, id);
         }

         common::process::Handle State::grandchild( common::strong::process::id pid) const noexcept
         {
            auto found = algorithm::find_if( grandchildren, [=]( auto& v){
               return v.pid == pid;
            });

            if( found) 
               return *found;

            return {};
         }

         common::process::Handle State::singleton( const common::Uuid& id) const noexcept
         {
            auto found = algorithm::find( singletons, id);
            if( found)
            {
               return found->second;
            }
            return {};
         }

         bool State::execute()
         {
            return ! ( runlevel() >= Runlevel::shutdown && tasks.empty());
         }


         void State::runlevel( Runlevel runlevel) noexcept
         {
            if( runlevel > m_runlevel)
            {
               m_runlevel = runlevel;
            }
         }


         std::vector< std::string> State::resources( common::strong::process::id pid)
         {
            auto process = server( pid);

            if( ! process)
            {
               return {};
            }

            auto resources = process->resources;

            for( auto& id : process->memberships)
            {
               auto& group = State::group( id);

               common::algorithm::append( group.resources, resources);
            }

            return range::to_vector( algorithm::unique( algorithm::sort( resources)));
         }

      } // manager
   } // domain
} // casual
