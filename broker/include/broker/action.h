//!
//! action.h
//!
//! Created on: May 12, 2013
//!     Author: Lazan
//!

#ifndef ACTION_H_
#define ACTION_H_

#include "config/domain.h"
#include "broker/broker.h"

#include "common/platform.h"
#include "common/string.h"
#include "common/logger.h"
#include "common/trace.h"
#include "common/queue.h"
#include "common/signal.h"


//TODO: temp
#include <iostream>

namespace casual
{
   namespace broker
   {
      namespace action
      {
         struct Base
         {
            Base( broker::State& state) : m_state( state) {}

            broker::State& m_state;
         };

         namespace transform
         {
            struct Instance
            {
               std::shared_ptr< broker::Server::Instance> operator () ( const common::message::server::Connect& message) const
               {
                  auto result = std::make_shared< broker::Server::Instance>();

                  //result->path = message.path;
                  result->pid = message.server.pid;
                  result->queue_id = message.server.queue_id;

                  return result;
               }

               common::message::server::Id operator () ( const std::shared_ptr< broker::Server::Instance>& value) const
               {
                  casual::common::message::server::Id result;

                  result.pid = value->pid;
                  result.queue_id = value->queue_id;

                  return result;
               }
            };

            struct Server : Base
            {
               using Base::Base;

               std::shared_ptr< broker::Server> operator () ( const config::domain::Server& server) const
               {
                  auto result = std::make_shared< broker::Server>();

                  result->alias = server.alias;
                  result->path = server.path;
                  result->arguments = server.arguments;
                  result->note = server.note;
                  result->restrictions = server.restriction;
                  std::sort( std::begin( result->restrictions), std::end( result->restrictions));

                  for( auto& group : server.memberships)
                  {
                     result->memberships.push_back( m_state.groups.at( group));
                  }

                  if( ! server.instances.empty())
                  {
                     result->configuredInstances = std::stoul( server.instances);
                  }

                  m_state.servers.emplace( result->alias, result);

                  return result;
               }
            };

            namespace transaction
            {
               inline std::shared_ptr< broker::Server> manager( const config::domain::transaction::Manager& manager)
               {
                  auto result = std::make_shared< broker::Server>();

                  result->alias = "casual-transaction-manager";
                  result->arguments = { "-db", manager.database};
                  result->path = manager.path;
                  result->note = "the one and only transaction manager for this domain";

                  return result;
               }

               struct Resource
               {

                  inline common::message::resource::Manager operator () ( const broker::Group::Resource& resource) const
                  {
                     common::message::resource::Manager result;

                     result.instances = resource.instances;
                     result.id = resource.id;
                     result.key = resource.key;
                     result.openinfo = resource.openinfo;
                     result.closeinfo = resource.closeinfo;

                     return result;

                  }
               };

               inline common::message::transaction::Configuration configuration( const State::group_mapping_type& groups)
               {
                  common::message::transaction::Configuration result;

                  for( auto& group : groups)
                  {
                     std::transform(
                        std::begin( group.second->resource),
                        std::end( group.second->resource),
                        std::back_inserter( result.resources),
                        Resource{});
                  }

                  return result;
               }

            } // transaction

            inline common::message::server::Configuration configuration( const std::shared_ptr< broker::Server::Instance>& instance, State& state)
            {
               common::message::server::Configuration result;

               result.transactionManagerQueue = state.transactionManagerQueue;

               if( instance->server)
               {
                  for( auto& group : instance->server->memberships)
                  {
                     std::transform(
                        std::begin( group->resource),
                        std::end( group->resource),
                        std::back_inserter( result.resourceManagers),
                        transaction::Resource());
                  }
               }
               return result;
            }

         } // transform


         template< common::message::Type type>
         void connect( std::shared_ptr< broker::Server::Instance>& instance, const common::message::server::basic_connect< type>& message)
         {
            instance->queue_id = message.server.queue_id;
            instance->alterState( broker::Server::Instance::State::idle);
         }


         std::vector< std::vector< std::shared_ptr< broker::Group> > > bootOrder( State& state);





         namespace add
         {
            struct Service : Base
            {
               Service( broker::State& state, std::shared_ptr< broker::Server::Instance>& instance)
                  : Base( state), m_instance( instance) {}

               void operator () ( const common::message::Service& service) const
               {
                  //
                  // Check if there are any restrictions
                  //
                  if( m_instance->server && ! m_instance->server->restrictions.empty())
                  {
                     if( ! std::binary_search(
                           std::begin( m_instance->server->restrictions),
                           std::end( m_instance->server->restrictions),
                           service.name))
                     {
                        //
                        // Service is not allowed for this server (and this instance of it)
                        //
                        return;
                     }
                  }

                  //
                  // We have to do the following
                  // - try to find the service
                  //   - if not, add it
                  // - add instance to service
                  // - add service to instance
                  //

                  auto serviceIter = m_state.services.find( service.name);

                  if( serviceIter == std::end( m_state.services))
                  {
                     serviceIter = m_state.services.emplace(
                           service.name, std::make_shared< broker::Service>( service.name)).first;
                  }

                  serviceIter->second->instances.push_back( m_instance);
                  m_instance->services.push_back( serviceIter->second);
               }

            private:
               std::shared_ptr< broker::Server::Instance>& m_instance;
            };
         } // add

         namespace find
         {
            struct Instance
            {
               Instance( broker::Server::pid_type pid) : m_pid( pid) {}

               bool operator () ( const std::shared_ptr< broker::Server::Instance>& server) const
               {
                  return server->pid == m_pid;
               }
            private:
               broker::Server::pid_type m_pid;
            };

            struct Service
            {
               Service( const std::string& name) : m_name( name) {}

               bool operator () ( const std::shared_ptr< broker::Service>& service) const
               {
                  return service->information.name == m_name;
               }

            private:
               std::string m_name;
            };

            template< Server::Instance::State state>
            struct basic_instance_state
            {
               bool operator () ( const std::shared_ptr< Server::Instance>& server) const
               {
                  return server->state == state;
               }
            };

            namespace idle
            {
               using Instance = basic_instance_state< Server::Instance::State::idle>;

            } // idle

            namespace busy
            {
               using Instance = basic_instance_state< Server::Instance::State::idle>;

            } // idle


            struct Pending
            {
               Pending( const std::shared_ptr< broker::Server::Instance>& instance) : m_instance( instance) {}

               bool operator () ( const common::message::service::name::lookup::Request& request)
               {
                  for( auto& service : m_instance->services)
                  {
                     if( service->information.name == request.requested)
                     {
                        return true;
                     }
                  }
                  return false;
               }
               std::shared_ptr< broker::Server::Instance> m_instance;
            };

            struct Running
            {
               bool operator () ( const std::shared_ptr< Server::Instance>& server) const
               {
                  switch( server->state)
                  {
                     case Server::Instance::State::idle:
                     case Server::Instance::State::busy:
                        return true;
                     default:
                        return false;
                  }
               }
            };

            struct Booted
            {
               bool operator () ( const std::shared_ptr< Server::Instance>& server) const
               {
                  return server->state != Server::Instance::State::prospect;
               }
            };

         } // find




         namespace remove
         {

            namespace internal
            {
               inline void instance( std::vector< std::shared_ptr< broker::Server::Instance>>& instances, const std::shared_ptr< broker::Server::Instance>& instance)
               {
                  auto findIter = std::find( std::begin( instances), std::end( instances), instance);
                  if( findIter != std::end( instances))
                  {
                     instances.erase( findIter);
                  }
               }

               inline void service( std::vector< std::shared_ptr< broker::Service>>& services, const std::shared_ptr< broker::Service>& service)
               {
                  auto findIter = std::find( std::begin( services), std::end( services), service);
                  if( findIter != std::end( services))
                  {
                     services.erase( findIter);
                  }
               }
            } // internal

            struct Service : Base
            {
               Service( broker::State& state, std::shared_ptr< broker::Server::Instance>& instance)
                  : Base( state), m_instance( instance) {}

               void operator () ( const std::shared_ptr< broker::Service>& service) const
               {
                  internal::instance( service->instances, m_instance);
                  internal::service( m_instance->services, service);
               }

               void operator () ( const common::message::Service& service) const
               {
                  //
                  // We have to do the following
                  // - try to find the service
                  //   - if not, log it
                  // - remove instance from service
                  // - remove service from instance
                  //

                  auto serviceIter = m_state.services.find( service.name);

                  if( serviceIter != std::end( m_state.services))
                  {
                     operator() ( serviceIter->second);
                  }
                  else
                  {
                     common::logger::error << "service " << service.name << " does not exist, hence can't be removed - instance: pid: " << m_instance->pid;
                  }

               }
            private:
               std::shared_ptr< broker::Server::Instance>& m_instance;

            };

            inline void instance( common::platform::pid_type pid, broker::State& state)
            {
               auto findInstance = state.instances.find( pid);

               if( findInstance != std::end( state.instances))
               {
                  auto& instance = findInstance->second;

                  for( auto& service : instance->services)
                  {
                     auto findService = state.services.find( service->information.name);
                     if( findService != std::end( state.services))
                     {
                        remove::internal::instance( findService->second->instances, instance);
                     }
                  }

                  if( instance->server)
                  {
                     remove::internal::instance( instance->server->instances, instance);
                  }

                  state.instances.erase( findInstance);
               }
               else
               {
                  common::logger::error << "failed to remove instance - pid '" << pid << " does not exist - action: discard";
               }
            }

            void groups( State& state, const std::vector< std::string>& remove);

         } // remove

         namespace add
         {
            void groups( State& state, const std::vector< config::domain::Group>& groups);

         } // add

         namespace update
         {
            void groups( State& state, const std::vector< config::domain::Group>& update, const std::vector< std::string>& remove);

            void servers( State& state, const std::vector< config::domain::Server>& update, const std::vector< std::string>& remove);

            template< typename P>
            struct basic_instances : Base
            {
               using policy_type = P;

               using Base::Base;

               std::vector< std::shared_ptr< broker::Server::Instance>> operator () ( std::shared_ptr< broker::Server>& server, std::size_t instances)
               {
                  if( instances > server->instances.size())
                  {
                     return boot( server, instances - server->instances.size());
                  }
                  else
                  {
                     return shutdown( server, server->instances.size() - instances);
                  }
               }
            private:
               std::vector< std::shared_ptr< broker::Server::Instance>> boot( std::shared_ptr< broker::Server>& server, std::size_t instances)
               {
                  std::vector< std::shared_ptr< broker::Server::Instance>> result;

                  while( instances-- != 0)
                  {
                     auto pid = policy_type::boot( server->path, server->arguments);

                     auto instance = std::make_shared< Server::Instance>();
                     instance->pid = pid;
                     instance->server = server;
                     instance->alterState( Server::Instance::State::prospect);

                     server->instances.push_back( instance);

                     m_state.instances.emplace( pid, instance);
                     result.push_back( std::move( instance));
                  }

                  return result;
               }

               std::vector< std::shared_ptr< broker::Server::Instance>> shutdown( std::shared_ptr< broker::Server>& server, std::size_t instances)
               {
                  assert( server->instances.size() <= instances);

                  std::vector< std::shared_ptr< broker::Server::Instance>> result(
                        std::end( server->instances) - instances,
                        std::end( server->instances));


                  for( auto& instance : result)
                  {
                     policy_type::shutdown( *instance);

                     instance->alterState( Server::Instance::State::shutdown);
                  }

                  return result;
               }
            };

            struct Policy
            {
               static common::platform::pid_type boot( const std::string& path, const std::vector< std::string>& arguments)
               {
                  return common::process::spawn( path, arguments);
               }

               static void shutdown( broker::Server::Instance& instance)
               {
                  common::process::terminate( instance.pid);
               }
            };

            using Instances = basic_instances< Policy>;

         } // update

         namespace filter
         {
            namespace empty
            {
               struct Membership
               {
                  template< typename T>
                  bool operator () ( T& value) const
                  {
                     return value->memberships.empty();
                  }
               };
            } // empty

            struct Membership
            {
               using groups_type = std::vector< std::shared_ptr< broker::Group>>;

               Membership( const groups_type& groups) : groups( groups) {}

               template< typename T>
               bool operator () ( T& value) const
               {
                  return std::find_first_of( std::begin( value->memberships), std::end( value->memberships),
                        std::begin( groups), std::end( groups)) != std::end( value->memberships);
               }
            private:
               const groups_type& groups;
            };

         } // filter

         namespace boot
         {


            inline void instance( State& state, const std::shared_ptr< Server>& server)
            {
               auto pid = common::process::spawn( server->path, {}); // todo:, commonserver->arguments);

               auto instance = std::make_shared< Server::Instance>();
               instance->pid = pid;
               instance->server = server;
               instance->alterState( Server::Instance::State::prospect);

               server->instances.push_back( instance);

               state.instances.emplace( pid, instance);

            }

            namespace transaction
            {
               inline void manager( State& state, const config::domain::transaction::Manager& manager)
               {
                  state.transactionManager = transform::transaction::manager( manager);

                  //
                  // Boot the instance...
                  //
                  update::Instances bootInstances{ state};

                  bootInstances( state.transactionManager, 1);
               }
            } // transaction

            struct Server : Base
            {
               using Base::Base;

               template< typename T>
               void operator () ( T& server)
               {

                  //
                  // boot all the instances
                  //
                  update::Instances bootInstances{ m_state};

                  bootInstances( server, server->configuredInstances);
               }

            };


            template< typename Q, typename TMH, typename CH>
            void domain( State& state, const config::domain::Domain& domain, Q& queueReader, TMH& tmConnectHhandler, CH& connectHandler)
            {
               add::groups( state, domain.groups);

               //
               // Boot the transaction manager first
               //
               try
               {
                  common::signal::alarm::Scoped timeout{ 10};

                  boot::transaction::manager( state, domain.transactionmanager);

                  common::trace::Exit trace( "transaction monitor connect");

                  typename TMH::message_type message;
                  queueReader( message);

                  tmConnectHhandler.dispatch( message);




               }
               catch( const common::exception::signal::Timeout& exception)
               {
                  common::logger::error << "failed to get response from transaction manager in a timely manner - action: abort";
                  throw common::exception::signal::Terminate{};

               }

               // TODO: Handle executables...

               //
               // Transform servers
               //
               std::vector< std::shared_ptr< broker::Server>> servers;

               std::transform(
                  std::begin( domain.servers),
                  std::end( domain.servers),
                  std::back_inserter( servers),
                  transform::Server{ state});


               auto serversStart = std::begin( servers);

               auto bootOrder = action::bootOrder( state);


               for( auto& batch : bootOrder)
               {
                  /*
                  typedef sf::functional::Chain< sf::functional::link::Or> Chain;

                  serversEnd = std::stable_partition( serversStart, std::end( servers),
                        Chain::link(
                              filter::empty::Membership{},
                              filter::Membership{ batch}));
                   */

                  for( auto& group : batch)
                  {
                     common::logger::debug << "boot group: " << group->name;
                  }



                  auto serversEnd = std::stable_partition( serversStart, std::end( servers),
                        [&]( const std::shared_ptr< broker::Server>& server)
                        {
                           return filter::empty::Membership{}( server) || filter::Membership{ batch}( server);
                        });


                  common::logger::debug << "boot " << std::distance( serversStart, serversEnd) << " servers";


                  std::for_each(
                     serversStart,
                     serversEnd,
                     boot::Server{ state});

                  //
                  // Wait for the batch boot
                  //
                  common::trace::Exit trace( "batch boot wait");


                  typename CH::message_type message;

                  // TODO: temp
                  typedef std::shared_ptr< broker::Server> server_type;

                  auto connectedServer = []( const server_type& value)
                        {
                           return std::all_of( std::begin( value->instances), std::end( value->instances), find::Booted{});
                        };

                  while( ! std::all_of( serversStart, serversEnd, connectedServer))
                  {
                     try
                     {
                        common::signal::alarm::Scoped timeout{ 5};

                        queueReader( message);
                        connectHandler.dispatch( message);
                     }
                     catch( const common::exception::signal::Timeout& exception)
                     {
                        common::logger::error << "failed to get response from spawned instances in a timely manner - action: abort";
                        throw common::exception::signal::Terminate{};
                     }
                  }

                  serversStart = serversEnd;

               }
            }
         } // boot


         namespace shutdown
         {
            template< typename QR, typename TMH, typename CH>
            void domain( State& state, QR& queueReader, TMH& tmConnectHhandler, CH& connectHandler)
            {

            }

         } // shutdown



      } // action
   } // broker
} // casual



#endif /* ACTION_H_ */
