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
#include "common/environment/normalize.h"
#include "common/server/handle/call.h"
#include "common/event/send.h"
#include "common/communication/instance.h"
#include "common/exception/casual.h"
#include "common/instance.h"

#include "domain/pending/message/send.h"


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
            State state( manager::Settings settings)
            {
               Trace trace{ "transaction::manager::action::state"};

               State state{ common::environment::string( std::move( settings.log))};

               // fetch configuration from domain-manager
               auto configuration = environment::normalize( communication::ipc::call( 
                  communication::instance::outbound::domain::manager::device(),
                  common::message::domain::configuration::Request{ process::handle()}));
                  

               {
                  Trace trace{ "transaction manager xa-switch configuration"};

                  auto resources = configuration::resource::property::get();

                  for( auto& resource : resources)
                  {
                     auto result = state.resource_properties.emplace( resource.key, std::move( resource));
                     if( ! result.second)
                        throw common::exception::casual::invalid::Configuration( "multiple keys in resource config: " + result.first->first);
                  }
               }

               // configure resources
               {
                  Trace trace{ "transaction manager resource configuration"};

                  auto transform_resource = []( const auto& r)
                  {
                     state::resource::Proxy proxy{ state::resource::Proxy::generate_id{}};

                     proxy.name = common::coalesce( r.name, common::string::compose( ".rm.", r.key, '.', proxy.id.value()));
                     proxy.concurency = r.instances;
                     proxy.key = r.key;
                     proxy.openinfo = r.openinfo;
                     proxy.closeinfo = r.closeinfo;
                     proxy.note = r.note;

                     return proxy;
                  };

                  auto validate = [&state]( const auto& r) 
                  {
                     if( ! common::algorithm::find( state.resource_properties, r.key))
                     {
                        log::line( log::category::error, "failed to correlate resource key '", r.key, "' - action: skip resource");

                        common::event::error::send( "failed to correlate resource key '" + r.key + "'");
                        return false;
                     }
                     return true;
                  };

                  common::algorithm::transform_if(
                     configuration.domain.transaction.resources,
                     state.resources,
                     transform_resource,
                     validate);
               }

               return state;
            }


            namespace resource
            {
               void Instances::operator () ( state::resource::Proxy& proxy)
               {
                  Trace trace( "resource::Instances::operator()");

                  log::line( log, "update instances for resource: ", proxy);
               
                  auto count = proxy.concurency - range::size( proxy.instances);

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
                                 "--id", std::to_string( proxy.id.value()),
                              },
                              { common::instance::variable( { proxy.name, proxy.id.value()})}
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

                              if( ! communication::device::non::blocking::send( instance.process.ipc, message::shutdown::Request{}))
                              {
                                 // We couldn't send shutdown for some reason, we send it to pending
                                 casual::domain::pending::message::send( instance.process, message::shutdown::Request{});
                              }
                              break;
                           }
                        }
                     }
                  }
               }

               std::vector< admin::model::resource::Proxy> instances( State& state, std::vector< admin::model::scale::Instances> instances)
               {
                  std::vector< admin::model::resource::Proxy> result;

                  // Make sure we only update a specific RM one time
                  for( auto& directive : algorithm::unique( algorithm::sort( instances)))
                  {
                     try
                     {
                        auto& resource = state.get_resource( directive.name);
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

                           if( communication::device::non::blocking::put( instance.process.ipc, message))
                           {
                              instance.state( state::resource::Proxy::Instance::State::busy);
                              instance.metrics.requested = platform::time::clock::type::now();
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
                     communication::device::blocking::put( domain.process.ipc, message.message);
                  }
                  return true;
               }

            } // resource

         } // action
      } // manager   
   } //transaction
} // casual
