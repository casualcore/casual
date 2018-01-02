//!
//! casual
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

#include "sf/log.h"




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
               namespace local
               {
                  namespace
                  {
                     common::code::xa convert( int value)
                     {
                        return static_cast< common::code::xa>( value);
                     }
                  } // <unnamed>
               } // local

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

                     auto openinfo = common::environment::string( m_state.rm_openinfo);

                     reply.process = common::process::handle();
                     reply.resource = m_state.rm_id;

                     reply.state = local::convert( 
                        m_state.xa_switches->xa_switch->xa_open_entry( 
                           openinfo.c_str(), m_state.rm_id.value(), common::cast::underlying( common::flag::xa::Flag::no_flags)));


                     {
                        common::log::trace::Outcome log_connect{ "resource connect to transaction monitor", log};

                        common::communication::ipc::blocking::send(
                              common::communication::ipc::transaction::manager::device(), reply);
                     }

                     if( reply.state != common::code::xa::ok)
                     {
                        auto message = common::string::compose( reply.state, " failed to open xa resource ", m_state.rm_key);

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

                     reply.statistics.start = common::platform::time::clock::type::now();


                     reply.process = common::process::handle();
                     reply.resource = m_state.rm_id;

                     reply.state = policy_type()( m_state, message);
                     reply.trid = std::move( message.trid);

                     reply.statistics.end = common::platform::time::clock::type::now();

                     common::communication::ipc::blocking::send(
                           common::communication::ipc::transaction::manager::device(), reply);

                  }
               };


               namespace policy
               {
                  struct Prepare
                  {
                     template< typename M>
                     common::code::xa operator() ( State& state, M& message) const
                     {
                        auto result = local::convert( state.xa_switches->xa_switch->xa_prepare_entry( 
                           &message.trid.xid, state.rm_id.value(), message.flags.underlaying()));

                        common::log::line( log, result, " prepare rm: ", state.rm_id, " trid: ", message.trid, " flags: ", message.flags);

                        return result;
                     }
                  };

                  struct Commit
                  {
                     template< typename M>
                     common::code::xa operator() ( State& state, M& message) const
                     {
                        auto result = local::convert( state.xa_switches->xa_switch->xa_commit_entry( 
                           &message.trid.xid, state.rm_id.value(), message.flags.underlaying()));

                        common::log::line( log, result, " commit rm: ", state.rm_id, " trid: ", message.trid, " flags: ", message.flags);

                        return result;
                     }
                  };

                  struct Rollback
                  {
                     template< typename M>
                     common::code::xa operator() ( State& state, M& message) const
                     {
                        auto result = local::convert( state.xa_switches->xa_switch->xa_rollback_entry( 
                           &message.trid.xid, state.rm_id.value(), message.flags.underlaying()));
                        
                        common::log::line( log, result, " rollback rm: ", state.rm_id, " trid: ", message.trid, " flags: ", message.flags);
                        
                        return result;
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

            namespace validate
            {
               void state( const State& state)
               {
                  if( ! ( state.xa_switches && state.xa_switches->key && state.xa_switches->key == state.rm_key
                        && ! state.rm_key.empty()))
                  {
                     throw common::exception::system::invalid::Argument( "mismatch between expected resource key and configured resource key");
                  }

                  if( ! state.xa_switches->xa_switch)
                  {
                     throw common::exception::system::invalid::Argument( "xa-switch is null");
                  }

               }
            } // validate
         } // proxy


        Proxy::Proxy( proxy::Settings&& settings, casual_xa_switch_mapping* switches)
           : m_state( std::move(settings), switches)
        {
           proxy::validate::state( m_state);

        }

        Proxy::~Proxy()
        {
           auto result = common::code::xa(
               m_state.xa_switches->xa_switch->xa_close_entry( 
                  m_state.rm_closeinfo.c_str(), 
                  m_state.rm_id.value(), 
                  common::cast::underlying( common::flag::xa::Flag::no_flags)));

           if( result != common::code::xa::ok)
           {
              common::code::stream( result) << result << " - failed to close resource\n";
              log << CASUAL_MAKE_NVP( m_state);
           }
        }

         void Proxy::start()
         {

            log << "open resource\n";
            proxy::handle::Open{ m_state}();

            //
            // prepare message dispatch handlers...
            //
            log << "prepare message dispatch handlers\n";

            auto handler = common::communication::ipc::inbound::device().handler(
               common::message::handle::Shutdown{},
               proxy::handle::Prepare{ m_state},
               proxy::handle::Commit{ m_state},
               proxy::handle::Rollback{ m_state}
            );


            log << "start message pump\n";

            while( handler( common::communication::ipc::inbound::device().next( common::communication::ipc::policy::Blocking{})))
            {
               ;
            }

         }
      } // resource


   } // transaction
} // casual





