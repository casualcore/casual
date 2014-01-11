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

               template< typename Q, typename M>
               struct Reply : state::Base
               {
                  using state::Base::Base;

                  template< typename T>
                  void operator () ( const T& request, int code)
                  {
                     M message;
                     message.id.queue_id = common::ipc::receive::id();
                     message.xid = request.xid;
                     message.state = code;

                     state::pending::Reply reply{ request.id.queue_id, message};

                     Q queue{ request.id.queue_id, m_state};

                     if( ! queue.send( reply.message))
                     {
                        common::log::internal::transaction << "failed to send reply directly to : " << request.id << " type: " << common::message::type( message) << " transaction: " << message.xid << " - action: pend reply\n";
                        m_state.pendingReplies.push_back( std::move( reply));
                     }
                  }

               };

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

                  template< typename Q>
                  bool request( State& state, const common::ipc::message::Complete& message, state::resource::Proxy::Instance& instance)
                  {
                     Q queue{ instance.server.queue_id, state};

                     if( queue.send( message))
                     {
                        instance.state = state::resource::Proxy::Instance::State::busy;
                        return true;
                     }
                     return false;
                  }


                  template< typename Q, typename M>
                  struct Requests : state::Base
                  {
                     using state::Base::Base;

                     void operator () ( Transaction& transaction, Transaction::Resource::State filter, Transaction::Resource::State newState, long flags = TMNOFLAGS)
                     {
                        M message;
                        message.id.queue_id = common::ipc::receive::id();
                        message.id.pid = common::process::id();
                        message.xid = transaction.xid;
                        message.flags = flags;

                        state::pending::Request request{ message};

                        auto resources = common::range::partition(
                              common::range::make( transaction.resources),
                              Transaction::Resource::state::Filter{ filter});

                        for( auto& resource : resources)
                        {
                           auto found = state::find::idle::instance( common::range::make( m_state.resources), resource.id);

                           if( found.empty() || ! resource::request< Q>( m_state, request.message, *found.first))
                           {
                              //
                              // We could not find an idle resource
                              //
                              request.resources.push_back( resource.id);
                           }
                        }

                        if( ! request.resources.empty())
                        {
                           m_state.pendingRequests.push_back( std::move( request));
                        }

                        //
                        // Update state on transaction-resources
                        //
                        common::range::for_each(
                              resources,
                              Transaction::Resource::state::Update{ newState});
                     }
                  };
               } // resource
            } // send

            namespace instance
            {
               template< typename M>
               void state( State& state, const M& message, state::resource::Proxy::Instance::State newState)
               {
                  auto instance = state::find::instance( common::range::make( state.resources), message);

                  if( ! instance.empty())
                  {
                     instance.first->state = newState;
                  }
               }

               template< typename Q, typename M>
               void done( State& state, M& message)
               {
                  auto request = common::range::find_if(
                        common::range::make( state.pendingRequests),
                        state::pending::filter::Request{ message.resource});


                  if( ! request.empty())
                  {
                     //
                     // We got a pending request for this resource, let's oblige
                     //
                     Q queue{ message.id.queue_id, state};

                     if( queue.send( request.first->message))
                     {
                        request.first->resources.erase(
                           std::find(
                              std::begin( request.first->resources),
                              std::end( request.first->resources),
                              message.resource));

                        if( request.first->resources.empty())
                        {
                           state.pendingRequests.erase( request.first);
                        }
                     }
                     else
                     {
                        common::log::warning << "failed to send pending request to resource, although the instance (" << message.id <<  ") reported idle" << std::endl;

                        instance::state( state, message, state::resource::Proxy::Instance::State::idle);
                     }
                  }
                  else
                  {
                     instance::state( state, message, state::resource::Proxy::Instance::State::idle);
                  }
               }
            } // instance

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

            namespace result
            {
               inline int normalize( int origin)
               {
                  return origin;
               }

               inline int denormalize( int normalized)
               {
                  return normalized;
               }




            } // result

         } // internal




         namespace domain
         {

            template< typename QP>
            void basic_prepare< QP>::dispatch( message_type& message)
            {
               common::trace::internal::Scope trace{ "transaction::handle::domain::prepare request"};

               typedef QP queue_policy;
               using non_block_writer = typename queue_policy::non_block_writer;

               //
               // Find the transaction
               //
               auto found = common::range::find_if( common::range::make( m_state.transactions), find::Transaction{ message.xid});

               if( ! found.empty() && ! found.first->resources.empty())
               {
                  auto& transaction = *found.first;
                  common::log::internal::transaction << "prepare - xid:" << transaction.xid << " owner: " << transaction.owner << " resources: " << common::range::make( transaction.resources) << "\n";

                  internal::send::resource::Requests<
                     non_block_writer,
                     common::message::transaction::resource::domain::prepare::Request> request{ m_state};

                  request( transaction, Transaction::Resource::State::cInvolved, Transaction::Resource::State::cPrepareRequested, message.flags);

               }
               else
               {
                  //
                  // We don't have the transaction. This could be for three reasons:
                  // 1) We had it, but it was already prepared from another domain, and XA
                  // optimization kicked in ("read only") and the transaction was done
                  // 2) casual have made some optimizations.
                  // 3) no resources involved
                  // Either way, we don't have it, and this domain does not own the transaction
                  // so we just reply that it has been prepared with "read only"
                  //

                  common::log::internal::transaction << "XA_RDONLY transaction (" << message.xid << ") either does not exists (longer) in this domain or there are no resources involved - action: send prepare-reply (read only)\n";

                  //
                  // Send reply
                  //
                  internal::send::Reply<
                     non_block_writer,
                     reply_type> sender{ m_state};

                  sender( message, XA_RDONLY);

               }
            }

            template< typename QP>
            void basic_commit< QP>::dispatch( message_type& message)
            {

               common::trace::internal::Scope trace{ "transaction::handle::domain::commit request"};

               typedef QP queue_policy;
               using non_block_writer = typename queue_policy::non_block_writer;

               //
               // Find the transaction
               //
               auto found = common::range::find_if( common::range::make( m_state.transactions), find::Transaction{ message.xid});

               if( ! found.empty())
               {
                  auto& transaction = *found.first;
                  common::log::internal::transaction << "commit - xid:" << transaction.xid << " owner: " << transaction.owner << " resources: " << common::range::make( transaction.resources) << "\n";

                  internal::send::resource::Requests<
                     non_block_writer,
                     common::message::transaction::resource::domain::commit::Request> request{ m_state};

                  request( transaction, Transaction::Resource::State::cPrepareReplied, Transaction::Resource::State::cCommitRequested, message.flags);

               }
               else
               {

                  common::log::internal::transaction << "XAER_NOTA xid: " << message.xid << " is not known to this TM - action: send XAER_NOTA reply\n";

                  //
                  // Send reply
                  //
                  internal::send::Reply<
                     non_block_writer,
                     reply_type> sender{ m_state};

                  sender( message, XAER_NOTA);
               }
            }

            template< typename QP>
            void basic_rollback< QP>::dispatch( message_type& message)
            {
               common::trace::internal::Scope trace{ "transaction::handle::domain::rollback request"};

               typedef QP queue_policy;
               using non_block_writer = typename queue_policy::non_block_writer;

               //
               // Find the transaction
               //
               auto found = common::range::find_if( common::range::make( m_state.transactions), find::Transaction{ message.xid});

               if( ! found.empty())
               {
                  auto& transaction = *found.first;
                  common::log::internal::transaction << "rollback - xid:" << transaction.xid << " owner: " << transaction.owner << " resources: " << common::range::make( transaction.resources) << "\n";

                  internal::send::resource::Requests<
                     non_block_writer,
                     common::message::transaction::resource::domain::commit::Request> request{ m_state};

                  request( transaction,
                        Transaction::Resource::State::cPrepareReplied,
                        Transaction::Resource::State::cRollbackRequested, message.flags);

               }
               else
               {

                  common::log::internal::transaction << "XAER_NOTA xid: " << message.xid << " is not known to this TM - action: send XAER_NOTA reply\n";

                  //
                  // Send reply
                  //
                  internal::send::Reply<
                     non_block_writer,
                     reply_type> sender{ m_state};

                  sender( message, XAER_NOTA);
               }
            }

            namespace resource
            {


               template< typename B>
               void basic_wrapper< B>::dispatch( message_type& message)
               {

                  using non_block_writer = typename queue_policy::non_block_writer;

                  //
                  // The instance is done and ready for more work
                  //
                  internal::instance::done< non_block_writer>( this->m_state, message);

                  //
                  // Find the transaction
                  //
                  auto found = common::range::find_if(
                        common::range::make( this->m_state.transactions), find::Transaction{ message.xid});

                  if( ! found.empty())
                  {
                     auto& transaction = *found.first;

                     auto resource = common::range::find_if(
                           common::range::make( transaction.resources),
                           Transaction::Resource::id::Filter{ message.resource});

                     if( ! resource.empty())
                     {
                        //
                        // We found all the stuff, let sub-class take care of business
                        //
                        base_type::dispatch( message, transaction, *resource.first);
                     }
                     else
                     {
                        // TODO: what to do? We have previously sent a prepare request, why do we not find the resource?
                        common::log::error << "failed to locate resource: " <<  message.resource  << " for xid: " << message.xid << " - action: discard?\n";
                     }

                  }
                  else
                  {
                     // TODO: what to do? We have previously sent a prepare request, why do we not find the xid?
                     common::log::error << "failed to locate xid: " << message.xid << " - action: discard?\n";
                  }


               }



               template< typename QP>
               void basic_prepare< QP>::dispatch(  message_type& message, Transaction& transaction, Transaction::Resource& resource)
               {
                  common::trace::internal::Scope trace{ "transaction::handle::domain::resource::prepare reply"};

                  resource.result = internal::result::normalize( message.state);
                  resource.state = Transaction::Resource::State::cPrepareReplied;

                  const auto state = transaction.state();


                  if( common::range::all_of( common::range::make( transaction.resources),
                        Transaction::Resource::state::Filter{ Transaction::Resource::State::cPrepareReplied}))
                  {
                     auto results = transaction.results();



                  }


                  //
                  // else we wait...
               }

               template< typename QP>
               void basic_commit< QP>::dispatch(  message_type& message, Transaction& transaction, Transaction::Resource& resource)
               {
                  common::trace::internal::Scope trace{ "transaction::handle::domain::resource::commit reply"};


                  using non_block_writer = typename queue_policy::non_block_writer;



               }

               template< typename QP>
               void basic_rollback< QP>::dispatch(  message_type& message, Transaction& transaction, Transaction::Resource& resource)
               {
                  common::trace::internal::Scope trace{ "transaction::handle::domain::resource::rollback reply"};

                  using non_block_writer = typename queue_policy::non_block_writer;



               }


            } // resource

         } // domain


         namespace resource
         {
            template< typename QP>
            void basic_connect< QP>::dispatch( message_type& message)
            {
               common::trace::internal::Scope trace{ "transaction::handle::resource::connect reply"};

               common::log::internal::transaction << "resource (id: " << message.resource << ") " <<  message.id << " connected" << std::endl;

               auto instanceRange = state::find::instance(
                     common::range::make( m_state.resources),
                     message);

               if( ! instanceRange.empty())
               {
                  if( message.state == XA_OK)
                  {
                     instanceRange.first->state = state::resource::Proxy::Instance::State::idle;
                     instanceRange.first->server = std::move( message.id);

                  }
                  else
                  {
                     common::log::error << "resource proxy pid: " <<  message.id.pid << " startup error" << std::endl;
                     instanceRange.first->state = state::resource::Proxy::Instance::State::startupError;
                     //throw common::exception::signal::Terminate{};
                     // TODO: what to do?
                  }
               }
               else
               {
                  common::log::error << "transaction manager - unexpected resource connecting - process: " << message.id << " - action: discard" << std::endl;
               }


               if( ! m_connected && common::range::all_of( common::range::make( m_state.resources), state::filter::Running{}))
               {
                  //
                  // We now have enough resource proxies up and running to guarantee consistency
                  // notify broker
                  //

                  common::log::internal::transaction << "enough resources are connected - send connect to broker\n";

                  typedef QP queue_policy;
                  using block_writer = typename queue_policy::block_writer;
                  block_writer brokerQueue{ m_brokerQueueId, m_state};

                  common::message::transaction::Connected running;
                  brokerQueue( running);

                  m_connected = true;
               }
            }

         } // resource


         template< typename QP>
         void basic_commit< QP>::dispatch( message_type& message)
         {
            common::trace::internal::Scope trace{ "transaction::handle::commit request"};

            typedef QP queue_policy;
            using non_block_writer = typename queue_policy::non_block_writer;

            auto found = common::range::find_if( common::range::make( m_state.transactions), find::Transaction{ message.xid});

            if( ! found.empty())
            {
               auto& transaction = *found.first;

               //
               // Only the instigator can fiddle with the transaction
               //
               //if( ! internal::check::owner< reply_type>( this->m_state, transaction, message))
               //   return;



               if( transaction.task != Transaction::Task::waitForCommitOrRollback)
               {
                  // TODO
               }

               switch( transaction.resources.size())
               {
                  case 0:
                  {
                     common::log::internal::transaction << "XA_RDONLY no resources involved for xid: " << transaction.xid << " - action: send reply to owner directly\n";

                     //
                     // We can remove this transaction from the log.
                     //
                     m_state.log.remove( transaction.xid);

                     //
                     // Send reply
                     //
                     internal::send::Reply<
                        non_block_writer,
                        common::message::transaction::resource::prepare::Reply> sender{ m_state};

                     sender( message, XA_RDONLY);
                     break;
                  }
                  case 1:
                  {
                     //
                     // Only one resource involved, we do a one-phase-commit optimization.
                     //
                     common::log::internal::transaction << "TMONEPHASE only one resource involved for xid: " << transaction.xid << " - action: one-phase-commit\n";


                     internal::send::resource::Requests<
                        non_block_writer,
                        common::message::transaction::resource::commit::Request> request{ m_state};

                     request( transaction, Transaction::Resource::State::cInvolved, Transaction::Resource::State::cCommitRequested, TMONEPHASE);

                     break;
                  }
                  default:
                  {
                     //
                     // More than one resource involved, we do the prepare stage
                     //
                     common::log::internal::transaction << "prepare xid:" << transaction.xid << " owner: " << transaction.owner << "\n";


                     internal::send::resource::Requests<
                        non_block_writer,
                        common::message::transaction::resource::prepare::Request> request{ m_state};

                     request( transaction, Transaction::Resource::State::cInvolved, Transaction::Resource::State::cPrepareRequested);

                     break;
                  }
               }

            }
            else
            {
               common::log::error << "XAER_NOTA Attempt to commit transaction " << message.xid << ", which is not known to TM - action: XAER_NOTA reply" << std::endl;

               //
               // Send reply
               //
               internal::send::Reply<
                  non_block_writer,
                  common::message::transaction::resource::prepare::Reply> sender{ m_state};

               sender( message, XAER_NOTA);
            }
         }

      } // handle
   } // transaction

} // casual

#endif // HANDLE_HPP_
