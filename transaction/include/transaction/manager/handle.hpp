//!
//! handle.hpp
//!
//! Created on: Dec 30, 2013
//!     Author: Lazan
//!

#ifndef HANDLE_HPP_
#define HANDLE_HPP_


#include "transaction/manager/state.h"

#include "common/message.h"
#include "common/internal/trace.h"
#include "common/internal/log.h"

#include "tx.h"

namespace casual
{

   namespace transaction
   {
      namespace handle
      {

         namespace internal
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

               template< typename Q>
               void commitReply( State& state, const Transaction& transaction, int code = XA_OK)
               {
                  common::message::transaction::commit::Reply reply;
                  reply.xid = transaction.xid;
                  reply.id.queue_id = common::ipc::receive::id();

                  Q write{ transaction.owner.queue_id, state};
                  if( ! write( reply))
                  {
                     common::log::internal::transaction << "failed to send reply to owner: " << transaction.owner << " - action: add to pending replies for retries\n";
                     state.pendingReplies.emplace_back( transaction.owner.queue_id, reply);
                  }
               }

               namespace resource
               {
                  template< typename Q, typename M>
                  struct Request : state::Base
                  {
                     using state::Base::Base;

                     bool operator () ( const Transaction::Resource& resource)
                     {

                        /*
                        auto found = state::find::idle::instance(
                              common::range::make( m_state.instances),
                              resource.id);
                         */

                        /*
                        if( ! found.empty())
                        {
                           M message;
                           message.resource = found.first->id;
                           message.id.queue_id = common::ipc::receive::id();
                           message.id.pid = common::process::id();

                           Q write{ found.first->server.queue_id, m_state};

                           if( write( message))
                           {
                              //
                              // Mark resource as busy
                              //
                              found.first->state = state::resource::Proxy::Instance::State::busy;
                              return true;
                           }

                           common::log::internal::transaction << "failed to send request to resource " << resource.id << " proxy: " << found.first->server << "\n";
                        }
                        */
                        return false;

                     }

                  };

               } // resource

            } // send

            namespace check
            {
               template< typename R, typename M>
               bool owner( State& state, const Transaction& transaction, const M& message)
               {
                  if( transaction.owner.pid != message.id.pid)
                  {
                     common::log::error << "TX_PROTOCOL_ERROR - Only owner of the transaction " << transaction.xid
                           <<  " can alter it's state (owner ["
                           << transaction.owner  << "] bad guy: [" << message.id << "]) - action: discard\n";

                     internal::send::reply< R>( state, message.id.queue_id, TX_PROTOCOL_ERROR, transaction.xid);

                     return false;
                  }
                  return true;
               }
            } // check


            namespace transform
            {
               template< typename R, typename M>
               R message( M&& message)
               {
                  R result;

                  result.id = message.id;
                  result.xid = message.xid;

                  return result;
               }
            } // transform

         } // internal


         template< typename B, int error_code>
         void basic_wrapper< B, error_code>::dispatch( message_type& message)
         {
            //
            // Find the transaction
            //
            auto found = common::range::find_if( common::range::make( this->m_state.transactions), find::Transaction{ message.xid});

            if( ! found.empty())
            {
               auto& transaction = *found.first;

               //
               // Only the instigator can fiddle with the transaction
               //
               if( ! internal::check::owner< reply_type>( this->m_state, transaction, message))
                  return;

               //
               // Call the actual implementation
               //
               base_type::dispatch( message, transaction);

            }
            else
            {
               common::log::error << "Attempt to alter state on transaction " << message.xid << ", which is not known to TM - action: error reply" << std::endl;
               internal::send::reply< reply_type>( this->m_state, message.id.queue_id, error_code, message.xid);
            }

         }


         namespace domain
         {
            template< typename QP>
            void basic_prepare< QP>::dispatch( message_type& message, Transaction& transaction)
            {


            }

            template< typename QP>
            void basic_prepare< QP>::invokeResources( Transaction& transaction)
            {
               common::log::internal::transaction << "prepare xid:" << transaction.xid << " owner: " << transaction.owner << " resources: " << common::range::make( transaction.resources) << "\n";

               typedef QP queue_policy;
               using non_block_writer = typename queue_policy::non_block_writer;

               internal::send::resource::Request< non_block_writer, common::message::transaction::resource::prepare::Request> requester{ m_state};


               auto sent = common::range::partition(
                     common::range::make( transaction.resources),
                     requester);



            }

            template< typename QP>
            void basic_commit< QP>::dispatch( message_type& message)
            {

            }

            template< typename QP>
            void basic_rollback< QP>::dispatch( message_type& message)
            {

            }

         } // domain

         template< typename QP>
         void basic_commit< QP>::dispatch( message_type& message, Transaction& transaction)
         {
            common::log::internal::transaction << "commit xid:" << transaction.xid << " owner: " << transaction.owner << "\n";

            typedef QP queue_policy;
            using non_block_writer = typename queue_policy::non_block_writer;

            if( transaction.task != Transaction::Task::waitForCommitOrRollback)
            {

            }

            if( transaction.resources.empty())
            {
               common::log::internal::transaction << "no resources involved for xid: " << transaction.xid << " - action: send reply to owner directly\n";

               //
               // We can remove this transaction from the log.
               //
               m_state.log.remove( transaction.xid);

               //
               // Send reply
               //
               internal::send::commitReply< non_block_writer>( m_state, transaction, XA_OK);
            }
            else
            {

               //
               // prepare. We use the domain prepare.
               //
               typedef domain::basic_prepare< queue_policy> prepare_handler;
               prepare_handler prepare{ m_state};
               prepare.invokeResources( transaction);
            }

         }

      } // handle
   } // transaction

} // casual

#endif // HANDLE_HPP_
