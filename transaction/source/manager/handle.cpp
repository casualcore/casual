//!
//! handle.cpp
//!
//! Created on: Nov 1, 2013
//!     Author: Lazan
//!

#include "transaction/manager/handle.h"


#include "common/algorithm.h"


namespace casual
{

   namespace transaction
   {
      namespace handle
      {

         namespace resource
         {

            namespace instance
            {

               template< typename M>
               void state( State& state, const M& message, state::resource::Proxy::Instance::State newState)
               {
                  auto instance = state::find::instance( common::range::make( state.instances), message);

                  if( ! instance.empty())
                  {
                     instance.first->state = newState;
                  }
               }

               template< typename M>
               void done( State& state, M& message)
               {
                  /*
                  auto request = common::range::find(
                        common::range::make( state.pendingRequest),
                        action::pending::resource::Request::Find{ message.resource});

                  if( ! request.empty())
                  {
                     //
                     // We got a pending request for this resource, let's oblige
                     //
                     queue::non_blocking::Writer writer{ message.id.queue_id, state};

                     if( writer.send( request.first->message))
                     {
                        common::range::erase( state.pendingRequest, request);
                     }
                     else
                     {
                        common::log::warning << "failed to send pending request to resource, although the instance reported idle" << std::endl;

                        instance::state( state, message, state::resource::Proxy::Instance::State::idle);
                     }
                  }
                  else
                  {

                     instance::state( state, message, state::resource::Proxy::Instance::State::idle);

                     // TODO: else what?
                  }
                  */

               }
            } // instance




            void Prepare::dispatch( message_type& message)
            {

               //
               // Instance is ready for more work
               //
               instance::done( m_state, message);

               auto found = common::range::find_if(
                     common::range::make( m_state.transactions),
                     find::Transaction{ message.xid});

               if( ! found.empty())
               {
                  auto& transaction = *found.first;

                  auto resource = common::range::find_if(
                        common::range::make( transaction.resources),
                        find::Transaction::Resource{ message.resource});

                  if( ! resource.empty())
                  {
                     resource.first->state = Transaction::Resource::State::cPrepared;
                  }
                  else
                  {
                     common::log::error << "Resource [" << message.resource << "]Êclaims to have prepared transaction: "  << message.xid.stringGlobal() << ", TM thinks the resource is not involved - action: discard" << std::endl;
                  }


                  auto state = transaction.state();

                  //
                  // Are we in a prepared state?
                  //
                  if( state >= Transaction::Resource::State::cPrepared)
                  {
                     m_state.log.prepareCommit( transaction.xid);
                  }

               }
               else
               {
                  common::log::error << "Resource [" << message.resource << "]Êclaims to have prepared transaction"  << message.xid.stringGlobal() << ", which is not known to TM - action: discard" << std::endl;
               }
            }

            void Commit::dispatch( message_type& message)
            {
               //
               // Instance is ready for more work
               //
               instance::done( m_state, message);

            }

            void Rollback::dispatch( message_type& message)
            {
               //
               // Instance is ready for more work
               //
               instance::done( m_state, message);
            }


         } // resource


         namespace local
         {
            namespace
            {
               namespace send
               {
                  template< typename M>
                  void reply( State& state, common::platform::queue_id_type target, int code, const common::transaction::ID& xid)
                  {
                     M message;
                     message.id.queue_id = common::ipc::receive::id();
                     message.xid = xid;
                     message.state = code;

                     state.pendingReplies.emplace_back( target, message);
                  }
               } // send



            } //


         } // local


         void Begin::dispatch( message_type& message)
         {
            typedef common::message::transaction::begin::Reply reply_type;

            if( message.xid.null())
            {
               message.xid.generate();
            }

            auto found = common::range::find_if( common::range::make( m_state.transactions), find::Transaction{ message.xid});

            if( found.empty())
            {

               m_state.transactions.push_back( transform::Transaction()( message));
               auto& transaction = m_state.transactions.back();

               assert( transaction.tasks.front() == Transaction::Task::logBegin);
               m_state.log.begin( message);
               transaction.tasks.pop_front();


               assert( transaction.tasks.front() == Transaction::Task::replyBegin);
               m_state.pendingReplies.push_back( transform::pending::reply< reply_type>( transaction, message.id.queue_id));
               transaction.tasks.pop_front();

            }
            else
            {
               common::log::error << "Attempt to start a transaction " << message.xid.stringGlobal() << ", which is already in progress" << std::endl;

               typedef common::message::transaction::rollback::Reply reply_type;
               local::send::reply< reply_type>( m_state, message.id.queue_id, TX_PROTOCOL_ERROR, message.xid);
            }


         }


         void Commit::dispatch( message_type& message)
         {
            //
            // Find the transaction
            //
            auto transaction = common::range::find_if( common::range::make( m_state.transactions), find::Transaction{ message.xid});

            if( ! transaction.empty())
            {
               if( transaction.first->tasks.front() != Transaction::Task::waitForCommitOrRollback)
               {

               }


               //
               // Prepare prepare-requests
               //
               //action::pending::transform::Request< common::message::transaction::resource::prepare::Request> transform{ message.xid};



            }
            else
            {
               common::log::error << "Attempt to commit a transaction " << message.xid.stringGlobal() << ", which is not known to TM - action: error reply" << std::endl;

               typedef common::message::transaction::commit::Reply reply_type;
               local::send::reply< reply_type>( m_state, message.id.queue_id, TX_PROTOCOL_ERROR, message.xid);
            }
         }

         void Rollback::dispatch( message_type& message)
         {
            //
            // Find the transaction
            //
            auto transaction = common::range::find_if( common::range::make( m_state.transactions), find::Transaction{ message.xid});

            if( ! transaction.empty())
            {
               if( transaction.first->tasks.front() != Transaction::Task::waitForCommitOrRollback)
               {

               }


               //
               // Prepare prepare-requests
               //
               //action::pending::transform::Request< common::message::transaction::resource::prepare::Request> transform{ message.xid};



            }
            else
            {
               common::log::error << "Attempt to rollback a transaction " << message.xid.stringGlobal() << ", which is not known to TM - action: error reply" << std::endl;

               typedef common::message::transaction::rollback::Reply reply_type;
               local::send::reply< reply_type>( m_state, message.id.queue_id, TX_PROTOCOL_ERROR, message.xid);
            }

         }


         void Involved::dispatch( message_type& message)
         {
            auto transcation = common::range::find_if( common::range::make( m_state.transactions), find::Transaction( message.xid));


            if( ! transcation.empty())
            {
               common::range::copy(
                  common::range::make( message.resources),
                  std::back_inserter( transcation.first->resources));

               common::range::trim( transcation.first->resources, common::range::unique( common::range::sort( common::range::make( transcation.first->resources))));
            }
            else
            {
               common::log::error << "resource " << common::range::make( message.resources) << " (process " << message.id << ") claims to be involved in transaction " << message.xid.stringGlobal() << ", which is not known to TM - action: discard" << std::endl;
            }
         }


      } // handle
   } // transaction



} // casual
