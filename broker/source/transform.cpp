//!
//! transform.cpp
//!
//! Created on: Sep 13, 2014
//!     Author: Lazan
//!


#include "broker/transform.h"
#include "broker/filter.h"

#include "common/environment.h"
#include "common/chronology.h"

namespace casual
{

   namespace broker
   {
      namespace transform
      {

         namespace configuration
         {
            namespace local
            {
               namespace
               {
                  std::vector< state::Group::id_type> membership( const std::vector< std::string>& members, const std::vector< state::Group>& groups)
                  {
                     std::vector< state::Group::id_type> result;

                     for( auto& name : members)
                     {
                        auto dependant = common::range::find_if( groups, filter::group::Name{ name});

                        if( dependant)
                        {
                           result.push_back( dependant->id);
                        }
                        else
                        {
                           throw state::exception::Missing{ "unresolved dependency to group '" + name + "'" };
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

                  template< typename R, typename V>
                  void executable( R& result, const V& value, const std::vector< state::Group>& groups)
                  {
                     result.alias = value.alias;
                     result.arguments = value.arguments;
                     result.configured_instances = std::stoul( value.instances);
                     result.note = value.note;
                     result.path = value.path;
                     result.restart = value.restart == "true";

                     result.environment.variables = local::environment( value.environment);

                     result.memberships = local::membership( value.memberships, groups);
                  }


               } // <unnamed>
            } // local

            state::Service Service::operator () ( const config::domain::Service& service) const
            {
               state::Service result;

               result.information.name = service.name;
               result.information.timeout = common::chronology::from::string( service.timeout);

               return result;
            }

            state::Executable Executable::operator () ( const config::domain::Executable& executable) const
            {
               state::Executable result;

               local::executable( result, executable, m_groups);

               return result;
            }

            state::Server Server::operator () ( const config::domain::Server& server) const
            {
               state::Server result;

               local::executable( result, server, m_groups);

               result.restrictions = server.restriction;


               return result;
            }


            state::Group::Resource Resource::operator () ( const config::domain::Resource& resource) const
            {
               state::Group::Resource result;

               result.key = resource.key;
               result.openinfo = resource.openinfo;
               result.closeinfo = resource.closeinfo;
               result.instances = std::stoul( resource.instances);

               return result;
            }


            state::Group Group::operator () ( const config::domain::Group& group) const
            {
               state::Group result;

               result.name = group.name;
               result.note = group.note;

               common::range::transform( group.resources, result.resources, Resource{});

               return result;
            }

            namespace local
            {
               namespace
               {
                  namespace add
                  {
                     struct Groupdendency
                     {
                        Groupdendency( state::Group::id_type id) : m_id( id) {}

                        void operator () ( state::Group& group) const
                        {
                           group.dependencies.push_back( m_id);
                        }
                        state::Group::id_type m_id;
                     };
                  } // add

                  inline std::vector< state::Group> groups( const std::vector< config::domain::Group>& groups, state::Group casual_group)
                  {
                     std::vector< state::Group> result;

                     common::range::transform( groups, result, Group{});

                     //
                     // Resolve dependencies
                     //
                     for( auto& group : groups)
                     {
                        auto found = common::range::find_if( result, filter::group::Name{ group.name});

                        if( found)
                        {
                           found->dependencies = local::membership( group.dependencies, result);
                        }
                        else
                        {
                           // TODO: could not happen?
                        }
                     }

                     //
                     // Transform and add master casual group
                     //
                     {
                        common::range::for_each( result, add::Groupdendency{ casual_group.id});

                        result.push_back( std::move( casual_group));
                     }

                     common::range::sort( result);

                     return result;
                  }



               } // <unnamed>
            } // local

            broker::State Domain::operator () ( const config::domain::Domain& domain) const
            {
               broker::State result;

               //
               // Handle groups
               //
               {
                  state::Group casual_group;
                  casual_group.name = "casual-group";
                  casual_group.note = "group for casual stuff";

                  result.casual_group_id = casual_group.id;

                  result.groups = local::groups( domain.groups, std::move( casual_group));
               }

               //
               // Make sure we "configure" the forward-cache
               //
               {
                  state::Server forward;
                  forward.alias = "casual-forward-cache";
                  forward.path = "casual-forward-cache";
                  forward.configured_instances = 1;
                  forward.memberships.push_back( result.casual_group_id);
                  forward.note = "TODO...";

                  result.add( std::move( forward));
               }

               //
               // Handle TM
               //
               {
                  auto tm = transaction::Manager{}( domain.transactionmanager);
                  tm.memberships.push_back( result.casual_group_id);

                  result.add( std::move( tm));
               }


               //
               // Servers
               //
               for( auto& s : domain.servers)
               {
                  result.add( Server{ result.groups}( s));
               }

               //
               // Executables
               //
               for( auto& e : domain.executables)
               {
                  result.add( Executable{ result.groups}( e));
               }

               //
               // Services
               //
               for( auto& s : domain.services)
               {
                  result.add( Service{}( s));
               }

               result.standard.service = Service{}( domain.casual_default.service);
               result.standard.environment = local::environment( domain.casual_default.environment);

               return result;
            }



            namespace transaction
            {
               state::Server Manager::operator () ( const config::domain::transaction::Manager& manager) const
               {
                  state::Server result;

                  result.alias = "casual-transaction-manager";
                  result.path = manager.path;
                  result.configured_instances = 1;
                  result.arguments = { "--database", manager.database};
                  result.note = "the one and only transaction manager in this domain";


                  return result;
               }

            } // transaction

         } // configuration



         state::Service Service::operator () ( const common::message::Service& value) const
         {
            state::Service result;

            result.information.name = value.name;
            //result.information.timeout = value.timeout;
            result.information.transaction = value.transaction;
            result.information.type = value.type;

            // TODD: set against configuration


            return result;
         }

         state::Server::Instance Instance::operator () ( const common::message::server::connect::Request& message) const
         {
            state::Server::Instance result;

            //result->path = message.path;
            result.process = message.process;

            return result;
         }


         common::process::Handle Instance::operator () ( const state::Server::Instance& value) const
         {
            return value.process;
         }

         namespace transaction
         {
            common::message::transaction::resource::Manager Resource::operator () ( const state::Group::Resource& resource) const
            {
               common::message::transaction::resource::Manager result;

               result.instances = resource.instances;
               result.id = resource.id;
               result.key = resource.key;
               result.openinfo = resource.openinfo;
               result.closeinfo = resource.closeinfo;

               return result;
            }


            common::message::transaction::manager::Configuration configuration( const broker::State& state)
            {
               common::message::transaction::manager::Configuration result;

               result.domain = common::environment::domain::name();

               for( auto& group : state.groups)
               {
                  common::range::transform( group.resources, result.resources, Resource{});
               }

               return result;
            }


            namespace client
            {

               common::message::transaction::client::connect::Reply reply( broker::State& state, const state::Server::Instance& instance)
               {
                  common::message::transaction::client::connect::Reply reply;

                  reply.domain = common::environment::domain::name();

                  try
                  {
                     auto& server = state.getServer( instance.server);

                     for( auto& groupId : server.memberships)
                     {
                        auto& group = state.getGroup( groupId);

                        common::range::transform(
                           group.resources,
                           reply.resources,
                           transaction::Resource{});
                     }

                  }
                  catch( const state::exception::Missing&)
                  {
                     // Do nothing. We send empty reply
                  }

                  return reply;
               }

            } // client
         } // transaction

      } // transform
   } // broker

} // casual


