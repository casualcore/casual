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
#include "common/communication/ipc/flush/send.h"
#include "common/instance.h"

#include "common/code/raise.h"
#include "common/code/casual.h"

#include "domain/configuration/fetch.h"



#include <string>

namespace casual
{
   using namespace common;

   namespace transaction::manager::action
   {

      namespace resource
      {
         namespace scale
         {
            void instances( State& state, state::resource::Proxy& proxy)
            {
               Trace trace( "resource::Instances::operator()");
               log::line( log, "update instances for resource: ", proxy);
            
               auto count = proxy.configuration.instances - range::size( proxy.instances);

               if( count > 0)
               {
                  while( count-- > 0)
                  {
                     auto found = algorithm::find( state.system.configuration.resources, proxy.configuration.key);

                     if( ! found)
                        return;

                     state::resource::Proxy::Instance instance;
                     instance.id = proxy.id;

                     try
                     {
                        instance.process.pid = process::spawn(
                           found->server,
                           {
                              "--id", std::to_string( proxy.id.value()),
                           },
                           { common::instance::variable( { proxy.configuration.name, proxy.id.value()})}
                        );

                        instance.state( state::resource::Proxy::Instance::State::started);

                        proxy.instances.push_back( std::move( instance));
                     }
                     catch( ...)
                     {
                        common::event::error::send( common::exception::capture().code(), "failed to spawn resource-proxy-instance: " + found->server);
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
                        using State = decltype( instance.state());

                        case State::absent:
                        case State::started:
                        {

                           log::line( log, "Instance has not register yet. We, kill it...: ", instance);

                           process::lifetime::terminate( { instance.process.pid});
                           instance.state( State::shutdown);
                           break;
                        }
                        case State::shutdown:
                        {
                           log::line( log, "instance already in shutdown state - ", instance);
                           break;
                        }
                        default:
                        {
                           log::line( log, "shutdown instance: ", instance);
                           instance.state( State::shutdown);
                           communication::ipc::flush::send( instance.process.ipc, message::shutdown::Request{});
                           break;
                        }
                     }
                  }
               }
            }
         } // scale


         std::vector< admin::model::resource::Proxy> instances( State& state, std::vector< admin::model::scale::Instances> instances)
         {
            std::vector< admin::model::resource::Proxy> result;

            // Make sure we only update a specific RM one time
            for( auto& directive : algorithm::unique( algorithm::sort( instances)))
            {
               if( auto resource = state.find_resource( directive.name))
               {
                  resource->configuration.instances = directive.instances;
                  scale::instances( state, *resource);
                  result.push_back( admin::transform::resource::proxy( *resource));
               }

               // else:
               // User did not use correct resource-id. We propagate this by not including
               // the resource in the result
      
            }

            return result;
         }

      } // resource
         
   } //transaction::manager::action
} // casual
