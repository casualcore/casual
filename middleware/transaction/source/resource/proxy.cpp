//!
//! server.cpp
//!
//! Created on: Jul 25, 2013
//!     Author: Lazan
//!

#include "transaction/resource/proxy.h"

#include "common/message/transaction.h"
#include "common/exception.h"
#include "common/process.h"
#include "common/trace.h"
#include "common/message/dispatch.h"
#include "common/message/handle.h"
#include "common/internal/trace.h"
#include "common/communication/ipc.h"

#include "sf/log.h"




namespace casual
{
   namespace transaction
   {
      namespace resource
      {
         using namespace common;

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
               using reply_type = message::transaction::resource::connect::Reply;

               using Base::Base;

               void operator() ()
               {
                  common::trace::internal::Scope trace{ "open resource"};

                  reply_type reply;

                  reply.process = common::process::handle();
                  reply.resource = m_state.rm_id;

                  reply.state = m_state.xa_switches->xa_switch->xa_open_entry( m_state.rm_openinfo.c_str(), m_state.rm_id, TMNOFLAGS);


                  common::trace::Outcome logConnect{ "resource connect to transaction monitor", common::log::internal::transaction};

                  communication::ipc::blocking::send( m_state.tm_queue, reply);

                  if( reply.state != XA_OK)
                  {
                     throw exception::NotReallySureWhatToNameThisException( "failed to open xa resurce " + m_state.rm_key + " with: " + m_state.rm_openinfo);
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

                  reply.statistics.start = platform::clock_type::now();


                  reply.process = common::process::handle();
                  reply.resource = m_state.rm_id;

                  reply.state = policy_type()( m_state, message);
                  reply.trid = std::move( message.trid);

                  reply.statistics.end = platform::clock_type::now();

                  communication::ipc::blocking::send( m_state.tm_queue, reply);

               }
            };


            namespace policy
            {
               struct Prepare
               {
                  template< typename M>
                  int operator() ( State& state, M& message) const
                  {
                     auto result = state.xa_switches->xa_switch->xa_prepare_entry( &message.trid.xid, state.rm_id, message.flags);
                     log::internal::transaction << error::xa::error( result) << " prepare rm: " << state.rm_id << " trid: " << message.trid << " flags: " << std::hex << message.flags << std::dec << std::endl;
                     return result;
                  }
               };

               struct Commit
               {
                  template< typename M>
                  int operator() ( State& state, M& message) const
                  {
                     auto result =  state.xa_switches->xa_switch->xa_commit_entry( &message.trid.xid, state.rm_id, message.flags);

                     if( log::internal::transaction)
                     {
                        log::internal::transaction << error::xa::error( result) << " commit rm: " << state.rm_id << " trid: " << message.trid << " flags: " << std::hex << message.flags << std::dec << std::endl;
                        if( result != XA_OK)
                        {
                           std::array< XID, 12> xids;
                           auto count = state.xa_switches->xa_switch->xa_recover_entry( xids.data(), xids.max_size(), state.rm_id, TMSTARTRSCAN | TMENDRSCAN);

                           while( count > 0)
                           {
                              log::internal::transaction << "prepared xid: " << xids[ count -1] << std::endl;
                              --count;
                           }

                        }
                     }

                     return result;
                  }
               };

               struct Rollback
               {
                  template< typename M>
                  int operator() ( State& state, M& message) const
                  {
                     auto result =  state.xa_switches->xa_switch->xa_rollback_entry( &message.trid.xid, state.rm_id, message.flags);
                     log::internal::transaction << error::xa::error( result) << " rollback rm: " << state.rm_id << " trid: " << message.trid << " flags: " << std::hex << message.flags << std::dec << std::endl;
                     return result;
                  }
               };

            } // policy

            using Open = basic_open;

            using Prepare = basic_handler<
                  message::transaction::resource::prepare::Request,
                  message::transaction::resource::prepare::Reply,
                  policy::Prepare>;


            using Commit = basic_handler<
                  message::transaction::resource::commit::Request,
                  message::transaction::resource::commit::Reply,
                  policy::Commit>;

            using Rollback = basic_handler<
                  message::transaction::resource::rollback::Request,
                  message::transaction::resource::rollback::Reply,
                  policy::Rollback>;


            /*
            namespace domain
            {
               using Prepare = basic_handler<
                     message::transaction::resource::domain::prepare::Request,
                     message::transaction::resource::domain::prepare::Reply,
                     policy::Prepare>;


               using Commit = basic_handler<
                     message::transaction::resource::domain::commit::Request,
                     message::transaction::resource::domain::commit::Reply,
                     policy::Commit>;

               using Rollback = basic_handler<
                     message::transaction::resource::domain::rollback::Request,
                     message::transaction::resource::domain::rollback::Reply,
                     policy::Rollback>;
            } // domain
            */


         } // handle

         namespace validate
         {
            void state( const State& state)
            {
               if( ! ( state.xa_switches && state.xa_switches->key && state.xa_switches->key == state.rm_key
                     && ! state.rm_key.empty()))
               {
                  throw exception::NotReallySureWhatToNameThisException( "mismatch between expected resource key and configured resource key");
               }

               if( ! state.xa_switches->xa_switch)
               {
                  throw exception::NotReallySureWhatToNameThisException( "xa-switch is null");
               }

            }
         } // validate

        Proxy::Proxy( State&& state)
           : m_state( std::move(state))
        {
           validate::state( m_state);

        }

        Proxy::~Proxy()
        {
           auto result = m_state.xa_switches->xa_switch->xa_close_entry( m_state.rm_closeinfo.c_str(), m_state.rm_id, TMNOFLAGS);

           if( result != XA_OK)
           {
              common::log::error << common::error::xa::error( result) << " - failed to close resource" << std::endl;
              common::log::internal::transaction << CASUAL_MAKE_NVP( m_state);
           }
        }

         void Proxy::start()
         {

            common::log::internal::transaction << "open resource\n";
            handle::Open{ m_state}();

            //
            // prepare message dispatch handlers...
            //
            common::log::internal::transaction << "prepare message dispatch handlers\n";

            message::dispatch::Handler handler{
               common::message::handle::Shutdown{},
               handle::Prepare{ m_state},
               handle::Commit{ m_state},
               handle::Rollback{ m_state},
               /*
               handle::domain::Prepare{ m_state},
               handle::domain::Commit{ m_state},
               handle::domain::Rollback{ m_state},
               */
            };


            common::log::internal::transaction << "start message pump\n";

            while( handler( communication::ipc::inbound::device().next( communication::ipc::policy::Blocking{})))
            {
               ;
            }

         }
      } // resource


   } // transaction
} // casual





