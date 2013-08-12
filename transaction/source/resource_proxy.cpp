//!
//! server.cpp
//!
//! Created on: Jul 25, 2013
//!     Author: Lazan
//!

#include "transaction/resource_proxy.h"

#include "common/message.h"
#include "common/exception.h"
#include "common/queue.h"
#include "common/trace.h"
#include "common/message_dispatch.h"

/*
int main( int argc, char** argv)
{
   std::cout << "rm name: " << tmswitch->name << std::endl;
   std::cout << "rm flags: " << tmswitch->flags << std::endl;

   auto result = tmswitch->xa_open_entry( "db=test,uid=db2,pwd=db2", 1, TMNOFLAGS);

   std::cout << "open test: " << status( result) << std::endl;

   result = tmswitch->xa_open_entry( "db=test2,uid=db2,pwd=db2", 2, TMNOFLAGS);

   std::cout << "open test2: " << status( result) << std::endl;

   std::cout << "close test: " << status( close( 1)) << std::endl;
   std::cout << "close test2: " << status( close( 2)) << std::endl;

   return 0;
}
 */

namespace casual
{
   namespace transaction
   {
      namespace resource
      {

         const int rm_id = 1;

        using namespace common;

         namespace handle
         {

            struct Base
            {
               Base( State& state) : m_state( state) {}

            protected:
               State& m_state;

            };


            struct Open : public Base
            {
               //typedef message::transaction::Begin message_type;

               using Base::Base;

               void operator() ()
               {

                  message::transaction::reply::resource::Connect reply;
                  reply.id.pid = common::process::id();
                  reply.id.queue_id = common::ipc::getReceiveQueue().id();

                  reply.state = m_state.xaSwitches->xaSwitch->xa_open_entry( m_state.rm_openinfo.c_str(), rm_id, TMNOFLAGS);

                  typedef queue::ipc_wrapper< queue::blocking::Writer> writer_type;
                  writer_type tm_queue( m_state.tm_queue);
                  tm_queue( reply);

                  if( reply.state != XA_OK)
                  {
                     throw exception::NotReallySureWhatToNameThisException( "failed to open xa resurce " + m_state.rm_key + " with: " + m_state.rm_openinfo);
                  }
               }
            };

            struct Prepare : public Base
            {
               typedef message::transaction::Prepare message_type;

               using Base::Base;

               void dispatch( message_type& message)
               {
                  message::transaction::reply::resource::Generic reply;

                  reply.id.pid = common::process::id();
                  reply.id.queue_id = common::ipc::getReceiveQueue().id();
                  reply.state = m_state.xaSwitches->xaSwitch->xa_prepare_entry( &message.xid, rm_id, TMNOFLAGS);

                  typedef queue::ipc_wrapper< queue::blocking::Writer> writer_type;
                  writer_type tm_queue( m_state.tm_queue);
                  tm_queue( reply);
               }
            };

            struct Commit : public Base
            {
               typedef message::transaction::Commit message_type;

               using Base::Base;

               void dispatch( message_type& message)
               {
                  message::transaction::reply::resource::Generic reply;

                  reply.id.pid = common::process::id();
                  reply.id.queue_id = common::ipc::getReceiveQueue().id();
                  reply.state = m_state.xaSwitches->xaSwitch->xa_commit_entry( &message.xid, rm_id, TMNOFLAGS);

                  typedef queue::ipc_wrapper< queue::blocking::Writer> writer_type;
                  writer_type tm_queue( m_state.tm_queue);
                  tm_queue( reply);

               }
            };

            struct Rollback : public Base
            {
               typedef message::transaction::Rollback message_type;

               using Base::Base;

               void dispatch( message_type& message)
               {
                  message::transaction::reply::resource::Generic reply;

                  reply.id.pid = common::process::id();
                  reply.id.queue_id = common::ipc::getReceiveQueue().id();
                  reply.state = m_state.xaSwitches->xaSwitch->xa_rollback_entry( &message.xid, rm_id, TMNOFLAGS);

                  typedef queue::ipc_wrapper< queue::blocking::Writer> writer_type;
                  writer_type tm_queue( m_state.tm_queue);
                  tm_queue( reply);

               }
            };

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
           : m_state(std::move(state))
         {
           validate::state( m_state);

           {
              common::trace::Exit log( "resource proxy open resource");

              handle::Open( m_state)();
           }

         }

         void Proxy::start()
         {
            common::logger::debug << "resource proxy start";

            //
            // prepare message dispatch handlers...
            //

            message::dispatch::Handler handler;

            handler.add< handle::Prepare>( m_state);
            handler.add< handle::Commit>( m_state);
            handler.add< handle::Rollback>( m_state);

            message::dispatch::pump( handler);

         }
      } // resource


   } // transaction
} // casual





