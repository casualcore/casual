//!
//! casual 
//!

#include "domain/transform.h"


#include "config/domain.h"


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

               struct Resources
               {
                  manager::state::Group::Resource operator () ( const config::domain::Resource& value) const
                  {
                     manager::state::Group::Resource result;

                     result.key = value.key;
                     result.instances = std::stoul( coalesce( value.instances, "0"));
                     result.openinfo = value.openinfo;
                     result.closeinfo = value.closeinfo;

                     return result;
                  }
               };

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
                  manager::state::Group operator () ( const config::domain::Group& group) const
                  {
                     manager::state::Group result;

                     result.name = group.name;
                     result.note = group.note;
                     range::transform( group.resources, result.resources, local::Resources{});


                     return result;
                  }
               };

               std::vector< manager::state::Group> groups( const std::vector< config::domain::Group>& groups, manager::state::Group&& global)
               {
                  std::vector< manager::state::Group> result;
                  result.push_back( std::move( global));

                  range::transform( groups, result, local::Group{});

                  //
                  // Resolve dependencies
                  //
                  for( auto& group : groups)
                  {
                     auto found = common::range::find( result, group.name);

                     if( found)
                     {
                        found->dependencies = local::membership( group.dependencies, result);

                        //
                        // All groups have is member of the global group
                        //
                        found->dependencies.push_back( result.front().id);

                     }
                     else
                     {
                        throw exception::invalid::Semantic{ "could not happen!"};
                     }
                  }

                  return result;
               }

               std::vector< std::string> environment( const config::Environment& environment)
               {
                  std::vector< std::string> result;
                  for( auto& variable : config::environment::fetch( environment))
                  {
                     result.push_back( variable.key + "=" + variable.value);
                  }
                  return result;
               }


               struct Executable
               {
                  template< typename V>
                  manager::state::Executable operator() ( V&& value, const std::vector< manager::state::Group>& groups)
                  {
                     manager::state::Executable result;

                     result.alias = value.alias;
                     result.arguments = value.arguments;
                     result.configured_instances = std::stoul( coalesce( value.instances, "0"));
                     result.note = value.note;
                     result.path = value.path;
                     result.restart = value.restart == "true";

                     result.environment.variables = local::environment( value.environment);

                     result.memberships = local::membership( value.memberships, groups);

                     if( result.memberships.empty())
                     {
                        result.memberships.push_back( groups.at( 0).id);
                     }


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
                  struct Resource
                  {
                     manager::admin::vo::Group::Resource operator () ( const manager::state::Group::Resource& value)
                     {
                        manager::admin::vo::Group::Resource result;

                        result.key = value.key;
                        result.openinfo = value.openinfo;
                        result.closeinfo = value.closeinfo;
                        result.instances = value.instances;

                        return result;
                     }
                  };

                  struct Group
                  {
                     manager::admin::vo::Group operator () ( const manager::state::Group& value)
                     {
                        manager::admin::vo::Group result;

                        result.id = value.id;
                        result.name = value.name;
                        result.note = value.note;

                        result.dependencies = value.dependencies;
                        result.resources = range::transform( value.resources, vo::Resource{});

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


         manager::State state( const config::domain::Domain& domain)
         {
            manager::State result;

            result.configuration = domain;


            //
            // Handle groups
            //
            {
               manager::state::Group global;
               global.name = "global";
               global.note = "global group - the group that everything has dependency to";

               result.groups = local::groups( domain.groups, std::move( global));
            }

            //
            // Handle executables
            //
            {
               result.executables = local::executables( domain.servers, result.groups);

               range::append( local::executables( domain.executables, result.groups), result.executables);

               range::for_each( result.executables, local::verify::Alias{});

            }


            return result;
         }

         namespace configuration
         {
            namespace transaction
            {
               common::message::domain::configuration::transaction::Resource resource( const manager::state::Group::Resource& value)
               {
                  common::message::domain::configuration::transaction::Resource result;

                  result.id = value.id;
                  result.key = value.key;
                  result.openinfo = value.openinfo;
                  result.closeinfo = value.closeinfo;
                  result.instances = value.instances;

                  return result;
               }
            } // transaction




         } // configuration

      } // transform

   } // domain


} // casual
