//!
//! transform.cpp
//!
//! Created on: Sep 13, 2014
//!     Author: Lazan
//!


#include "broker/transform.h"
#include "broker/filter.h"

#include "common/environment.h"

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

                  template< typename R, typename V>
                  void executable( R& result, const V& value, const std::vector< state::Group>& groups)
                  {
                     result.alias = value.alias;
                     result.arguments = value.arguments;
                     result.configuredInstances = std::stoul( value.instances);
                     result.note = value.note;
                     result.path = value.path;

                     result.memberships = local::membership( value.memberships, groups);
                  }


               } // <unnamed>
            } // local

            state::Service Service::operator () ( const config::domain::Service& service) const
            {
               state::Service result;

               result.information.name = service.name;
               result.information.timeout = std::stoul( service.timeout);

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

               if( ! group.resource.key.empty())
               {
                  result.resource.push_back( Resource{}( group.resource));
               }


               return result;
            }

            namespace local
            {
               namespace
               {
                  inline std::vector< state::Group> groups( const std::vector< config::domain::Group>& groups)
                  {
                     std::vector< state::Group> result;

                     common::range::transform( groups, result, Group{});

                     //
                     // Now we got to resolve dependencies
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

                     return result;
                  }



               } // <unnamed>
            } // local

            broker::State Domain::operator () ( const config::domain::Domain& domain) const
            {
               broker::State result;

               result.groups = local::groups( domain.groups);

               //
               // Servers
               //
               for( auto& s : domain.servers)
               {
                  auto server = Server{ result.groups}( s);
                  result.servers.emplace( server.id, std::move( server));
               }

               //
               // Executables
               //
               for( auto& e : domain.executables)
               {
                  auto executable = Executable{ result.groups}( e);
                  result.executables.emplace( executable.id, std::move( executable));
               }

               //
               // Services
               //
               for( auto& s : domain.services)
               {
                  auto service = Service{}( s);
                  result.services.emplace( service.information.name, std::move( service));
               }

               result.standard.service = Service{}( domain.casual_default.service);

               return result;
            }

         } // configuration


         state::Service Service::operator () ( const common::message::Service& value) const
         {
            state::Service result;

            result.information.name = value.name;

            // TODD: set against configuration


            return result;
         }

         state::Server::Instance Instance::operator () ( const common::message::server::connect::Request& message) const
         {
            state::Server::Instance result;

            //result->path = message.path;
            result.pid = message.server.pid;
            result.queue_id = message.server.queue_id;

            return result;
         }


         common::message::server::Id Instance::operator () ( const state::Server::Instance& value) const
         {
            casual::common::message::server::Id result;

            result.pid = value.pid;
            result.queue_id = value.queue_id;

            return result;
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


            common::message::transaction::Configuration configuration( const broker::State& state)
            {
               common::message::transaction::Configuration result;

               result.domain = common::environment::domain::name();

               for( auto& group : state.groups)
               {
                  common::range::transform( group.resource, result.resources, Resource{});
               }

               return result;
            }


            namespace client
            {

               common::message::transaction::client::connect::Reply reply( broker::State& state, const state::Server::Instance& instance)
               {
                  common::message::transaction::client::connect::Reply reply;

                  reply.domain = common::environment::domain::name();
                  reply.transactionManagerQueue = state.transactionManagerQueue;

                  try
                  {
                     auto& server = state.getServer( instance.server);

                     for( auto& groupId : server.memberships)
                     {
                        auto& group = state.getGroup( groupId);

                        common::range::transform( group.resource,
                           reply.resourceManagers,
                           transaction::Resource());
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


