//!
//! casual 
//!

#include "domain/transform.h"

#include "configuration/domain.h"
#include "configuration/message/transform.h"

#include "common/domain.h"

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

                     void operator () ( manager::state::Executable& executable)
                     {
                        if( executable.alias.empty())
                        {
                           executable.alias = file::name::base( executable.path);

                           if( executable.alias.empty())
                           {
                              throw exception::invalid::Configuration{ "executables has to have a path", CASUAL_NIP( executable)};
                           }
                        }



                        auto& count = m_mapping[ executable.alias];
                        ++count;

                        if( count > 1)
                        {
                           executable.alias = executable.alias + "_" + std::to_string( count);

                           //
                           // Just to make sure we don't get duplicates if users has configure aliases
                           // such as 'alias_1', and so on, we do another run
                           //
                           operator ()( executable);
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
                     auto found = common::range::find( groups, name);

                     if( found)
                     {
                        result.push_back( found->id);
                     }
                     else
                     {
                        throw exception::invalid::Argument{ "unresolved dependency to group '" + name + "'" };
                     }
                  }

                  return result;
               }

               struct Group
               {

                  Group( const manager::State& state) : m_state( state) {}

                  manager::state::Group operator () ( const casual::configuration::group::Group& group) const
                  {
                     manager::state::Group result{ group.name, { m_state.group_id.global}, group.note};

                     if( group.resources.has_value()) result.resources = group.resources.value();

                     if( group.dependencies)
                     {
                        for( auto& dependency : group.dependencies.value())
                        {
                           result.dependencies.push_back( id( dependency));
                        }
                     }

                     return result;
                  }

               private:

                  manager::state::Group::id_type id( const std::string& name) const
                  {
                     auto found = range::find_if( m_state.groups, [&]( const manager::state::Group& group){
                        return group.name == name;
                     });

                     if( found)
                     {
                        return found->id;
                     }
                     throw exception::invalid::Argument{ "unresolved dependency to group '" + name + "'" };
                  }

                  const manager::State& m_state;

               };



               std::vector< std::string> environment( const casual::configuration::Environment& environment)
               {
                  std::vector< std::string> result;
                  for( auto& variable : casual::configuration::environment::fetch( environment))
                  {
                     result.push_back( variable.key + "=" + variable.value);
                  }
                  return result;
               }


               struct Executable
               {

                  manager::state::Executable operator() ( const configuration::server::Executable& value, const std::vector< manager::state::Group>& groups)
                  {
                     return transform( value, groups);
                  }

                  manager::state::Executable operator() ( const configuration::server::Server& value, const std::vector< manager::state::Group>& groups)
                  {
                     auto result = transform( value, groups);

                     if( value.resources)
                        result.resources = value.resources.value();

                     if( value.restrictions)
                        result.restrictions = value.restrictions.value();

                     return result;
                  }

               private:
                  manager::state::Executable transform( const configuration::server::Executable& value, const std::vector< manager::state::Group>& groups)
                  {
                     manager::state::Executable result;

                     result.alias = value.alias.value_or( "");
                     result.arguments = value.arguments.value_or( result.arguments);
                     result.configured_instances = value.instances.value_or( 0);
                     result.note = value.note.value_or( "");
                     result.path = value.path;
                     result.restart = value.restart.value_or( false);

                     if( value.environment)
                        result.environment.variables = local::environment( value.environment.value());

                     if( value.memberships)
                        result.memberships = local::membership( value.memberships.value(), groups);

                     // If empty, we make it member of '.global'
                     if( result.memberships.empty())
                        result.memberships = local::membership( { ".global"}, groups);


                     return result;
                  }
               };

               template< typename C>
               std::vector< manager::state::Executable> executables( C&& values, const std::vector< manager::state::Group>& groups)
               {
                  return range::transform( values, std::bind( Executable{}, std::placeholders::_1, std::ref( groups)));
               }

               namespace vo
               {

                  struct Group
                  {
                     manager::admin::vo::Group operator () ( const manager::state::Group& value)
                     {
                        manager::admin::vo::Group result;

                        result.id = value.id;
                        result.name = value.name;
                        result.note = value.note;

                        result.dependencies = value.dependencies;
                        result.resources = value.resources;

                        return result;
                     }

                  };

                  struct Executable
                  {
                     manager::admin::vo::Executable operator () ( const manager::state::Executable& value)
                     {
                        manager::admin::vo::Executable result;

                        result.id = value.id;
                        result.alias = value.alias;
                        result.path = value.path;
                        result.arguments = value.arguments;
                        result.note = value.note;
                        result.instances = value.instances;
                        result.memberships = value.memberships;
                        result.environment.variables = value.environment.variables;
                        result.configured_instances = value.configured_instances;
                        result.restart = value.restart;
                        result.restarts = value.restarts;

                        return result;
                     }
                  };
               } // vo

            } // <unnamed>
         } // local

         manager::admin::vo::State state( const manager::State& state)
         {
            manager::admin::vo::State result;

            result.groups = range::transform( state.groups, local::vo::Group{});
            result.executables = range::transform( state.executables, local::vo::Executable{});

            return result;
         }


         manager::State state( casual::configuration::domain::Manager domain)
         {

            //
            // Set the domain
            //
            common::domain::identity( common::domain::Identity{ domain.name});

            manager::State result;

            result.configuration = casual::configuration::transform::configuration( domain);



            //
            // Handle groups
            //
            {
               manager::state::Group master{ ".casual.master", {}, "the master and (implicit) parent of all groups"};
               result.group_id.master = master.id;
               result.groups.push_back( std::move( master));


               manager::state::Group transaction{ ".casual.transaction", { result.group_id.master}};
               result.group_id.transaction = transaction.id;
               result.groups.push_back( std::move( transaction));

               manager::state::Group queue{ ".casual.queue", { transaction.id}};
               result.group_id.queue = queue.id;
               result.groups.push_back( std::move( queue));

               manager::state::Group global{ ".global", { queue.id, transaction.id}, "user global group"};
               result.group_id.global = global.id;
               result.groups.push_back( std::move( global));
            }

            {
               //
               // We need to remove any of the reserved groups (that we created above), either because
               // the user has used one of the reserved names, or we're reading from a persistent stored
               // configuration
               //
               const std::vector< std::string> reserved{
                  ".casual.master", ".casual.transaction", ".casual.queue", ".global", ".casual.gateway"};

               auto groups = common::range::remove_if( domain.groups, [&reserved]( const casual::configuration::group::Group& g){
                  return common::range::find( reserved, g.name);
               });


               //
               // We transform user defined groups
               //
               range::transform( groups, result.groups, local::Group{ result});
            }

            {
               //
               // We need to make sure the gateway have dependencies to all user groups. We could
               // order the groups and pick the last one, but it's more semantic correct to make have dependencies
               // to all, since that is exactly what we're trying to represent.
               //
               manager::state::Group gateway{ ".casual.gateway", {}};
               result.group_id.gateway = gateway.id;

               for( auto& group : result.groups)
               {
                  gateway.dependencies.push_back( group.id);
               }
               result.groups.push_back( std::move( gateway));
            }


            //
            // Handle executables
            //
            {

               //
               // Add our self to processes that this domain has. Mostly to
               // make it symmetric
               //
               {

                  result.processes[ common::process::handle().pid] = common::process::handle();

                  manager::state::Executable manager;
                  result.manager_id = manager.id;
                  manager.alias = "casual-domain-manager";
                  manager.path = "casual-domain-manager";
                  manager.configured_instances = 1;
                  manager.memberships.push_back( result.group_id.master);
                  manager.note = "responsible for all executables in this domain";

                  manager.instances.push_back( common::process::id());

                  result.executables.push_back( std::move( manager));
               }

               range::append( local::executables( domain.servers, result.groups), result.executables);
               range::append( local::executables( domain.executables, result.groups), result.executables);

               range::for_each( result.executables, local::verify::Alias{});

            }


            return result;
         }

      } // transform
   } // domain
} // casual
