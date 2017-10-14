//!
//! casual 
//!

#include "domain/manager/state.h"
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
               namespace remove
               {
                  void ipc( strong::ipc::id ipc)
                  {
                     if( communication::ipc::exists( ipc))
                     {
                        communication::ipc::remove( ipc);
                     }
                  }
               } // remove


               template< typename T>
               void print_executables( std::ostream& out, const T& value)
               {
                  out << "id: " << value.id
                     << ", alias: " << value.alias
                     << ", path: " << value.path
                     << ", arguments: " << range::make( value.arguments)
                     << ", restart: " << value.restart
                     << ", memberships: " << range::make( value.memberships)
                     << ", instances: " << range::make( value.instances);
               }

            } // <unnamed>
         } // local

         namespace state
         {


            std::ostream& operator << ( std::ostream& out, const Group& value)
            {
               return out << "{ id: " << value.id
                     << ", name: " << value.name
                     << ", dependencies: " << range::make( value.dependencies)
                     << ", resources: " << range::make( value.resources)
                     << '}';
            }

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
               auto rhs_depend = range::find( rhs.dependencies, lhs.id);
               auto lhs_depend = range::find( lhs.dependencies, rhs.id);

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

                        return range::sorted::subrange( instances, []( auto& i){
                           return i.state ==  state_type::scale_out && ! common::process::id( i.handle);
                        });
                     }

                     template< typename I>
                     auto shutdownable( I& instances)
                     {
                        using state_type = typename I::value_type::state_type;

                        return range::sorted::subrange( instances, []( auto& i){
                           return i.state == state_type::scale_in && common::process::id( i.handle);
                        });
                     }

                     template< typename I>
                     void scale( I& instances, size_type count)
                     {
                        Trace trace{ "domain::manager::state::local::scale"};

                        using state_type = typename I::value_type::state_type;

                        auto split = range::stable_partition( instances, []( auto& i){
                           return compare::any( i.state, { state_type::running, state_type::scale_out});
                        });

                        auto running = std::get< 0>( split);

                        //
                        // Do we scale in, or scale out?
                        //
                        if( running.size() < count)
                        {
                           count -= running.size();

                           // scale out
                           // check if we got any 'exit' to reuse
                           {
                              auto exit = std::get< 0>( range::partition( std::get< 1>( split), []( auto& i){
                                 return i.state == state_type::exit;
                              }));

                              count -= range::for_each_n( exit, count, []( auto& i){
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
                           range::for_each( running.advance( count), []( auto& i){
                              i.state = state_type::scale_in;
                           });
                        }

                        range::stable_sort( instances);

                        log::debug << "instances: " << range::make( instances) << '\n';

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

            void Executable::remove( pid_type instance)
            {
               auto found = range::find( instances, instance);

               if( found)
               {
                  auto const state = found->state;

                  if( state == state_type::scale_in)
                  {
                     instances.erase( std::begin( found));
                  }
                  else
                  {
                     found->handle = common::strong::process::id{};
                     found->state =  state == state_type::running && restart ? state_type::scale_out : state_type::exit;
                  }
               }
            }


            std::ostream& operator << ( std::ostream& out, const Executable& value)
            {
               out << "{ ";
               manager::local::print_executables( out, value);
               return out << '}';
            }

            Server::instance_type Server::instance( common::strong::process::id pid) const
            {
               auto found = range::find_if( instances, [pid]( auto& p){
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
               auto found = range::find_if( instances, [pid]( auto& p){
                  return p.handle.pid == pid;
               });

               instance_type result;

               if( found)
               {
                  result = *found;

                  auto const state = found->state;

                  if( state == state_type::scale_in)
                  {
                     instances.erase( std::begin( found));
                  }
                  else
                  {
                     found->handle = common::process::Handle{};
                     found->state =  state == state_type::running && restart ? state_type::scale_out : state_type::exit;
                  }
               }
               return result;
            }

            bool Server::connect( common::process::Handle process)
            {
               auto found = range::find_if( instances, [=]( auto& i){
                  return i.handle.pid == process.pid;
               });

               if( found)
               {
                  found->handle = process;
                  found->state = instance::State::running;
               }
               return found;
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


            std::ostream& operator << ( std::ostream& out, const Server& value)
            {
               out << "{ ";
               manager::local::print_executables( out, value);
               return out << ", resources: " << range::make( value.resources)
                  << ", restrictions: " << range::make( value.restrictions)
                  << '}';
            }

            bool operator == ( const Server& lhs, common::strong::process::id rhs)
            {
               return lhs.instance( rhs).handle.pid == rhs;
            }


            Batch::Batch( Group::id_type group) : group( group) {}


            std::ostream& operator << ( std::ostream& out, const Batch& value)
            {
               return out << "{ group: " << value.group
                     << ", servers: " << range::make( value.servers)
                     << ", executables: " << range::make( value.executables)
                     << '}';
            }

         } // state

         namespace local
         {
            namespace
            {
               namespace order
               {
                  std::vector< state::Batch> boot( State& state)
                  {
                     std::vector< state::Batch> result;

                     std::vector< std::reference_wrapper< const state::Group>> groups;
                     range::copy( state.groups, std::back_inserter( groups));
                     range::stable_sort( groups, state::Group::boot::Order{});

                     std::vector< std::reference_wrapper< state::Server>> server_wrappers;
                     std::vector< std::reference_wrapper< state::Executable>> excutable_wrappers;

                     //
                     // We make sure we don't include our self in the boot sequence.
                     //
                     range::copy_if( state.servers, std::back_inserter( server_wrappers), [&state]( const auto& e){
                        return e.id != state.manager_id;
                     });

                     range::copy( state.executables, std::back_inserter( excutable_wrappers));

                     auto executable = range::make( excutable_wrappers);
                     auto servers = range::make( server_wrappers);

                     //
                     // Reverse the order, so we 'consume' executable based on the group
                     // that is the farthest in the dependency chain
                     //
                     for( auto& group : range::reverse( groups))
                     {
                        state::Batch batch{ group.get().id};

                        auto extract = [&]( auto& entites, auto& output){

                           //
                           // Partition executables so we get the ones that has current group as a dependency
                           //
                           auto slice = range::stable_partition( entites, [&]( const auto& e){
                              return static_cast< bool>( range::find( e.get().memberships, group.get().id));
                           });

                           range::transform( std::get< 0>( slice), output, []( auto& e){
                              common::traits::concrete::type_t< decltype( range::front( output))> result;
                              result = e.get().id;
                              return result;
                           });

                           return std::get< 1>( slice);
                        };

                        servers = extract( servers, batch.servers);
                        executable = extract( executable, batch.executables);

                        result.push_back( std::move( batch));
                     }

                     //
                     // We reverse the result so the dependency order is correct
                     //
                     return range::reverse( result);
                  }
               } // order


            } // <unnamed>
         } // local

         std::vector< state::Batch> State::bootorder()
         {
            Trace trace{ "domain::manager::State::bootorder"};

            return local::order::boot( *this);
         }

         std::vector< state::Batch> State::shutdownorder()
         {
            Trace trace{ "domain::manager::State::shutdownorder"};

            return range::reverse( local::order::boot( *this));
         }



         std::tuple< state::Server*, state::Executable*> State::exited( common::strong::process::id pid)
         {
            Trace trace{ "domain::manager::State::exited"};

            //
            // Vi remove from event listeners if one of them has died
            //
            event.remove( pid);

            //
            // Remove from singletons
            //
            {
               auto found = range::find_if( singletons, [pid]( auto& v){
                  return v.second.pid == pid;
               });

               if( found)
               {
                  log << "remove singleton: " << found->second << '\n';
                  singletons.erase( std::begin( found));
               }
            }


            using result_type = std::tuple< state::Server*, state::Executable*>;

            //
            // Check if it's a server
            //
            {
               auto found = server( pid);

               if( found)
               {
                  auto instance = found->remove( pid);
                  log << "remove server instance: " << instance << '\n';
                  //
                  // Try to remove ipc-queue (no-op if it's removed already)
                  //
                  local::remove::ipc( instance.handle.queue);

                  if( found->restart && runlevel() == Runlevel::running)
                  {
                     return result_type{ found, nullptr};
                  }
               }
            }

            //
            // Find and remove from executable
            //
            {
               auto found = executable( pid);

               if( found)
               {
                  found->remove( pid);
                  log << "remove executable instance: " << pid << '\n';

                  if( found->restart && runlevel() == Runlevel::running)
                  {
                     return result_type{ nullptr, found};
                  }
               }
            }

            //
            // check if it's a grandchild
            //
            {
               auto found = range::find_if( grandchildren, [=]( const auto& v){
                  return v.pid == pid;
               });

               if( found)
               {
                  log << "remove grandchild: " << *found << '\n';
                  grandchildren.erase( std::begin( found));
               }
            }

            return result_type{};
         }

         std::vector< std::string> State::variables( const state::Process& process)
         {
            auto result = casual::configuration::environment::transform( casual::configuration::environment::fetch( environment));

            range::append( process.environment.variables, result);

            return result;
         }

         namespace local
         {
            namespace
            {
               template< typename S>
               auto server( S& servers, common::strong::process::id pid)
               {
                  return range::find_if( servers, [pid]( const auto& s){
                     return s.instance( pid).handle.pid == pid;
                  }).data();
               }
            } // <unnamed>
         } // local

         state::Server* State::server( common::strong::process::id pid)
         {
            return local::server( servers, pid);
         }

         const state::Server* State::server( common::strong::process::id pid) const
         {
            return local::server( servers, pid);
         }


         state::Executable* State::executable( common::strong::process::id pid)
         {
            return range::find_if( executables, [=]( const auto& e){
               return range::find( e.instances, pid) == true;
            }).data();
         }

         state::Group& State::group( state::Group::id_type id)
         {
            return range::front( range::find_if( groups, [=]( const auto& g){
               return g.id == id;
            }));
         }

         const state::Group& State::group( state::Group::id_type id) const
         {
            return range::front( range::find_if( groups, [=]( const auto& g){
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
                  return range::front( range::find_if( instances, [id]( auto& i){
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

         common::process::Handle State::grandchild( common::strong::process::id pid) const
         {
            auto found = range::find_if( grandchildren, [=]( auto& v){
               return v.pid == pid;
            });

            if( found) 
               return *found;

            return {};
         }

         common::process::Handle State::singleton( const common::Uuid& id) const
         {
            auto found = range::find( singletons, id);
            if( found)
            {
               return found->second;
            }
            return {};
         }

         bool State::execute()
         {
            tasks.execute();

            return ! ( runlevel() >= Runlevel::shutdown && tasks.empty());
         }


         void State::runlevel( Runlevel runlevel)
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

               common::range::append( group.resources, resources);
            }

            return range::to_vector( range::unique( range::sort( resources)));
         }

         std::ostream& operator << ( std::ostream& out, const State& state)
         {
            return out << "{ groups: " << range::make( state.groups)
               << ", executables: " << range::make( state.executables)
               << ", tasks: " << state.tasks
                  << '}';
         }


      } // manager
   } // domain
} // casual
