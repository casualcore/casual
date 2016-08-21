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
            } // <unnamed>
         } // local

         namespace state
         {

            std::ostream& operator << ( std::ostream& out, const Group::Resource& value)
            {
               return out << "{ id: " << value.id
                     << ", key: " << value.key
                     << ", openinfo: " << value.openinfo
                     << ", closeinfo: " << value.closeinfo
                     << '}';
            }

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
               return out << "{ id: " << value.id
                     << ", alias: " << value.alias
                     << ", path: " << value.path
                     << ", arguments: " << range::make( value.arguments)
                     << ", restart: " << value.restart
                     << ", memberships: " << range::make( value.memberships)
                     << ", configured-instances: " << value.configured_instances
                     << ", instances: " << range::make( value.instances)
                     << '}';
            }

            Batch::Batch( const Group& group) : group{ group} {}

            std::chrono::milliseconds Batch::timeout() const
            {
               std::chrono::milliseconds result{ 4000};
               constexpr std::chrono::milliseconds timeout{ 400};

               range::for_each( executables, [&]( const Executable& e){ result +=  e.instances.size() * timeout;});

               return result;
            }


            bool Batch::online() const
            {
               return range::all_of( executables, []( const Executable& e){ return e.online();});
            }

            bool Batch::offline() const
            {
               return range::all_of( executables, []( const Executable& e){ return e.offline();});
            }

            std::ostream& operator << ( std::ostream& out, const Batch& value)
            {
               return out << "{ group: " << value.group << ", executables: " << range::make( value.executables) << ", timeout: " << value.timeout().count() << "ms}";
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

                     std::vector< std::reference_wrapper< state::Executable>> excutable_wrappers;

                     //
                     // We make sure we don't include our self in the boot sequence.
                     //
                     range::copy_if( state.executables, std::back_inserter( excutable_wrappers), [&state]( const state::Executable& e){
                        return e.id != state.global.manager;
                     });

                     auto executable = range::make( excutable_wrappers);

                     //
                     // Reverse the order, so we 'consume' executable based on the group
                     // that is the farthest in the dependency chain
                     //
                     for( auto& group : range::reverse( groups))
                     {
                        state::Batch batch{ group};

                        //
                        // Partition executables so we get the ones that has current group as a dependency
                        //
                        auto slice = range::stable_partition( executable, [&]( const state::Executable& e){
                           return static_cast< bool>( range::find( e.memberships, group.get().id));
                        });

                        //
                        // copy and consume the executables
                        //
                        range::copy( std::get< 0>( slice), std::back_inserter( batch.executables));
                        executable = std::get< 1>( slice);

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
            // Vi remove from listeners if one of them has died
            //
            {
               auto found = range::find_if( termination.listeners, common::process::Handle::equal::pid{ pid});
               if( found)
               {
                  termination.listeners.erase( std::begin( found));
               }
            }


            //
            // Find and remove process
            //
            {
               auto found = range::find( processes, pid);

               if( found)
               {
                  //
                  // Try to remove ipc-queue (no-op if it's removed already)
                  //
                  local::remove::ipc( found->second.queue);

                  processes.erase( std::begin( found));
               }
               else
               {
                  log << "process with pid: " << pid << " has not register - action: assume it's a basic executable\n";
               }
            }

            //
            // Remove from instances
            //
            {
               using value_type = decltype( *std::begin( singeltons));
               auto found = range::find_if( singeltons, [&]( value_type& v){
                  return v.second.pid == pid;
               });

               if( found)
               {
                  singeltons.erase( std::begin( found));
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

                  if( ! found->complete() && found->restart)
                  {
                     communication::ipc::inbound::device().push( message::domain::scale::Executable{ found->id });
                  }
               }
               else
               {
                  log << "failed to find executable with pid: " << pid << " - assume it's a grand child";
               }
            }
         }

         state::Executable& State::executable( common::platform::pid::type pid)
         {
            auto found = range::find_if( executables, [=]( const state::Executable& e){
               return range::find( e.instances, pid) == true;
            });
            if( found)
            {
               return *found;
            }
            throw exception::invalid::Argument{ "failed to locate executable", CASUAL_NIP( pid)};
         }

         state::Group& State::group( state::Group::id_type id)
         {
            auto found = range::find_if( groups, [=]( const state::Group& g){
               return g.id == id;
            });
            if( found)
            {
               return *found;
            }
            throw exception::invalid::Argument{ "failed to locate group", CASUAL_NIP( id)};
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
