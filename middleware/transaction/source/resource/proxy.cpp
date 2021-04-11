//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "transaction/resource/proxy.h"
#include "transaction/common.h"

#include "common/transaction/id.h"
#include "common/message/transaction.h"
#include "common/process.h"
#include "common/message/dispatch.h"
#include "common/message/handle.h"
#include "common/communication/ipc.h"
#include "common/environment.h"

#include "common/code/raise.h"
#include "common/code/xa.h"

#include "common/communication/instance.h"

#include "serviceframework/log.h"


namespace casual
{
   using namespace common;

   namespace transaction
   {
      namespace resource
      {
         namespace proxy
         {
            namespace local
            {
               namespace
               {
                  namespace handle
                  {
                     void open( State& state)
                     {
                        Trace trace{ "open resource"};

                        auto code = state.resource.open();

                        if( code != code::xa::ok)
                           code::raise::error( code, "failed to open xa resource - id: ", state.resource.id(), ", name: ", state.resource.name());

                        message::transaction::resource::Ready ready{ common::process::handle()};
                        ready.id = state.resource.id();

                        communication::device::blocking::send(
                           communication::instance::outbound::transaction::manager::device(), ready);
                     }

                     template< typename M, typename A> 
                     void helper( State& state, M&& message, A&& action)
                     {
                        auto reply = common::message::reverse::type( message);

                        reply.statistics.start = platform::time::clock::type::now();

                        reply.process = common::process::handle();
                        reply.resource = state.resource.id();

                        reply.state = action( state.resource, message.trid, message.flags);
                        reply.trid = std::move( message.trid);

                        reply.statistics.end = platform::time::clock::type::now();

                        communication::device::blocking::send(
                           communication::instance::outbound::transaction::manager::device(), reply);  
                     }

                     auto prepare( State& state)
                     {
                        return [&state]( common::message::transaction::resource::prepare::Request& message)
                        {
                           helper( state, message, [](auto& resource, auto& trid, auto flags)
                           {
                              return resource.prepare( trid, flags);
                           });
                        };
                     }

                     auto commit( State& state)
                     {
                        return [&state]( common::message::transaction::resource::commit::Request& message)
                        {
                           helper( state, message, [](auto& resource, auto& trid, auto flags)
                           {
                              return resource.commit( trid, flags);
                           });
                        };
                     }

                     auto rollback( State& state)
                     {
                        return [&state]( common::message::transaction::resource::rollback::Request& message)
                        {
                           helper( state, message, [](auto& resource, auto& trid, auto flags)
                           {
                              return resource.rollback( trid, flags);
                           });
                        };
                     }

                  } // handle

                  namespace initialize
                  {
                     template< typename... Ts>
                     void validate( bool ok, Ts&&... ts)
                     {
                        if( ! ok)
                           code::raise::error( code::casual::invalid_argument, std::forward< Ts>( ts)...);
                     };

                     State state( proxy::Settings settings, casual_xa_switch_mapping* switches)
                     {
                        validate( switches->key && switches->xa_switch, "invalid xa-switch mapping ");

                        auto request = []( auto& settings)
                        {
                           message::transaction::resource::configuration::Request result{ process::handle()};
                           result.id = strong::resource::id{ settings.id};
                           return result;
                        };

                        auto configuration = communication::ipc::call( 
                           communication::instance::outbound::transaction::manager::device(), request( settings));

                        validate( configuration.resource.key == switches->key, "mismatch between expected resource key and configured resource key - resource: ", configuration.resource, ", key: ", switches->key);


                        return { { 
                           common::transaction::resource::Link{ switches->key, switches->xa_switch},
                           configuration.resource.id,
                           configuration.resource.openinfo,
                           configuration.resource.closeinfo}};
                     }
                  } // initialize

               } // <unnamed>
            } // local


         } // proxy


        Proxy::Proxy( proxy::Settings settings, casual_xa_switch_mapping* switches)
           : m_state{ proxy::local::initialize::state( std::move( settings), switches)}
        {
        }

        Proxy::~Proxy()
        {
           m_state.resource.close();
        }

         void Proxy::start()
         {
            auto& device = common::communication::ipc::inbound::device();
            
            auto handler = common::message::dispatch::handler( device,
               common::message::handle::defaults( device),
               proxy::local::handle::prepare( m_state),
               proxy::local::handle::commit( m_state),
               proxy::local::handle::rollback( m_state)
            );

            proxy::local::handle::open( m_state);

            common::log::line( log, "start message pump");

            common::message::dispatch::pump( 
               handler, 
               device);

         }
      } // resource


   } // transaction
} // casual





