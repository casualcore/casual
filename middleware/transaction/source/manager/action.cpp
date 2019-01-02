//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "transaction/manager/action.h"
#include "transaction/manager/handle.h"
#include "transaction/manager/admin/transform.h"
#include "transaction/common.h"

#include "common/process.h"
#include "common/environment.h"
#include "common/server/handle/call.h"
#include "common/event/send.h"
#include "common/communication/instance.h"


#include "serviceframework/log.h"

// std
#include <string>

namespace casual
{
   using namespace common;

   namespace transaction
   {
      namespace manager
      {
         namespace action
         {
            void configure( State& state)
            {
               Trace trace( "configure");

               common::message::domain::configuration::Request request;
               request.process = process::handle();

               auto configuration = communication::ipc::call( communication::instance::outbound::domain::manager::device(), request);

               // configure state
               state::configure( state, configuration);
            }


            namespace resource
            {
               void Instances::operator () ( state::resource::Proxy& proxy)
               {
                  Trace trace( "resource::Instances::operator()");

                  log::line( log, "update instances for resource: ", proxy);

                  auto count = static_cast< long>( proxy.concurency - proxy.instances.size());

                  if( count > 0)
                  {
                     while( count-- > 0)
                     {
                        auto& info = m_state.resource_properties.at( proxy.key);

                        state::resource::Proxy::Instance instance;
                        instance.id = proxy.id;

                        try
                        {
                           instance.process.pid = process::spawn(
                                 info.server,
                                 {
                                       "--rm-key", info.key,
                                       "--rm-openinfo", proxy.openinfo,
                                       "--rm-closeinfo", proxy.closeinfo,
                                       "--rm-id", std::to_string( proxy.id.value()),
                                 }
                              );

                           instance.state( state::resource::Proxy::Instance::State::started);

                           proxy.instances.push_back( std::move( instance));
                        }
                        catch( ...)
                        {
                           common::exception::handle();
                           common::event::error::send( "failed to spawn resource-proxy-instance: " + info.server);
                        }
                     }
                  }
                  else
                  {
                     auto end = std::end( proxy.instances);

                     for( auto& instance : range::make( end + count, end))
                     {
                        switch( instance.state())
                        {
                           case state::resource::Proxy::Instance::State::absent:
                           case state::resource::Proxy::Instance::State::started:
                           {

                              log::line( log, "Instance has not register yet. We, kill it...: ", instance);

                              process::lifetime::terminate( { instance.process.pid});
                              instance.state( state::resource::Proxy::Instance::State::shutdown);
                              break;
                           }
                           case state::resource::Proxy::Instance::State::shutdown:
                           {
                              log::line( log, "instance already in shutdown state - ", instance);
                              break;
                           }
                           default:
                           {
                              log::line( log, "shutdown instance: ", instance);


                              instance.state( state::resource::Proxy::Instance::State::shutdown);

                              if( ! ipc::device().non_blocking_send( instance.process.ipc, message::shutdown::Request{}))
                              {
                                 // We couldn't send shutdown for some reason, we put the message in 'persistent-replies' and
                                 // hope to send it later...
                                 log::line( log::category::warning, "failed to send shutdown to instance: ", instance, " - action: try send it later");

                                 m_state.persistent.replies.emplace_back( instance.process.ipc, message::shutdown::Request{});
                              }
                              break;
                           }
                        }
                     }
                  }
               }

               std::vector< admin::resource::Proxy> insances( State& state, std::vector< admin::update::Instances> instances)
               {
                  std::vector< admin::resource::Proxy> result;

                  // Make sure we only update a specific RM one time
                  for( auto& directive : algorithm::unique( algorithm::sort( instances)))
                  {
                     try
                     {
                        auto& resource = state.get_resource( directive.id);
                        resource.concurency = directive.instances;

                        Instances{ state}( resource);

                        result.push_back( admin::transform::resource::Proxy{}( resource));
                     }
                     catch( const common::exception::system::invalid::Argument&)
                     {
                        // User did not use correct resource-id. We propagate this by not including
                        // the resource in the result
                     }
                  }

                  return result;
               }

               namespace local
               {
                  namespace
                  {
                     namespace instance
                     {
                        bool request( const common::communication::message::Complete& message, state::resource::Proxy::Instance& instance)
                        {
                           Trace trace{ "transaction::action::resource::instance::request"};

                           if( ipc::device().non_blocking_push( instance.process.ipc, message))
                           {
                              instance.state( state::resource::Proxy::Instance::State::busy);
                              instance.metrics.requested = common::platform::time::clock::type::now();
                              return true;
                           }
                           return false;

                        }
                     } // instance

                  } // <unnamed>
               } // local



               bool request( State& state, state::pending::Request& message)
               {
                  Trace trace{ "transaction::action::resource::request"};

                  if( state::resource::id::local( message.resource))
                  {
                     auto found = state.idle_instance( message.resource);

                     if( found )
                     {

                        if( ! local::instance::request( message.message, *found))
                        {
                           log::line( log, "failed to send resource request - type: ", message.message.type, " to: ", found->process);
                           return false;
                        }
                     }
                     else
                     {
                        log::line( log, "failed to find idle resource - action: wait");
                        return false;
                     }
                  }
                  else
                  {
                     auto& domain = state.get_external( message.resource);

                     ipc::device().blocking_push( domain.process.ipc, message.message);
                  }
                  return true;
               }

            } // resource


            namespace persistent
            {

               bool Send::operator () ( state::pending::Reply& message) const
               {
                  try
                  {
                     if( ! ipc::device().non_blocking_push( message.target, message.message))
                     {
                        log::line( log, "failed to send reply - type: ", message.message.type, " to: ", message.target);
                        return false;
                     }
                  }
                  catch( const exception::system::communication::Unavailable&)
                  {
                     log::line( log::category::error, "failed to send reply - target: ", message.target, ", message: ", message.message, " - TODO: rollback transaction?");
                     //
                     // ipc-queue has been removed...
                     // TODO attention: deduce from message.message.type what we should do
                     //  We should rollback if we are in a prepare stage?
                     //
                  }
                  return true;
               }

               bool Send::operator () ( state::pending::Request& message) const
               {
                  return resource::request( m_state, message);
               }

            } // pending

         } // action
               
      } // manager   
   } //transaction
} // casual
