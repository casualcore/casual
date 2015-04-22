//!
//! server.cpp
//!
//! Created on: Jul 25, 2013
//!     Author: Lazan
//!

#include "transaction/resource/proxy.h"

#include "common/message/transaction.h"
#include "common/exception.h"
#include "common/queue.h"
#include "common/process.h"
#include "common/trace.h"
#include "common/message/dispatch.h"
#include "common/message/handle.h"
#include "common/internal/trace.h"

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


            template< typename TQ>
            struct basic_open : public Base
            {
               using tm_queue_type = TQ;
               using reply_type = message::transaction::resource::connect::Reply;

               using Base::Base;

               void operator() ()
               {
                  common::trace::internal::Scope trace{ "open resource"};

                  reply_type reply;

                  reply.process = common::process::handle();
                  reply.resource = m_state.rm_id;

                  reply.state = m_state.xaSwitches->xaSwitch->xa_open_entry( m_state.rm_openinfo.c_str(), m_state.rm_id, TMNOFLAGS);


                  common::trace::Outcome logConnect{ "resource connect to transaction monitor", common::log::internal::transaction};

                  tm_queue_type m_tmQueue{ m_state.tm_queue};
                  m_tmQueue( reply);

                  if( reply.state != XA_OK)
                  {
                     throw exception::NotReallySureWhatToNameThisException( "failed to open xa resurce " + m_state.rm_key + " with: " + m_state.rm_openinfo);
                  }
               }
            };

            template< typename M, typename R, typename P, typename TQ>
            struct basic_handler : public Base
            {
               using tm_queue_type = TQ;
               using message_type = M;
               using reply_type = R;
               using policy_type = P;

               using Base::Base;

               void operator () ( message_type& message)
               {
                  reply_type reply;

                  reply.process = common::process::handle();
                  reply.resource = m_state.rm_id;

                  reply.state = policy_type()( m_state, message);
                  reply.trid = std::move( message.trid);


                  tm_queue_type m_tmQueue{ m_state.tm_queue};
                  m_tmQueue( reply);

               }
            };


            namespace policy
            {
               struct Prepare
               {
                  template< typename M>
                  int operator() ( State& state, M& message) const
                  {
                     auto result = state.xaSwitches->xaSwitch->xa_prepare_entry( &message.trid.xid, state.rm_id, message.flags);
                     log::internal::transaction << error::xa::error( result) << " prepare rm: " << state.rm_id << " trid: " << message.trid << " flags: " << std::hex << message.flags << std::dec << std::endl;
                     return result;
                  }
               };

               struct Commit
               {
                  template< typename M>
                  int operator() ( State& state, M& message) const
                  {
                     auto result =  state.xaSwitches->xaSwitch->xa_commit_entry( &message.trid.xid, state.rm_id, message.flags);

                     if( log::internal::transaction)
                     {
                        log::internal::transaction << error::xa::error( result) << " commit rm: " << state.rm_id << " trid: " << message.trid << " flags: " << std::hex << message.flags << std::dec << std::endl;
                        if( result != XA_OK)
                        {
                           std::array< XID, 12> xids;
                           auto count = state.xaSwitches->xaSwitch->xa_recover_entry( xids.data(), xids.max_size(), state.rm_id, TMSTARTRSCAN | TMENDRSCAN);

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
                     auto result =  state.xaSwitches->xaSwitch->xa_rollback_entry( &message.trid.xid, state.rm_id, message.flags);
                     log::internal::transaction << error::xa::error( result) << " rollback rm: " << state.rm_id << " trid: " << message.trid << " flags: " << std::hex << message.flags << std::dec << std::endl;
                     return result;
                  }
               };

            } // policy

            using Open = basic_open< queue::blocking::Writer>;

            using Prepare = basic_handler<
                  message::transaction::resource::prepare::Request,
                  message::transaction::resource::prepare::Reply,
                  policy::Prepare,
                  queue::blocking::Writer>;


            using Commit = basic_handler<
                  message::transaction::resource::commit::Request,
                  message::transaction::resource::commit::Reply,
                  policy::Commit,
                  queue::blocking::Writer>;

            using Rollback = basic_handler<
                  message::transaction::resource::rollback::Request,
                  message::transaction::resource::rollback::Reply,
                  policy::Rollback,
                  queue::blocking::Writer>;


            namespace domain
            {
               using Prepare = basic_handler<
                     message::transaction::resource::domain::prepare::Request,
                     message::transaction::resource::domain::prepare::Reply,
                     policy::Prepare,
                     queue::blocking::Writer>;


               using Commit = basic_handler<
                     message::transaction::resource::domain::commit::Request,
                     message::transaction::resource::domain::commit::Reply,
                     policy::Commit,
                     queue::blocking::Writer>;

               using Rollback = basic_handler<
                     message::transaction::resource::domain::rollback::Request,
                     message::transaction::resource::domain::rollback::Reply,
                     policy::Rollback,
                     queue::blocking::Writer>;
            } // domain


         } // handle

         namespace validate
         {
            void state( const State& state)
            {
               if( ! ( state.xaSwitches && state.xaSwitches->key && state.xaSwitches->key == state.rm_key
                     && ! state.rm_key.empty()))
               {
                  throw exception::NotReallySureWhatToNameThisException( "mismatch between expected resource key and configured resource key");
               }

               if( ! state.xaSwitches->xaSwitch)
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
           auto result = m_state.xaSwitches->xaSwitch->xa_close_entry( m_state.rm_closeinfo.c_str(), m_state.rm_id, TMNOFLAGS);

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
               handle::domain::Prepare{ m_state},
               handle::domain::Commit{ m_state},
               handle::domain::Rollback{ m_state},
            };


            common::log::internal::transaction << "start message pump\n";

            common::queue::blocking::Reader receiveQueue( common::ipc::receive::queue());
            message::dispatch::pump( handler, receiveQueue);

         }
      } // resource


   } // transaction
} // casual





