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
#include "common/instance.h"

#include "common/code/raise.h"
#include "common/code/casual.h"

#include "domain/pending/message/send.h"
#include "domain/configuration/fetch.h"



#include <string>

namespace casual
{
   using namespace common;

   namespace transaction::manager::action
   {

      State state( manager::Settings settings)
      {
         Trace trace{ "transaction::manager::action::state"};

         return State{ 
            std::move( settings), 
            casual::domain::configuration::fetch(),
            configuration::resource::property::get()};
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
                  auto& info = m_state.resource.properties.at( proxy.key);

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
                     common::event::error::send( common::exception::code(), "failed to spawn resource-proxy-instance: " + info.server);
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
               if( auto resource = state.find_resource( directive.name))
               {
                  resource->concurency = directive.instances;
                  Instances{ state}( *resource);
                  result.push_back( admin::transform::resource::Proxy{}( *resource));
               }

               // else:
               // User did not use correct resource-id. We propagate this by not including
               // the resource in the result
      
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
            log::line( verbose::log, "message: ", message);

            if( state::resource::id::local( message.resource))
            {
               if( auto found = state.idle_instance( message.resource))
               {
                  if( local::instance::request( message.message, *found))
                     return true;
                  
                  log::line( log, "failed to send resource request - type: ", message.message.type, " to: ", found->process, " - action: try later");
                  return false;  
               }
      
               log::line( log, "failed to find idle resource - action: wait");
               return false;
            }

            // 'external' resource proxy
            auto& resource = state.get_external( message.resource);

            // we flush our inbound ipc to maximise success of sending directly (the resource might be trying to send
            // stuff to us right now, and our inbound and the resource inbound might be 'full', unlikely on linux
            // system but it can occur on OSX/BSD with extremely high loads)
            communication::ipc::inbound::device().flush();

            if( communication::device::non::blocking::put( resource.process.ipc, message.message).empty())
            {
               log::line( log, "failed to send resource request to 'external' resource -  ", resource.process, " - action: send via pending");
               casual::domain::pending::message::send( resource.process, std::move( message.message));
            }
            
            return true;
         }

      } // resource
         
   } //transaction::manager::action
} // casual
