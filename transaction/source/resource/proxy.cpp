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

            template< typename TQ>
            struct Base
            {

               Base( State& state, TQ& tm_queue) : m_state( state), m_tmQueue( tm_queue) {}

            protected:
               State& m_state;
               TQ& m_tmQueue;

            };


            template< typename TQ>
            struct Open : public Base< TQ>
            {

               using Base< TQ>::Base;

               void operator() ()
               {
                  //
                  // Note: base is template parameter dependent, we have to use this->.
                  // this-> should never be used in other constructs...
                  //

                  common::trace::Exit logOpen{ "resource proxy open"};

                  message::transaction::resource::connect::Reply reply;
                  reply.id.pid = common::process::id();
                  reply.resource = this->m_state.rm_id;
                  reply.id.queue_id = common::ipc::getReceiveQueue().id();

                  reply.state = this->m_state.xaSwitches->xaSwitch->xa_open_entry( this->m_state.rm_openinfo.c_str(), rm_id, TMNOFLAGS);

                  common::trace::Exit logConnect{ "resource connect to transaction monitor"};

                  this->m_tmQueue( reply);

                  if( reply.state != XA_OK)
                  {
                     throw exception::NotReallySureWhatToNameThisException( "failed to open xa resurce " + this->m_state.rm_key + " with: " + this->m_state.rm_openinfo);
                  }
               }
            };
            template< typename TQ>
            Open< TQ> open( State& state, TQ&& tmQueue)
            {
               return Open< TQ>{ state, std::forward<TQ>( tmQueue)};
            }

            template< typename TQ>
            struct Prepare : public Base< TQ>
            {

               typedef message::transaction::resource::prepare::Request message_type;

               using Base< TQ>::Base;

               void dispatch( message_type& message)
               {
                  //
                  // Note: base is template parameter dependent, we have to use this->.
                  // this-> should never be used in other constructs...
                  //

                  message::transaction::resource::prepare::Reply reply;

                  reply.id.pid = common::process::id();
                  reply.id.queue_id = common::ipc::getReceiveQueue().id();
                  reply.state = this->m_state.xaSwitches->xaSwitch->xa_prepare_entry( &message.xid.xid(), rm_id, TMNOFLAGS);

                  this->m_tmQueue( reply);
               }
            };

            template< typename TQ>
            Prepare< TQ> prepare( State& state, TQ&& tmQueue)
            {
               return Prepare< TQ>{ state, std::forward<TQ>( tmQueue)};
            }

            template< typename TQ>
            struct Commit : public Base< TQ>
            {

               typedef message::transaction::resource::commit::Request message_type;

               using Base< TQ>::Base;

               void dispatch( message_type& message)
               {
                  //
                  // Note: base is template parameter dependent, we have to use this->.
                  // this-> should never be used in other constructs...
                  //

                  message::transaction::resource::commit::Reply reply;

                  reply.id.pid = common::process::id();
                  reply.id.queue_id = common::ipc::getReceiveQueue().id();
                  reply.state = this->m_state.xaSwitches->xaSwitch->xa_commit_entry( &message.xid.xid(), rm_id, TMNOFLAGS);

                  this->m_tmQueue( reply);

               }
            };

            template< typename TQ>
            Commit< TQ> commit( State& state, TQ&& tmQueue)
            {
               return Commit< TQ>{ state, std::forward<TQ>( tmQueue)};
            }

            template< typename TQ>
            struct Rollback : public Base< TQ>
            {
               typedef message::transaction::resource::rollback::Request message_type;

               using Base< TQ>::Base;

               void dispatch( message_type& message)
               {
                  //
                  // Note: base is template parameter dependent, we have to use this->.
                  // this-> should never be used in other constructs...
                  //

                  message::transaction::resource::rollback::Reply reply;

                  reply.id.pid = common::process::id();
                  reply.id.queue_id = common::ipc::getReceiveQueue().id();
                  reply.state = this->m_state.xaSwitches->xaSwitch->xa_rollback_entry( &message.xid.xid(), rm_id, TMNOFLAGS);


                  this->m_tmQueue( reply);

               }
            };

            template< typename TQ>
            Rollback< TQ> rollback( State& state, TQ&& tmQueue)
            {
               return Rollback< TQ>{ state, std::forward<TQ>( tmQueue)};
            }



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

           {
              common::trace::Exit log( "resource proxy open resource");

           }

         }

         void Proxy::start()
         {
            common::logger::debug << "resource proxy start";

            typedef queue::ipc_wrapper< queue::blocking::Writer> writer_type;
            writer_type tm_queue( m_state.tm_queue);

            handle::open( m_state, tm_queue)();

            //
            // prepare message dispatch handlers...
            //

            message::dispatch::Handler handler;

            handler.add( handle::prepare( m_state, tm_queue));
            handler.add( handle::commit( m_state, tm_queue));
            handler.add( handle::rollback( m_state, tm_queue));

            common::queue::blocking::Reader receiveQueue( common::ipc::getReceiveQueue());
            message::dispatch::pump( handler, receiveQueue);

         }
      } // resource


   } // transaction
} // casual





