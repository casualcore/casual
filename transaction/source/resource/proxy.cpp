//!
//! server.cpp
//!
//! Created on: Jul 25, 2013
//!     Author: Lazan
//!

#include "transaction/resource/proxy.h"

#include "common/message.h"
#include "common/exception.h"
#include "common/queue.h"
#include "common/trace.h"
#include "common/message_dispatch.h"
#include "common/internal/trace.h"



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

                  reply.id.pid = common::process::id();
                  reply.resource = m_state.rm_id;
                  reply.id.queue_id = common::ipc::receive::id();

                  reply.state = m_state.xaSwitches->xaSwitch->xa_open_entry( m_state.rm_openinfo.c_str(), m_state.rm_id, TMNOFLAGS);


                  common::trace::Exit logConnect{ "resource connect to transaction monitor"};

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

               void dispatch( message_type& message)
               {
                  reply_type reply;

                  reply.id.pid = common::process::id();
                  reply.id.queue_id = common::ipc::receive::id();
                  reply.resource = m_state.rm_id;

                  reply.state = policy_type()( m_state, message);
                  reply.xid = std::move( message.xid);


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
                     return state.xaSwitches->xaSwitch->xa_prepare_entry( &message.xid.xid(), state.rm_id, TMNOFLAGS);
                  }
               };

               struct Commit
               {
                  template< typename M>
                  int operator() ( State& state, M& message) const
                  {
                     return state.xaSwitches->xaSwitch->xa_commit_entry( &message.xid.xid(), state.rm_id, TMNOFLAGS);
                  }
               };

               struct Rollback
               {
                  template< typename M>
                  int operator() ( State& state, M& message) const
                  {
                     return state.xaSwitches->xaSwitch->xa_rollback_entry( &message.xid.xid(), state.rm_id, TMNOFLAGS);
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

         void Proxy::start()
         {

            common::log::internal::transaction << "open resource\n";
            handle::Open{ m_state}();

            //
            // prepare message dispatch handlers...
            //
            common::log::internal::transaction << "prepare message dispatch handlers\n";

            message::dispatch::Handler handler;

            handler.add( handle::Prepare{ m_state});
            handler.add( handle::Commit{ m_state});
            handler.add( handle::Rollback{ m_state});
            handler.add( handle::domain::Prepare{ m_state});
            handler.add( handle::domain::Commit{ m_state});
            handler.add( handle::domain::Rollback{ m_state});


            common::log::internal::transaction << "start message pump\n";

            common::queue::blocking::Reader receiveQueue( common::ipc::receive::queue());
            message::dispatch::pump( handler, receiveQueue);

         }
      } // resource


   } // transaction
} // casual





