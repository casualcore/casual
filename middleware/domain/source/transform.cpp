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


            } // <unnamed>
         } // local

         manager::admin::vo::State state( const manager::State& state)
         {
            return {};
         }


         manager::State state( const config::domain::Domain& domain)
         {
            manager::State result;

            //
            // Handle groups
            //
            {
               manager::state::Group global;
               global.name = "global";
               global.note = "global group - the group that everything has dependency to";

               //result.casual_group_id = casual_group.id;

               result.groups = local::groups( domain.groups, std::move( global));
            }

            //
            // Handle executables
            //
            {
               result.executables = local::executables( domain.servers, result.groups);

               range::append( local::executables( domain.executables, result.groups), result.executables);

            }


            return result;
         }

      } // transform

   } // domain


} // casual
