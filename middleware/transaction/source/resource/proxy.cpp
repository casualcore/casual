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
   namespace transaction
   {
      namespace resource
      {
         namespace proxy
         {
            namespace handle
            {
               struct Base
               {
                  Base( State& state ) : m_state( state) {}
               protected:
                  State& m_state;
               };


               struct basic_open : public Base
               {
                  using reply_type = common::message::transaction::resource::connect::Reply;

                  using Base::Base;

                  void operator() ()
                  {
                     Trace trace{ "open resource"};

                     reply_type reply;

                     reply.process = common::process::handle();
                     reply.resource = m_state.resource.id();

                     reply.state = m_state.resource.open();

                     {
                        common::log::trace::Outcome log_connect{ "resource connect to transaction monitor", log};

                        common::communication::ipc::blocking::send(
                              common::communication::instance::outbound::transaction::manager::device(), reply);
                     }

                     if( reply.state != common::code::xa::ok)
                     {
                        auto message = common::string::compose( reply.state, " failed to open xa resource ", m_state.resource.id());

                        common::event::error::send( message);

                        throw common::exception::xa::exception{ reply.state, message};
                     }
                  }
               };

               template< typename M, typename R, typename P>
               struct basic_handler : public Base
               {
                  using message_type = M;
                  using reply_type = R;
                  using policy_type = P;

                  using Base::Base;

                  void operator () ( message_type& message)
                  {
                     reply_type reply;

                     reply.statistics.start = platform::time::clock::type::now();


                     reply.process = common::process::handle();
                     reply.resource = m_state.resource.id();

                     reply.state = policy_type()( m_state, message);
                     reply.trid = std::move( message.trid);

                     reply.statistics.end = platform::time::clock::type::now();

                     common::communication::ipc::blocking::send(
                           common::communication::instance::outbound::transaction::manager::device(), reply);

                  }
               };


               namespace policy
               {
                  struct Prepare
                  {
                     template< typename M>
                     common::code::xa operator() ( State& state, M& message) const
                     {
                        return state.resource.prepare( message.trid, message.flags);
                     }
                  };

                  struct Commit
                  {
                     template< typename M>
                     common::code::xa operator() ( State& state, M& message) const
                     {
                        return state.resource.commit( message.trid, message.flags);
                     }
                  };

                  struct Rollback
                  {
                     template< typename M>
                     common::code::xa operator() ( State& state, M& message) const
                     {
                        return state.resource.rollback( message.trid, message.flags);
                     }
                  };

               } // policy

               using Open = basic_open;

               using Prepare = basic_handler<
                     common::message::transaction::resource::prepare::Request,
                     common::message::transaction::resource::prepare::Reply,
                     policy::Prepare>;


               using Commit = basic_handler<
                     common::message::transaction::resource::commit::Request,
                     common::message::transaction::resource::commit::Reply,
                     policy::Commit>;

               using Rollback = basic_handler<
                     common::message::transaction::resource::rollback::Request,
                     common::message::transaction::resource::rollback::Reply,
                     policy::Rollback>;


            } // handle

            State::State( Settings&& settings, casual_xa_switch_mapping* switches)
               : resource{
                  common::transaction::resource::Link{ std::move( settings.key), switches->xa_switch},
                  common::strong::resource::id{ settings.id}, 
                  std::move( settings.openinfo), 
                  std::move( settings.closeinfo)}
            {
               if( resource.key() != switches->key)
               {
                  throw common::exception::system::invalid::Argument( "mismatch between expected resource key and configured resource key");
               }
            }

         } // proxy


        Proxy::Proxy( proxy::Settings&& settings, casual_xa_switch_mapping* switches)
           : m_state( std::move(settings), switches)
        {
        }

        Proxy::~Proxy()
        {
           m_state.resource.close();
        }

         void Proxy::start()
         {

            common::log::line( log, "open resource");
            proxy::handle::Open{ m_state}();

            // prepare message dispatch handlers...
            common::log::line( log, "prepare message dispatch handlers");

            auto handler = common::communication::ipc::inbound::device().handler(
               common::message::handle::Shutdown{},
               proxy::handle::Prepare{ m_state},
               proxy::handle::Commit{ m_state},
               proxy::handle::Rollback{ m_state}
            );

            common::log::line( log, "start message pump");

            common::message::dispatch::blocking::pump( 
               handler, 
               common::communication::ipc::inbound::device());

         }
      } // resource


   } // transaction
} // casual





