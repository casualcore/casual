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

#include "common/internal/log.h"
#include "common/internal/trace.h"
#include "common/queue.h"
#include "common/signal.h"
#include "common/algorithm.h"
#include "common/exception.h"

#include "common/message/transaction.h"
#include "common/message/dispatch.h"


#include "sf/log.h"


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


         namespace find
         {

            namespace group
            {
               struct Name
               {
                  Name( std::string name) : name( std::move( name)) {}

                  bool operator () ( const Group& group) const
                  {
                     return group.name == name;
                  }

               private:
                  std::string name;
               };


               struct Id
               {
                  Id( Group::id_type id) : id( std::move( id)) {}

                  bool operator () ( const Group& group) const
                  {
                     return group.id == id;
                  }

               private:
                  Group::id_type id;

               };

            } // group
            struct Instance
            {
               Instance( broker::Server::pid_type pid) : m_pid( pid) {}

               bool operator () ( const broker::Server::Instance& instance) const
               {
                  return instance.pid == m_pid;
               }
            private:
               broker::Server::pid_type m_pid;
            };



            struct Service
            {
               Service( std::string name) : m_name( std::move( name)) {}

               bool operator () ( const broker::Service& service) const
               {
                  return service.information.name == m_name;
               }

            private:
               std::string m_name;
            };

            template< Server::Instance::State state>
            struct basic_instance_state
            {
               bool operator () ( const Server::Instance& instance) const
               {
                  return instance.state == state;
               }
            };

            namespace idle
            {
               using Instance = basic_instance_state< Server::Instance::State::idle>;

            } // idle

            namespace busy
            {
               using Instance = basic_instance_state< Server::Instance::State::busy>;

            } // idle


            struct Pending
            {
               Pending( const broker::Server::Instance& instance) : m_instance( instance) {}

               bool operator () ( const common::message::service::name::lookup::Request& request)
               {
                  for( auto& service : m_instance.services)
                  {
                     if( service.get().information.name == request.requested)
                     {
                        return true;
                     }
                  }
                  return false;
               }
               const broker::Server::Instance& m_instance;
            };

            struct Running
            {
               bool operator () ( const Server::Instance& instance) const
               {
                  switch( instance.state)
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
               bool operator () ( const Server::Instance& instance) const
               {
                  return instance.state != Server::Instance::State::prospect;
               }
            };

         } // find

         namespace transform
         {

            struct Group
            {
               broker::Group operator() ( const config::domain::Group& group) const;

            };

            struct Instance
            {
               broker::Server::Instance operator () ( const common::message::server::connect::Request& message) const
               {
                  broker::Server::Instance result;

                  //result->path = message.path;
                  result.pid = message.server.pid;
                  result.queue_id = message.server.queue_id;

                  return result;
               }

               common::message::server::Id operator () ( const broker::Server::Instance& value) const
               {
                  casual::common::message::server::Id result;

                  result.pid = value.pid;
                  result.queue_id = value.queue_id;

                  return result;
               }
            };

            struct Server : Base
            {
               using Base::Base;

               broker::Server operator () ( const config::domain::Server& server) const
               {
                  broker::Server result;

                  result.alias = server.alias;
                  result.path = server.path;
                  result.arguments = server.arguments;
                  result.note = server.note;
                  result.restrictions = server.restriction;
                  common::range::sort( result.restrictions);

                  for( auto& group : server.memberships)
                  {
                     auto found = common::range::find_if( m_state.groups, find::group::Name{ group});
                     if( found)
                     {
                        result.memberships.push_back( found.first->id);
                     }
                     else
                     {
                        // TODO: error?
                     }
                  }

                  if( ! server.instances.empty())
                  {
                     result.configuredInstances = std::stoul( server.instances);
                  }

                  //m_state.servers.emplace( result->alias, result);

                  return result;
               }
            };

            namespace transaction
            {
               inline broker::Server manager( const config::domain::transaction::Manager& manager)
               {
                  broker::Server result;

                  result.alias = "casual-transaction-manager";
                  result.arguments = { "-db", manager.database};
                  result.path = manager.path;
                  result.note = "the one and only transaction manager for this domain";

                  return result;
               }

               struct Resource
               {

                  inline common::message::transaction::resource::Manager operator () ( const state::Group::Resource& resource) const
                  {
                     common::message::transaction::resource::Manager result;

                     result.instances = resource.instances;
                     result.id = resource.id;
                     result.key = resource.key;
                     result.openinfo = resource.openinfo;
                     result.closeinfo = resource.closeinfo;

                     return result;

                  }
               };


               inline common::message::transaction::Configuration configuration( const std::vector< state::Group>& groups)
               {
                  common::message::transaction::Configuration result;

                  result.domain = common::environment::domain::name();

                  for( auto& group : groups)
                  {
                     common::range::transform( group.resource, result.resources, Resource{});
                  }

                  return result;
               }


               namespace client
               {

                  inline common::message::transaction::client::connect::Reply reply( const broker::Server::Instance& instance, State& state)
                  {
                     common::message::transaction::client::connect::Reply reply;

                     reply.domain = common::environment::domain::name();
                     reply.transactionManagerQueue = state.transactionManagerQueue;

                     auto server = common::range::find( state.servers, instance.server);

                     if( server)
                     {
                        for( auto& group : server->second.memberships)
                        {
                           auto found = common::range::find_if( state.groups, find::group::Id{ group});

                           if( found)
                           {
                              common::range::transform( found->resource,
                                 reply.resourceManagers,
                                 transaction::Resource());
                           }
                        }
                     }

                     return reply;
                  }

               } // client

            } // transaction

         } // transform


         template< common::message::Type type>
         void connect( broker::Server::Instance& instance, const common::message::server::basic_connect< type>& message)
         {
            instance.queue_id = message.server.queue_id;
            instance.alterState( broker::Server::Instance::State::idle);

            //if( instance->server)
            {
               //common::log::internal::debug << instance->server->path << " - ";
            }

            common::log::internal::debug << "pid: " << instance.pid <<  " queue: " << instance.queue_id << " is up and running" << std::endl;

         }


         //std::vector< std::vector< std::shared_ptr< broker::Group> > > bootOrder( State& state);





         namespace add
         {
            struct Service : Base
            {
               Service( broker::State& state, broker::Server::Instance& instance)
                  : Base( state), m_instance( instance)
               {
                  auto found = common::range::find( state.servers, m_instance.server);

                  if( found)
                  {
                     m_server = &(found->second);
                  }
               }

               void operator () ( const common::message::Service& service) const
               {
                  //
                  // Check if there are any restrictions
                  //
                  if( m_server && ! m_server->restrictions.empty())
                  {
                     if( ! common::range::sorted::search( m_server->restrictions, service.name))
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

                  auto found = common::range::find( m_state.services, service.name);

                  if( ! found)
                  {
                     found.first = m_state.services.emplace(
                           service.name, std::make_shared< broker::Service>( service.name)).first;
                     found.last = found.first + 1;
                  }

                  found->second.instances.push_back( m_instance);
                  m_instance.services.push_back( found->second);
               }

            private:
               broker::Server::Instance& m_instance;
               broker::Server* m_server = nullptr;
            };
         } // add






         namespace remove
         {

            namespace internal
            {
               template< typename R, typename I>
               inline void instance( R& instances, const I& instance)
               {
                  auto found = common::range::find( instances, instance);
                  if( found)
                  {
                     instances.erase( found.first);
                  }
               }

               template< typename R, typename S>
               inline void service( R& services, const S& service)
               {
                  auto found = common::range::find( services, service);
                  if( found)
                  {
                     services.erase( found.first);
                  }
               }
            } // internal

            struct Service : Base
            {
               Service( broker::State& state, broker::Server::Instance& instance)
                  : Base( state), m_instance( instance) {}

               void operator () ( broker::Service& service) const
               {
                  internal::instance( service.instances, m_instance);
                  internal::service( m_instance.services, service);
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

                  auto found = common::range::find( m_state.services, service.name);

                  if( found)
                  {
                     operator() ( found->second);
                  }
                  else
                  {
                     common::log::error << "service " << service.name << " does not exist, hence can't be removed - instance: pid: " << m_instance.pid << std::endl;
                  }

               }
            private:
               broker::Server::Instance& m_instance;

            };

            inline void instance( common::platform::pid_type pid, broker::State& state)
            {
               auto found = common::range::find( state.instances, pid);

               if( found)
               {
                  auto& instance = found->second;

                  for( auto& service : instance.services)
                  {
                     auto findService = state.services.find( service.get().information.name);
                     if( findService != std::end( state.services))
                     {
                        remove::internal::instance( findService->second.instances, instance);
                     }
                  }

                  auto server = common::range::find( state.servers, instance.server);

                  if( server)
                  {
                     remove::internal::instance( server->second.instances, instance);
                  }

                  state.instances.erase( found.first);
               }
               else
               {
                  common::log::error << "failed to remove instance - pid '" << pid << " does not exist - action: discard" << std::endl;
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
                     return value.memberships.empty();
                  }
               };
            } // empty

            struct Membership
            {

               Membership( const broker::Group& group) : m_id( group.id) {}

               template< typename T>
               bool operator () ( T&& value) const
               {
                  auto result = common::range::find( value.memberships, m_id);
                  return ! result.empty();
               }
            private:
               broker::Group::id_type m_id;
            };

         } // filter

         namespace boot
         {


            void instance( State& state, const std::shared_ptr< broker::Server>& server);

            namespace transaction
            {
               inline void manager( State& state, const config::domain::transaction::Manager& manager)
               {
                  {
                     auto tm = transform::transaction::manager( manager);

                     auto inserted = state.servers.emplace( tm.id, std::move( tm));

                     state.transactionManager = &inserted.first->second;
                  }

                  //
                  // Boot the instance...
                  //
                  state.instance( *state.transactionManager, 1);
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
                  m_state.instance( server, server.configuredInstances);
               }

            };

            template< typename S>
            void servers( State& state, S&& servers)
            {
               common::log::internal::debug << "boot " << servers.size() << " servers" << std::endl;

               common::range::for_each( servers, boot::Server{ state});

               //
               // Wait for the batch boot
               //
               common::trace::internal::Scope trace( "batch boot wait");

               broker::QueueBlockingReader queueReader{ common::ipc::receive::queue(), state};

               common::message::dispatch::Handler handler{
                  handle::Connect{ state},
                  handle::transaction::client::Connect{ state},
               };

               auto connectedServer = [&]( const broker::Server& value)
                     {
                        for( auto&& instance : value.instances)
                        {
                           auto found = common::range::find( state.instances, instance);

                           if( ! found || ! find::Booted{}( *found))
                           {
                              return false;
                           }
                        }
                        return true;
                     };

               while( ! common::range::all_of( servers, connectedServer))
               {
                  try
                  {
                     common::signal::alarm::Scoped timeout{ 5};

                     auto marshal = queueReader.next();
                     handler.dispatch( marshal);
                  }
                  catch( const common::exception::signal::Timeout& exception)
                  {
                     common::log::error << "failed to get response from spawned instances in a timely manner - action: abort" << std::endl;
                     throw common::exception::signal::Terminate{};
                  }
               }
            }


            void domain( State& state, const config::domain::Domain& domain)
            {
               add::groups( state, domain.groups);

               //
               // Boot the transaction manager first
               //
               try
               {
                  common::signal::alarm::Scoped timeout{ 10};

                  boot::transaction::manager( state, domain.transactionmanager);

                  common::log::internal::transaction << "wait for transaction monitor connect";

                  handle::transaction::ManagerConnect tmConnect( state);

                  broker::QueueBlockingReader queueReader{ common::ipc::receive::queue(), state};

                  typename handle::transaction::ManagerConnect::message_type connect;
                  queueReader( connect);

                  tmConnect.dispatch( connect);

                  //
                  // wait for ack
                  //
                  common::message::transaction::Connected connected;
                  queueReader( connected);

                  if( ! connected.success)
                  {
                     //
                     // Abort boot
                     //
                     throw common::exception::NotReallySureWhatToNameThisException{};
                  }

               }
               catch( const common::exception::signal::Timeout& exception)
               {
                  common::log::error << "failed to get response from transaction manager in a timely manner - action: abort" << std::endl;
                  throw common::exception::signal::Terminate{};

               }

               // TODO: Handle executables...

               //
               // Transform servers
               //
               std::vector< broker::Server> servers;

               common::range::transform( domain.servers, servers, transform::Server{ state});


               auto serverRange = common::range::make( servers);

               auto groups = state.groups;
               common::range::stable_sort( groups);


               for( auto& group : groups)
               {
                  common::log::internal::debug << "boot group: " << group.name << std::endl;

                  auto bootRange = common::range::stable_partition(
                        serverRange, filter::Membership{ group});

                  boot::servers( state, bootRange);

                  serverRange = serverRange - bootRange;
               }

               //
               // Boot the ones that has no memberships, ie the ones left...
               //
               boot::servers( state, serverRange);



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
