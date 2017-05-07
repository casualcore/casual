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
                  void ipc( platform::ipc::id::type ipc)
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
                     << ", configured-instances: " << value.configured_instances
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


            bool Group::boot::Order::operator () ( const Group& lhs, const Group& rhs)
            {
               auto rhs_depend = range::find( rhs.dependencies, lhs.id);
               auto lhs_depend = range::find( lhs.dependencies, rhs.id);

               return rhs_depend && ! lhs_depend;
            }


            bool Executable::remove( pid_type instance)
            {
               auto found = range::find( instances, instance);

               if( found)
               {
                  instances.erase( std::begin( found));
                  return true;
               }
               return false;
            }

            bool Executable::offline() const
            {
               return instances.empty();
            }

            bool Executable::online() const
            {
               return configured_instances == 0 || ! instances.empty();
            }

            bool Executable::complete() const
            {
               return instances.size() >= configured_instances;
            }

            std::ostream& operator << ( std::ostream& out, const Executable& value)
            {
               out << "{ ";
               local::print_executables( out, value);
               return out << '}';
            }

            common::process::Handle Server::process( common::platform::pid::type pid) const
            {
               auto found = range::find_if( instances, [pid]( auto& p){
                  return p.pid == pid;
               });

               if( found)
               {
                  return *found;
               }
               return {};
            }

            common::process::Handle Server::remove( common::platform::pid::type pid)
            {
               auto found = range::find_if( instances, [pid]( auto& p){
                  return p.pid == pid;
               });

               common::process::Handle result;

               if( found)
               {
                  result = *found;

                  instances.erase( std::begin( found));
               }
               return result;
            }

            bool Server::connect( common::process::Handle process)
            {
               auto found = range::find_if( instances, common::process::Handle::equal::pid( process.pid));

               if( found)
               {
                  *found = process;
               }
               return found;
            }

            bool Server::offline() const
            {
               return instances.empty();
            }

            bool Server::online() const
            {
               return configured_instances == 0
                     || ( ! instances.empty()
                           && range::all_of( instances, []( auto& p){ return p;}));
            }

            bool Server::complete() const
            {
               return instances.size() >= configured_instances;
            }

            std::ostream& operator << ( std::ostream& out, const Server& value)
            {
               out << "{ ";
               local::print_executables( out, value);
               return out << ", resources: " << range::make( value.resources)
                  << ", restrictions: " << range::make( value.restrictions)
                  << '}';
            }

            bool operator == ( const Server& lhs, common::platform::pid::type rhs)
            {
               return lhs.process( rhs).pid == rhs;
            }


            Batch::Batch( State& state, Group::id_type group) : group( group), m_state( state) {}

            std::chrono::milliseconds Batch::timeout() const
            {
               std::chrono::milliseconds result{ 4000};
               constexpr std::chrono::milliseconds timeout{ 400};

               range::for_each( servers, [&]( const auto& e){ result +=  this->state().server( e.id).instances.size() * timeout;});
               range::for_each( executables, [&]( const auto& e){ result +=  this->state().executable( e.id).instances.size() * timeout;});

               return result;
            }


            bool Batch::online() const
            {
               return range::all_of( servers, [&]( auto task){
                  return this->state().server( task.id).online();
               })
               && range::all_of( executables, [&]( auto task){
                  return this->state().executable( task.id).online();
               });
            }

            bool Batch::offline() const
            {
               return range::all_of( servers, [&]( auto task){
                  return this->state().server( task.id).offline();
               })
               && range::all_of( executables, [&]( auto task){
                  return this->state().executable( task.id).offline();
               });
            }

            const State& Batch::state() const { return m_state.get();}
            State& Batch::state() { return m_state.get();}


            std::ostream& operator << ( std::ostream& out, const Batch& value)
            {
               return out << "{ group: " << value.group
                     << ", servers: " << range::make( value.servers)
                     << ", executables: " << range::make( value.executables)
                     << ", timeout: " << value.timeout().count() << "ms}";
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
                        state::Batch batch{ state, group.get().id};

                        auto extract = [&]( auto& entites, auto& output){

                           //
                           // Partition executables so we get the ones that has current group as a dependency
                           //
                           auto slice = range::stable_partition( entites, [&]( const auto& e){
                              return static_cast< bool>( range::find( e.get().memberships, group.get().id));
                           });

                           range::transform( std::get< 0>( slice), output, []( auto& e){
                              common::traits::concrete::type_t< decltype( range::front( output))> result;
                              result.id = e.get().id;
                              result.instances = e.get().configured_instances;
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

         void State::remove_process( common::platform::pid::type pid)
         {
            Trace trace{ "domain::manager::State::remove_process"};

            //
            // Vi remove from event listeners if one of them has died
            //
            {
               event.remove( pid);
            }

            message::domain::scale::Executable restart;

            auto restart_process = [&]( auto& executable, auto& output){

               if( ! executable.complete() && executable.restart && m_runlevel == Runlevel::running)
               {
                  message::domain::scale::Executable::Scale scale;
                  scale.id = common::id::underlaying( executable.id);
                  scale.instances = executable.configured_instances;
                  output.push_back( std::move( scale));
               }
            };

            //
            // Check if it's a server
            //
            {
               auto found = range::find( servers, pid);

               if( found)
               {
                  auto process = found->remove( pid);
                  //
                  // Try to remove ipc-queue (no-op if it's removed already)
                  //
                  local::remove::ipc( process.queue);

                  restart_process( *found, restart.servers);
               }
            }

            //
            // Find and remove from executable
            //
            {
               auto found = range::find_if( executables, [pid]( state::Executable& e){
                  return e.remove( pid);
               });

               if( found)
               {
                  log << "removed executable with pid: " << pid << '\n';

                  restart_process( *found, restart.executables);
               }
            }

            //
            // Remove from singeltons
            //
            {
               auto found = range::find_if( singeltons, [pid]( auto& v){
                  return v.second.pid == pid;
               });

               if( found)
               {
                  singeltons.erase( std::begin( found));
               }
            }

            if( restart)
            {
               communication::ipc::inbound::device().push( restart);
            }

         }


         state::Server* State::server( common::platform::pid::type pid)
         {
            return range::find_if( servers, [pid]( const auto& s){
               return s.process( pid).pid == pid;
            }).data();
         }


         state::Executable* State::executable( common::platform::pid::type pid)
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

         const state::Server& State::server( state::Server::id_type id) const
         {
            return range::front( range::find_if( servers, [id]( const auto& s){
               return s.id == id;
            }));
         }

         const state::Executable& State::executable( state::Executable::id_type id) const
         {
            return range::front( range::find_if( executables, [id]( const auto& e){
               return e.id == id;
            }));
         }

         common::process::Handle State::singleton( const common::Uuid& id) const
         {
            auto found = range::find( singeltons, id);
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


         std::vector< std::string> State::resources( common::platform::pid::type pid)
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
