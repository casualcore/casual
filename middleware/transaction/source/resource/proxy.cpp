//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "transaction/resource/proxy.h"
#include "transaction/common.h"

#include "common/message/transaction.h"
#include "common/exception/xa.h"
#include "common/process.h"
#include "common/message/dispatch.h"
#include "common/message/handle.h"
#include "common/communication/ipc.h"
#include "common/event/send.h"
#include "common/environment.h"

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
                     struct Base
                     {
                        Base( State& state ) : state( state) {}
                     protected:
                        State& state;
                     };


                     void open( State& state)
                     {
                        Trace trace{ "open resource"};

                        auto result = state.resource.open();

                        if( result != code::xa::ok)
                        {
                           auto message = string::compose( result, " failed to open xa resource ", state.resource.id());
                           event::error::send( message);

                           throw exception::xa::exception{ result, message};
                        }

                        message::transaction::resource::Ready ready{ common::process::handle()};
                        ready.id = state.resource.id();

                        communication::ipc::blocking::send(
                           communication::instance::outbound::transaction::manager::device(), ready);
                     }


                     template< typename P>
                     struct basic_handler : public Base
                     {
                        using policy_type = P;
                        //! deduce second argument as message type.
                        using message_type = common::traits::remove_cvref_t< typename common::traits::function< policy_type>::template argument_t< 1>>; 

                        using Base::Base;

                        void operator () ( message_type& message)
                        {
                           auto reply = common::message::reverse::type( message);

                           reply.statistics.start = platform::time::clock::type::now();

                           reply.process = common::process::handle();
                           reply.resource = state.resource.id();

                           reply.state = policy_type()( state, message);
                           reply.trid = std::move( message.trid);

                           reply.statistics.end = platform::time::clock::type::now();

                           communication::ipc::blocking::send(
                              communication::instance::outbound::transaction::manager::device(), reply);

                        }
                     };


                     namespace policy
                     {
                        struct Prepare
                        {
                           auto operator() ( State& state, common::message::transaction::resource::prepare::Request& message) const
                           {
                              return state.resource.prepare( message.trid, message.flags);
                           }
                        };

                        struct Commit
                        {
                           auto operator() ( State& state, common::message::transaction::resource::commit::Request& message) const
                           {
                              return state.resource.commit( message.trid, message.flags);
                           }
                        };

                        struct Rollback
                        {
                           auto operator() ( State& state, common::message::transaction::resource::rollback::Request& message) const
                           {
                              return state.resource.rollback( message.trid, message.flags);
                           }
                        };

                     } // policy

                     using Prepare = basic_handler< policy::Prepare>;
                     using Commit = basic_handler< policy::Commit>;
                     using Rollback = basic_handler< policy::Rollback>;

                  } // handle

                  namespace initialize
                  {
                     template< typename... Ts>
                     void validate( bool ok, Ts&&... ts)
                     {
                        if( ! ok)
                           throw exception::system::invalid::Argument{ common::string::compose( std::forward< Ts>( ts)...)};
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
            auto handler = common::communication::ipc::inbound::device().handler(
               common::message::handle::Shutdown{},
               proxy::local::handle::Prepare{ m_state},
               proxy::local::handle::Commit{ m_state},
               proxy::local::handle::Rollback{ m_state}
            );

            proxy::local::handle::open( m_state);


            common::log::line( log, "start message pump");

            common::message::dispatch::blocking::pump( 
               handler, 
               common::communication::ipc::inbound::device());

         }
      } // resource


   } // transaction
} // casual





