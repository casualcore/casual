//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "domain/manager/transform.h"
#include "domain/manager/task.h"

#include "common/domain.h"
#include "common/environment/string.h"

#include "common/code/raise.h"
#include "common/code/casual.h"

#include "common/algorithm/compare.h"
#include "common/array.h"

#include "common/build.h"


namespace casual
{
   using namespace common;

   namespace domain::manager::transform
   {
      namespace local
      {
         namespace
         {
            namespace verify
            {

               auto alias()
               {
                  return [ mapping = std::map< std::string, std::size_t>{}]( auto& process) mutable
                  {
                     if( process.alias.empty())
                     {
                        process.alias = process.path.filename();

                        if( process.alias.empty())
                           code::raise::error( code::casual::invalid_configuration, "executables has to have a path - process: ", process);
                     }

                     auto potentally_add_version = []( auto& mapping, auto& process)
                     {
                        auto count = ++mapping[ process.alias];

                        if( count == 1)
                           return false;

                        process.alias = process.alias + "." + std::to_string( count);
                        return true;
                     };

                     while( potentally_add_version( mapping, process))
                        ; // no-op
                  };
               }

            } // verify


            std::vector< manager::state::Group::id_type> membership( const std::vector< std::string>& members, const std::vector< manager::state::Group>& groups)
            {
               std::vector< manager::state::Group::id_type> result;

               for( auto& name : members)
               {
                  auto found = common::algorithm::find( groups, name);

                  if( found)
                     result.push_back( found->id);
                  else
                     code::raise::error( code::casual::invalid_configuration, "unresolved dependency to group '", name, "'" );
               }

               return result;
            }

            auto group( manager::State& state)
            {
               return [&state]( auto& group)
               {
                  auto transform_id = [&state]( auto& name) 
                  {
                     if( auto found = algorithm::find( state.groups, name))
                        return found->id;

                     code::raise::error( code::casual::invalid_configuration, "unresolved dependency to group '", name, "'" );
                  };

                  manager::state::Group result{ group.name, { state.group_id.master}, group.note};
                  result.dependencies = algorithm::transform( group.dependencies, transform_id);

                  return result;
               };
            }


            namespace transform
            {
               namespace detail
               {
                  template< typename R, typename C>
                  R transform( const C& value, const std::vector< manager::state::Group>& groups)
                  {
                     R result;

                     result.alias = value.alias;
                     result.arguments = value.arguments;
                     result.instances.resize( value.instances);
                     result.note = value.note;
                     result.path = value.path;
                     result.restart = value.lifetime.restart;

                     result.environment.variables = value.environment.variables;

                     result.memberships = local::membership( value.memberships, groups);

                     // If empty, we make it member of '.global'
                     if( result.memberships.empty())
                        result.memberships = local::membership( { ".global"}, groups);


                     return result;
                  }

                  manager::state::Executable executable( const configuration::model::domain::Executable& value, const std::vector< manager::state::Group>& groups)
                  {
                     return transform< manager::state::Executable>( value, groups);
                  }

                  manager::state::Server executable( const configuration::model::domain::Server& value, const std::vector< manager::state::Group>& groups)
                  {
                     auto result = transform< manager::state::Server>( value, groups);


                     return result;
                  }
                  
               } // detail

               auto executable( const std::vector< manager::state::Group>& groups)
               {
                  return [&groups]( auto& value)
                  {
                     return detail::executable( value, groups);
                  };
               }
            } // transform


            namespace model
            {
               auto group()
               {
                  return []( const manager::state::Group& value)
                  {
                     manager::admin::model::Group result;

                     result.id = value.id.value();
                     result.name = value.name;
                     result.note = value.note;

                     result.dependencies = algorithm::transform( value.dependencies, []( auto& id)
                     {
                        return id.value();
                     });

                     return result;
                  };
               }

               namespace detail
               {

                  template< typename R>
                  auto instance()
                  {
                     return []( auto& value)
                     {
                        auto state = []( auto state)
                        {
                           using IN = decltype( state);
                           using OUT = manager::admin::model::instance::State;

                           switch( state)
                           {
                              case IN::running: return OUT::running;
                              case IN::spawned: return OUT::spawned;
                              case IN::scale_out: return OUT::scale_out;
                              case IN::scale_in: return OUT::scale_in;
                              case IN::exit: return OUT::exit;
                              case IN::error: return OUT::error;
                           }
                           return OUT::error;

                        };

                        R result;
                        result.handle = value.handle;
                        result.state = state( value.state);
                        result.spawnpoint = value.spawnpoint;
                        return result;
                     };
                  }

                  template< typename R, typename T>
                  auto transform( const T& value)
                  {
                     R result;

                     result.id = value.id.value();
                     result.alias = value.alias;
                     result.path = value.path;
                     result.arguments = value.arguments;
                     result.note = value.note;
                     using instance_type = typename R::instance_type;
                     result.instances = algorithm::transform( value.instances, detail::instance< instance_type>());
                     result.memberships = algorithm::transform( value.memberships, []( auto id){
                        return id.value();
                     });

                     result.environment.variables = algorithm::transform( value.environment.variables, []( auto& v)
                     {  
                        return static_cast< const std::string&>( v);
                     });

                     result.restart = value.restart;
                     result.restarts = value.restarts;

                     return result;
                  }
                  
               } // detail

               auto excecutable()
               {
                  return []( const manager::state::Executable& value)
                  {
                     return detail::transform< manager::admin::model::Executable>( value);
                  };
               }

               auto server( const manager::State& state)
               {
                  return []( const manager::state::Server& value)
                  {
                     auto result = detail::transform< manager::admin::model::Server>( value);

                     return result;
                  };
               }

            
               auto tasks( const manager::task::Queue& tasks)
               {
                  manager::admin::model::State::Tasks result;

                  auto transform_task = []( auto& task)
                  {
                     manager::admin::model::Task result;
                     result.id = task.context().id.value();
                     result.description = task.context().descripton;
                     return result;
                  };

                  result.running = algorithm::transform( tasks.running(), transform_task);
                  result.pending = algorithm::transform( tasks.pending(), transform_task);
                  
                  return result;
               }

            } // model

         } // <unnamed>
      } // local


      manager::admin::model::State state( const manager::State& state)
      {
         manager::admin::model::State result;

         auto transform_runlevel = []( auto runlevel)
         {
            using Result = manager::admin::model::state::Runlevel;
            switch( runlevel)
            {
               using Source = decltype( runlevel);
               case Source::startup: return Result::startup;
               case Source::running: return Result::running;
               case Source::shutdown: return Result::shutdown;
               case Source::error: return Result::error;
            }
            return Result::error;
         };

         result.runlevel = transform_runlevel( state.runlevel());

         result.version = build::version();
         result.identity = common::domain::identity();

         result.groups = algorithm::transform( state.groups, local::model::group());
         result.servers = algorithm::transform( state.servers, local::model::server( state));
         result.executables = algorithm::transform( state.executables, local::model::excecutable());
         //result.event = local::model::event( state.event);
         result.tasks = local::model::tasks( state.tasks);

         return result;
      }


      manager::State model( configuration::Model model)
      {
         Trace trace{ "domain::transform::state"};
         log::line( verbose::log, "configuration: ", model);


         
         // Set the domain
         common::domain::identity( common::domain::Identity{ environment::string( model.domain.name)});
         manager::State result;

         result.configuration.model = std::move( model);
         auto& domain = result.configuration.model.domain;

         // Handle groups
         {
            manager::state::Group core{ ".casual.core", {}, "domain-manager internal group"};
            result.group_id.core = core.id;

            manager::state::Group master{ ".casual.master", { result.group_id.core}, "the master and (implicit) parent of all groups"};
            result.group_id.master = master.id;
            
            manager::state::Group transaction{ ".casual.transaction", { result.group_id.master}};
            result.group_id.transaction = transaction.id;
            
            manager::state::Group queue{ ".casual.queue", { transaction.id}};
            result.group_id.queue = queue.id;
            
            manager::state::Group global{ ".global", { queue.id, transaction.id}, "user global group"};
            result.group_id.global = global.id;

            result.groups.push_back( std::move( core));
            result.groups.push_back( std::move( master));
            result.groups.push_back( std::move( transaction));
            result.groups.push_back( std::move( queue));
            result.groups.push_back( std::move( global));
         }

         {
            // We need to remove any of the reserved groups (that we created above), either because
            // the user has used one of the reserved names, or we're reading from a persistent stored
            // configuration
            const std::vector< std::string> reserved{
               ".casual.domain", ".casual.master", ".casual.transaction", ".casual.queue", ".global", ".casual.gateway"};

            auto groups = common::algorithm::remove_if( domain.groups, [&reserved]( const auto& g)
            {
               return common::algorithm::find( reserved, g.name);
            });

            // We transform user defined groups
            algorithm::transform( groups, result.groups, local::group( result));
         }

         {
            // We need to make sure the gateway have dependencies to all user groups. We could
            // order the groups and pick the last one, but it's more semantic correct to have dependencies
            // to all, since that is exactly what we're trying to represent.
            manager::state::Group gateway{ ".casual.gateway", {}};
            result.group_id.gateway = gateway.id;

            for( auto& group : result.groups)
            {
               gateway.dependencies.push_back( group.id);
            }
            result.groups.push_back( std::move( gateway));
         }

         // Handle executables
         {
            // Add our self to processes that this domain has. Mostly to
            // make it symmetric
            {

               manager::state::Server manager;
               result.manager_id = manager.id;
               manager.alias = "casual-domain-manager";
               manager.path = "casual-domain-manager";
               manager.memberships.push_back( result.group_id.master);
               manager.note = "responsible for all executables in this domain";

               manager::state::Server::instance_type instance{ common::process::handle()};
               instance.state = manager::state::Server::state_type::running;
               instance.spawnpoint = platform::time::clock::type::now();
               manager.instances.push_back( std::move( instance));

               result.servers.push_back( std::move( manager));
            }

            algorithm::append( algorithm::transform( domain.servers, local::transform::executable( result.groups)), result.servers);
            algorithm::append( algorithm::transform( domain.executables, local::transform::executable( result.groups)), result.executables);

            auto verify = local::verify::alias();

            algorithm::for_each( result.servers, verify);
            algorithm::for_each( result.executables, verify);

         }

         return result;
      }

      std::vector< manager::state::Executable> alias( 
         const std::vector< configuration::model::domain::Executable>& values, 
         const std::vector< manager::state::Group>& groups)
      {
         Trace trace{ "domain::transform::alias"};

         return algorithm::transform( values, local::transform::executable( groups));
      }

      std::vector< manager::state::Server> alias( 
         const std::vector< configuration::model::domain::Server>& values, 
         const std::vector< manager::state::Group>& groups)
      {
         Trace trace{ "domain::transform::alias"};

         return algorithm::transform( values, local::transform::executable( groups));
      }


      configuration::Model model( const manager::State& state)
      {
         configuration::Model result;

         auto name_groups = [&state]( auto& ids)
         {
            return algorithm::transform( ids, [&state]( auto id)
            {
               return state.group( id).name;
            });
         };

         result.domain.name = common::domain::identity().name;
         result.domain.environment = state.configuration.model.domain.environment;
         
         result.domain.groups = algorithm::transform_if( state.groups, [&name_groups]( auto& group)
         {
            configuration::model::domain::Group result;
            result.name = group.name;
            result.note = group.note;
            result.dependencies = name_groups( group.dependencies);
            return result;
         }, []( auto& group)
         {
            // leading `.` is reserved for casual internal...
            return group.name.empty() || group.name[ 0] != '.';
         });

         auto reserved_groups = array::make( state.group_id.master, state.group_id.core, state.group_id.transaction, state.group_id.queue, state.group_id.gateway);

         result.domain.servers = algorithm::transform_if( state.servers, [&name_groups]( auto& server)
         {
            configuration::model::domain::Server result;
            result.alias = server.alias;
            result.path = server.path;
            result.arguments = server.arguments;
            result.environment.variables = server.environment.variables;
            result.memberships = name_groups( server.memberships);
            result.instances = server.instances.size();
            result.note = server.note;

            return result;
         }, [&reserved_groups]( auto& server)
         {
            return ! predicate::boolean( algorithm::find_first_of( server.memberships, reserved_groups));
         });

         result.domain.executables = algorithm::transform( state.executables, [&name_groups]( auto& executable)
         {
            configuration::model::domain::Executable result;
            result.alias = executable.alias;
            result.path = executable.path;
            result.arguments = executable.arguments;
            result.environment.variables = executable.environment.variables;
            result.memberships = name_groups( executable.memberships);
            result.instances = executable.instances.size();
            result.note = executable.note;

            return result;
         });

         

         return result;
      }

   } // domain::manager::transform
} // casual
