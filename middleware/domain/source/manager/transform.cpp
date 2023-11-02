//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "domain/manager/transform.h"
#include "domain/manager/task.h"

#include "common/domain.h"
#include "common/environment/expand.h"

#include "common/code/raise.h"
#include "common/code/casual.h"

#include "common/algorithm/compare.h"
#include "common/array.h"

#include "common/build.h"

#include "casual/assert.h"


namespace casual
{
   using namespace common;

   namespace domain::manager::transform
   {
      namespace local
      {
         namespace
         {

            auto membership( const std::vector< std::string>& members, const std::vector< manager::state::Group>& groups)
            {
               return algorithm::accumulate( members, std::vector< manager::state::Group::id_type>{}, [&groups]( auto result, auto& member)
               {
                  if( auto found = common::algorithm::find( groups, member))
                     result.push_back( found->id);
                  else
                     code::raise::error( code::casual::invalid_configuration, "unresolved dependency to group '", member, "' - dependency to non existing group?" );
                  
                  return result;
               });
            }

            template< typename Groups>
            void groups( const manager::State& state, Groups&& source, std::vector< manager::state::Group>& target)
            {
               // 'two phase' - we add all groups, and then take care of dependencies. Hence, we don't rely on order...
               algorithm::transform( source, std::back_inserter( target), [master = state.group_id.master]( auto& group)
               {
                  return manager::state::Group{ group.name, { master}, group.note};
               });

               // take care of dependencies
               for( auto& group : source)
               {
                  auto found = algorithm::find( target, group.name);
                  casual::assertion( found, "failed to lookup group ", group.name);
                  found->dependencies = local::membership( group.dependencies, target);
               }
            }

            template< typename C, typename T>
            void modify( const C& value, T& target, const std::vector< manager::state::Group>& groups)
            {
               target.alias = value.alias;
               target.arguments = value.arguments;
               target.scale( value.instances);
               target.note = value.note;
               target.path = value.path;
               target.restart = value.lifetime.restart;

               target.environment.variables = value.environment.variables;

               target.memberships = local::membership( value.memberships, groups);

               // If empty, we make it member of '.global'
               if( target.memberships.empty())
                  target.memberships = local::membership( { ".global"}, groups);

            }
         } // <unnamed>
      } // local

      void modify( const casual::configuration::model::domain::Executable& source, manager::state::Executable& target, const std::vector< manager::state::Group>& groups)
      {
         local::modify( source, target, groups);
      }

      void modify( const casual::configuration::model::domain::Server& source, manager::state::Server& target, const std::vector< manager::state::Group>& groups)
      {
         local::modify( source, target, groups);
      }

      namespace local
      {
         namespace
         {

            namespace transform
            {
               namespace detail
               {
                  manager::state::Executable executable( const configuration::model::domain::Executable& value, const std::vector< manager::state::Group>& groups)
                  {
                     auto result = manager::state::Executable::create();
                     local::modify( value, result, groups);
                     return result;
                  }

                  manager::state::Server executable( const configuration::model::domain::Server& value, const std::vector< manager::state::Group>& groups)
                  {
                     auto result = manager::state::Server::create();
                     local::modify( value, result, groups);
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
                     result.restarts = value.initiated_restarts;

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
                     result.description = task.context().description;
                     return result;
                  };

                  result.running = algorithm::transform( tasks.running(), transform_task);
                  result.pending = algorithm::transform( tasks.pending(), transform_task);
                  
                  return result;
               }

               auto grandchild()
               {
                  return []( const manager::state::Grandchild& grandchild)
                  {
                     manager::admin::model::Grandchild result;
                     result.handle = grandchild.handle;
                     result.alias = grandchild.alias;
                     result.path = grandchild.path;

                     return result;
                  };
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
         result.grandchildren = algorithm::transform( state.grandchildren, local::model::grandchild());

         return result;
      }


      manager::State model( configuration::Model model)
      {
         Trace trace{ "domain::transform::state"};
         log::line( verbose::log, "configuration: ", model);
         
         // Set the domain
         common::domain::identity( common::domain::Identity{ environment::expand( model.domain.name)});
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
            constexpr auto reserved = common::array::make( 
               std::string_view{ ".casual.domain"}, ".casual.master", ".casual.transaction", ".casual.queue", ".global", ".casual.gateway");

            auto groups = common::algorithm::remove_if( domain.groups, [&reserved]( const auto& g)
            {
               return common::algorithm::find( reserved, g.name);
            });

            local::groups( result, groups, result.groups);
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

               manager::state::Server manager{ strong::server::id::generate()};
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
         }

         return result;
      }


      std::vector< manager::state::Executable> alias( 
         detail::range::Executables values, 
         const std::vector< manager::state::Group>& groups)
      {
         Trace trace{ "domain::transform::alias"};

         return algorithm::transform( values, local::transform::executable( groups));
      }

      std::vector< manager::state::Server> alias( 
         detail::range::Servers values, 
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

         result.system = state.configuration.model.system;

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
