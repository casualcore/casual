//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "domain/transform.h"
#include "domain/manager/task.h"

#include "configuration/domain.h"
#include "configuration/message/transform.h"

#include "common/domain.h"
#include "common/message/gateway.h"

#include "common/code/raise.h"
#include "common/code/casual.h"


namespace casual
{
   using namespace common;

   namespace domain
   {
      namespace transform
      {
         namespace local
         {
            namespace
            {
               namespace verify
               {

                  struct Alias
                  {

                     template< typename E>
                     void operator () ( E& process)
                     {
                        if( process.alias.empty())
                        {
                           process.alias = file::name::base( process.path);

                           if( process.alias.empty())
                              code::raise::error( code::casual::invalid_configuration, "executables has to have a path - process: ", process);
                        }

                        auto& count = m_mapping[ process.alias];
                        ++count;

                        if( count > 1)
                        {
                           process.alias = process.alias + "_" + std::to_string( count);

                           // Just to make sure we don't get duplicates if users has configure aliases
                           // such as 'alias_1', and so on, we do another run
                           operator ()( process);
                        }
                     }

                     std::map< std::string, std::size_t> m_mapping;
                  };

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

               struct Group
               {

                  Group( const manager::State& state) : m_state( state) {}

                  manager::state::Group operator () ( const casual::configuration::Group& group) const
                  {
                     manager::state::Group result{ group.name, { m_state.group_id.master}, group.note};

                     if( group.resources.has_value()) 
                        result.resources = group.resources.value();

                     if( group.dependencies)
                     {
                        for( auto& dependency : group.dependencies.value())
                           result.dependencies.push_back( id( dependency));
                     }

                     return result;
                  }

               private:

                  manager::state::Group::id_type id( const std::string& name) const
                  {
                     auto has_name = [&name]( auto& group){ return group.name == name;};

                     if( auto found = algorithm::find_if( m_state.groups, has_name))
                        return found->id;

                     code::raise::error( code::casual::invalid_configuration, "unresolved dependency to group '", name, "'" );
                  }

                  const manager::State& m_state;

               };

               namespace transform
               {
                  namespace detail
                  {
                     template< typename R, typename C>
                     R transform( const C& value, const std::vector< manager::state::Group>& groups)
                     {
                        R result;

                        result.alias = value.alias.value_or( "");
                        result.arguments = value.arguments.value_or( result.arguments);
                        result.instances.resize( value.instances.value_or( 0));
                        result.note = value.note.value_or( "");
                        result.path = value.path;
                        result.restart = value.restart.value_or( false);

                        if( value.environment)
                           result.environment.variables = casual::domain::transform::environment::variables( value.environment.value());

                        if( value.memberships)
                           result.memberships = local::membership( value.memberships.value(), groups);

                        // If empty, we make it member of '.global'
                        if( result.memberships.empty())
                           result.memberships = local::membership( { ".global"}, groups);


                        return result;
                     }

                     manager::state::Executable executable( const configuration::Executable& value, const std::vector< manager::state::Group>& groups)
                     {
                        return transform< manager::state::Executable>( value, groups);
                     }

                     manager::state::Server executable( const configuration::Server& value, const std::vector< manager::state::Group>& groups)
                     {
                        auto result = transform< manager::state::Server>( value, groups);

                        if( value.resources)
                           result.resources = value.resources.value();

                        if( value.restrictions)
                           result.restrictions = value.restrictions.value();

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
                  struct Group
                  {
                     manager::admin::model::Group operator () ( const manager::state::Group& value)
                     {
                        manager::admin::model::Group result;

                        result.id = value.id.value();
                        result.name = value.name;
                        result.note = value.note;

                        result.dependencies = algorithm::transform( value.dependencies, []( auto& id){
                           return id.value();
                        });
                        result.resources = value.resources;

                        return result;
                     }

                  };

                  struct Executable
                  {
                     manager::admin::model::Executable operator () ( const manager::state::Executable& value)
                     {
                        return transform< manager::admin::model::Executable>( value);
                     }

                     manager::admin::model::Server operator () ( const manager::state::Server& value)
                     {
                        auto result = transform< manager::admin::model::Server>( value);

                        result.resources = value.resources;
                        result.restriction = value.restrictions;

                        return result;
                     }

                  private:

                     struct Instance
                     {
                        manager::admin::model::Executable::instance_type operator () ( const manager::state::Executable::instance_type& value)
                        {
                           return transform< manager::admin::model::Executable::instance_type>( value);
                        }

                        manager::admin::model::Server::instance_type operator () ( const manager::state::Server::instance_type& value)
                        {
                           return transform< manager::admin::model::Server::instance_type>( value);
                        }
                     private:
                        template< typename R, typename T> 
                        R transform( const T& value)
                        {
                           R result;
                           result.handle = value.handle;
                           result.state = state( value.state);
                           result.spawnpoint = value.spawnpoint;
                           return result;
                        }

                        template< typename S>
                        manager::admin::model::instance::State state( S state)
                        {
                           switch( state)
                           {
                              case S::running: return manager::admin::model::instance::State::running;
                              case S::spawned: return manager::admin::model::instance::State::spawned;
                              case S::scale_out: return manager::admin::model::instance::State::scale_out;
                              case S::scale_in: return manager::admin::model::instance::State::scale_in;
                              case S::exit: return manager::admin::model::instance::State::exit;
                              case S::error: return manager::admin::model::instance::State::error;
                           }
                           return manager::admin::model::instance::State::error;
                        }
                     };

                     template< typename R, typename T>
                     R transform( const T& value)
                     {
                        R result;

                        result.id = value.id.value();
                        result.alias = value.alias;
                        result.path = value.path;
                        result.arguments = value.arguments;
                        result.note = value.note;
                        result.instances = algorithm::transform( value.instances, Instance{});
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

                  };

                  auto tasks( const manager::task::Queue& tasks)
                  {
                     manager::admin::model::State::Tasks result;

                     auto transform_task = []( auto& task)
                     {
                        manager::admin::model::Task result;
                        result.id = task.context().id;
                        result.description = task.context().descripton;
                        return result;
                     };

                     result.running = algorithm::transform( tasks.running(), transform_task);
                     result.pending = algorithm::transform( tasks.pending(), transform_task);
                     
                     return result;
                  }

                  // TODO maintenance: This should perhaps be in a more central place?
                  auto version()
                  {
                     manager::admin::model::Version result;

                     // casual version
#ifdef CASUAL_BUILD_VERSION
                     result.casual = CASUAL_BUILD_VERSION;
#endif 
                     // compiler version
#ifdef __clang_version__
                     result.compiler = string::compose( "clang: ", __clang_version__);
#elif __GNUC__
                     result.compiler = string::compose( "g++: ", __GNUC__, '.', __GNUC_MINOR__, '.', __GNUC_PATCHLEVEL__);
#endif

                     // gateway protocols
                     result.gateway.protocols = { 
                        cast::underlying( message::gateway::domain::protocol::Version::version_1)
                     };
                     static_assert( message::gateway::domain::protocol::Version::version_1 == message::gateway::domain::protocol::Version::latest, "fix versions...");

                     return result;
                  }

               } // model

            } // <unnamed>
         } // local

         std::vector< manager::state::Executable> executables( const std::vector< casual::configuration::Executable>& values, const std::vector< manager::state::Group>& groups)
         {
            return algorithm::transform( values, local::transform::executable( groups));
         }

         std::vector< manager::state::Server> executables( const std::vector< casual::configuration::Server>& values, const std::vector< manager::state::Group>& groups)
         {
            return algorithm::transform( values, local::transform::executable( groups));
         }


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

            result.version = local::model::version();
            result.identity = common::domain::identity();

            result.groups = algorithm::transform( state.groups, local::model::Group{});
            result.servers = algorithm::transform( state.servers, local::model::Executable{});
            result.executables = algorithm::transform( state.executables, local::model::Executable{});
            //result.event = local::model::event( state.event);
            result.tasks = local::model::tasks( state.tasks);

            return result;
         }


         manager::State state( casual::configuration::domain::Manager domain)
         {
            Trace trace{ "domain::transform::state"};

            log::line( verbose::log, "configuration: ", domain);

            // Set the domain
            common::domain::identity( common::domain::Identity{ domain.name});

            manager::State result;

            result.configuration = casual::configuration::transform::configuration( domain);
            result.environment = domain.manager_default.environment;

            // Handle groups
            {               
               manager::state::Group master{ ".casual.master", {}, "the master and (implicit) parent of all groups"};
               result.group_id.master = master.id;
               
               manager::state::Group transaction{ ".casual.transaction", { result.group_id.master}};
               result.group_id.transaction = transaction.id;
               
               manager::state::Group queue{ ".casual.queue", { transaction.id}};
               result.group_id.queue = queue.id;
               
               manager::state::Group global{ ".global", { queue.id, transaction.id}, "user global group"};
               result.group_id.global = global.id;

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
               algorithm::transform( groups, result.groups, local::Group{ result});
            }

            {
               // We need to make sure the gateway have dependencies to all user groups. We could
               // order the groups and pick the last one, but it's more semantic correct to make have dependencies
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

               algorithm::append( transform::executables( domain.servers, result.groups), result.servers);
               algorithm::append( transform::executables( domain.executables, result.groups), result.executables);

               local::verify::Alias verify;

               algorithm::for_each( result.servers, verify);
               algorithm::for_each( result.executables, verify);

            }

            return result;
         }

         namespace environment
         {
            std::vector< common::environment::Variable> variables( const casual::configuration::Environment& environment)
            {
               return casual::configuration::environment::transform( casual::configuration::environment::fetch( environment));
            }

         } // environment
         


      } // transform
   } // domain
} // casual
