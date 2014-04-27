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
#include "xa.h"

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
               namespace delayed
               {
                  template< typename R, typename M>
                  void reply( State& state, const M& request, int code, const common::message::server::Id& target)
                  {
                     R message;
                     message.id.queue_id = common::ipc::receive::id();
                     message.xid = request.xid;
                     message.state = code;

                     state.pendingReplies.emplace_back( target.queue_id, message);
                  }

                  template< typename R, typename M>
                  void reply( State& state, const M& request, int code)
                  {
                     reply< R>( state, request, code, request.id);
                  }

               } // delayed

               template< typename Q, typename M, typename Enable = void>
               struct Reply;

               template< typename Q, typename M>
               struct Reply< Q, M, typename std::enable_if< ! common::queue::is_blocking< Q>::value>::type> : state::Base
               {
                  using state::Base::Base;

                  template< typename T>
                  void operator () ( const T& request, int code, const common::message::server::Id& target)
                  {
                     M message;
                     message.id.queue_id = common::ipc::receive::id();
                     message.xid = request.xid;
                     message.state = code;

                     state::pending::Reply reply{ target.queue_id, message};

                     Q queue{ target.queue_id, m_state};

                     if( ! queue.send( reply.message))
                     {
                        common::log::internal::transaction << "failed to send reply directly to : " << target << " type: " << common::message::type( message) << " transaction: " << message.xid << " - action: pend reply\n";
                        m_state.pendingReplies.push_back( std::move( reply));
                     }
                  }

                  template< typename T>
                  void operator () ( const T& request, int code)
                  {
                     operator()( request, code, request.id);
                  }

               };

               template< typename Q, typename M>
               struct Reply< Q, M, typename std::enable_if< common::queue::is_blocking< Q>::value>::type> : state::Base
               {
                  using state::Base::Base;

                  template< typename T>
                  void operator () ( const T& request, int code, const common::message::server::Id& target)
                  {
                     M message;
                     message.id.queue_id = common::ipc::receive::id();
                     message.xid = request.xid;
                     message.state = code;

                     Q queue{ target.queue_id, m_state};

                     queue( message);
                  }

                  template< typename T>
                  void operator () ( const T& request, int code)
                  {
                     operator()( request, code, request.id);
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

            /*
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
            */


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

               inline int normalize( const std::vector< Transaction::Resource>& resources)
               {
                  //
                  // We start with read-only
                  //
                  int result = XA_RDONLY;



                  return result;
               }




            } // result

         } // internal







         namespace resource
         {
            inline void Involved::dispatch( message_type& message)
            {
               common::trace::internal::Scope trace{ "transaction::handle::resource::Involved"};

               auto transcation = common::range::find_if(
                     common::range::make( m_state.transactions), find::Transaction( message.xid));

               if( ! transcation.empty())
               {
                  common::log::internal::transaction << "involved xid : " << message.xid << " resources: " << common::range::make( message.resources) << " process: " <<  message.id << std::endl;

                  common::range::copy(
                     common::range::make( message.resources),
                     std::back_inserter( transcation.first->resources));

                  common::range::trim( transcation.first->resources, common::range::unique( common::range::sort( common::range::make( transcation.first->resources))));
               }
               else
               {
                  //
                  // We assume it's instigated from another domain, and that domain
                  // own's the transaction.
                  // TODO: keep track of domain-id?
                  //
                  // We indicate that this TM is owner of the transaction, for the time being...
                  //
                  // Note: We don't keep this transaction persistent.
                  //
                  m_state.transactions.emplace_back( common::message::server::Id::current(), message.xid);

                  common::log::internal::transaction << "inter-domain involved xid : " << message.xid << " resources: " << common::range::make( message.resources) << " process: " <<  message.id << std::endl;
               }
            }

            namespace reply
            {

               template< typename QP, typename H>
               void Wrapper< QP, H>::dispatch( message_type& message)
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
                        common::range::make( m_state.transactions), find::Transaction{ message.xid});

                  if( ! found.empty())
                  {
                     auto& transaction = *found.first;

                     auto resource = common::range::find_if(
                           common::range::make( transaction.resources),
                           Transaction::Resource::id::Filter{ message.resource});

                     if( ! resource.empty())
                     {
                        //
                        // We found all the stuff, let the real handler handle the message
                        //
                        if( m_handler.dispatch( message, transaction, *resource.first))
                        {
                           //
                           // We remove the transaction from our state
                           //
                           m_state.transactions.erase( found.first);
                        }
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

                     using block_writer = typename queue_policy::block_writer;
                     block_writer brokerQueue{ m_brokerQueueId, m_state};

                     common::message::transaction::Connected running;
                     brokerQueue( running);

                     m_connected = true;
                  }
               }


               template< typename QP>
               bool basic_prepare< QP>::dispatch( message_type& message, Transaction& transaction, Transaction::Resource& resource)
               {
                  common::trace::internal::Scope trace{ "transaction::handle::resource::prepare reply"};

                  resource.state = Transaction::Resource::State::cPrepareReplied;
                  resource.result = Transaction::Resource::convert( message.state);

                  auto state = transaction.state();

                  //
                  // Are we in a prepared state?
                  //
                  if( state == Transaction::Resource::State::cPrepareReplied)
                  {

                     using non_block_writer = typename queue_policy::non_block_writer;
                     using reply_type = common::message::transaction::prepare::Reply;

                     //
                     // Normalize all the resources return-state
                     //
                     auto result = transaction.results();

                     switch( result)
                     {
                        case Transaction::Resource::Result::cXA_RDONLY:
                        {
                           common::log::internal::transaction << "prepare reply - xid: " << transaction.xid << " read only\n";

                           //
                           // Read-only optimazation. We can send the reply directly and
                           // discard the transaction
                           //
                           m_state.log.remove( transaction.xid);

                           //
                           // Send reply
                           //
                           internal::send::Reply<
                              non_block_writer,
                              reply_type> send{ m_state};

                           send( message, XA_RDONLY, transaction.owner);

                           //
                           // Indicate that wrapper should remove the transaction from state
                           //
                           return true;
                        }
                        case Transaction::Resource::Result::cXA_OK:
                        {
                           common::log::internal::transaction << "prepare reply - xid: " << transaction.xid << " read only\n";

                           //
                           // Prepare has gone ok. Log state
                           //
                           m_state.log.prepareCommit( transaction.xid);

                           //
                           // prepare send reply. Will be sent after persistent write to file
                           //
                           internal::send::delayed::reply< reply_type>( m_state, message, XA_OK, transaction.owner);

                           //
                           // All XA_OK is to be committed.
                           //
                           auto resources = common::range::partition(
                              common::range::make( transaction.resources),
                              Transaction::Resource::result::Filter{ Transaction::Resource::Result::cXA_OK});

                           break;
                        }
                        default:
                        {
                           //
                           // Something has gone wrong.
                           //
                           common::log::internal::debug << "TODO: something has gone wrong...\n";


                           break;
                        }
                     }
                  }
                  //
                  // Else we wait for more replies...
                  //
                  return false;
               }

               template< typename QP>
               bool basic_commit< QP>::dispatch( message_type& message, Transaction& transaction, Transaction::Resource& resource)
               {
                  common::trace::internal::Scope trace{ "transaction::handle::resource::commit reply"};

                  resource.state = Transaction::Resource::State::cCommitReplied;

                  auto state = transaction.state();


                  //
                  // Are we in a commited state?
                  //
                  if( state == Transaction::Resource::State::cCommitReplied)
                  {
                     using non_block_writer = typename queue_policy::non_block_writer;
                     using reply_type = common::message::transaction::commit::Reply;

                     //
                     // Normalize all the resources return-state
                     //
                     auto result = transaction.results();

                     switch( result)
                     {
                        case Transaction::Resource::Result::cXA_OK:
                        {

                           //
                           // Send reply
                           //
                           internal::send::Reply<
                              non_block_writer,
                              reply_type> send{ m_state};

                           send( message, XA_OK, transaction.owner);

                           //
                           // Remove transaction
                           //
                           m_state.log.remove( message.xid);
                           return true;

                           break;
                        }
                        default:
                        {
                           //
                           // Something has gone wrong.
                           //
                           common::log::error << "TODO: something has gone wrong...\n";

                           //
                           // prepare send reply. Will be sent after persistent write to file
                           //
                           internal::send::delayed::reply< reply_type>( m_state, message, Transaction::Resource::convert( result), transaction.owner);

                           break;
                        }
                     }
                  }
                  return false;
               }

               template< typename QP>
               bool basic_rollback< QP>::dispatch( message_type& message, Transaction& transaction, Transaction::Resource& resource)
               {
                  common::trace::internal::Scope trace{ "transaction::handle::resource::rollback reply"};


                  return false;
               }


            } // reply

         } // resource


         template< typename QP>
         void basic_begin< QP> ::dispatch( message_type& message)
         {
            common::trace::internal::Scope trace{ "transaction::handle::begin request"};

            using non_block_writer = typename queue_policy::non_block_writer;

            if( message.xid.null())
            {
               message.xid.generate();
            }

            auto found = common::range::find_if( m_state.transactions, find::Transaction{ message.xid});

            if( ! found)
            {

               //
               // Add transaction
               //
               {
                  m_state.log.begin( message);

                  m_state.transactions.emplace_back( message.id, message.xid);
               }



               //
               // prepare send reply. Will be sent after persistent write to file
               //
               internal::send::delayed::reply< reply_type>( m_state, message, XA_OK);
            }
            else
            {
               common::log::error << "XAER_DUPID Attempt to start a transaction " << message.xid << ", which is already in progress" << std::endl;

               //
               // Send reply
               //
               internal::send::Reply<
                  non_block_writer,
                  reply_type> sender{ m_state};

               sender( message, XAER_DUPID);
            }


         }


         template< typename QP>
         void basic_commit< QP>::dispatch( message_type& message)
         {
            common::trace::internal::Scope trace{ "transaction::handle::commit request"};

            using non_block_writer = typename queue_policy::non_block_writer;

            auto found = common::range::find_if( m_state.transactions, find::Transaction{ message.xid});

            if( found)
            {
               auto& transaction = *found;

               //
               // Only the instigator can fiddle with the transaction
               //
               //if( ! internal::check::owner< reply_type>( this->m_state, transaction, message))
               //   return;

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
                        common::message::transaction::prepare::Reply> sender{ m_state};

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
                  reply_type> sender{ m_state};

               sender( message, XAER_NOTA);
            }
         }


         template< typename QP>
         void basic_rollback< QP>::dispatch( message_type& message)
         {
            using non_block_writer = typename queue_policy::non_block_writer;

            //
            // Find the transaction
            //
            auto found = common::range::find_if( m_state.transactions, find::Transaction{ message.xid});

            if( found)
            {
               auto& transaction = *found;

               internal::send::resource::Requests<
                  non_block_writer,
                  common::message::transaction::resource::rollback::Request> request{ m_state};

               request( transaction, Transaction::Resource::State::cInvolved, Transaction::Resource::State::cRollbackRequested);

            }
            else
            {
               common::log::error << "Attempt to rollback a transaction " << message.xid.stringGlobal() << ", which is not known to TM - action: error reply" << std::endl;

               //
               // Send reply
               //
               internal::send::Reply<
                  non_block_writer,
                  reply_type> sender{ m_state};

               sender( message, XAER_NOTA);
            }

         }


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
               auto found = common::range::find_if( m_state.transactions, find::Transaction{ message.xid});

               if( found && ! found->resources.empty())
               {
                  auto& transaction = *found;
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
               auto found = common::range::find_if( m_state.transactions, find::Transaction{ message.xid});

               if( found)
               {
                  auto& transaction = *found;
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
               auto found = common::range::find_if( m_state.transactions, find::Transaction{ message.xid});

               if( found)
               {
                  auto& transaction = *found;
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
               namespace reply
               {

                  template< typename QP>
                  bool basic_prepare< QP>::dispatch(  message_type& message, Transaction& transaction, Transaction::Resource& resource)
                  {
                     common::trace::internal::Scope trace{ "transaction::handle::domain::resource::prepare reply"};

                     resource.result = Transaction::Resource::convert( message.state);
                     resource.state = Transaction::Resource::State::cPrepareReplied;


                     if( common::range::all_of( transaction.resources,
                           Transaction::Resource::state::Filter{ Transaction::Resource::State::cPrepareReplied}))
                     {

                        //
                        // All resources has replied, we're done with prepare stage.
                        //
                        using reply_type = common::message::transaction::resource::prepare::Reply;
                        using sender_type = typename queue_policy::block_writer;


                        auto result = transaction.results();

                        common::log::internal::transaction << "domain prepared: " << transaction.xid << " - state: " << result << std::endl;

                        //
                        // This TM does not own the transaction, so we don't need to store
                        // state.
                        //
                        // Send reply
                        //
                        internal::send::Reply<
                           sender_type,
                           reply_type> sender{ m_state};

                        sender( message, Transaction::Resource::convert( result));
                     }

                     //
                     // else we wait...
                     return false;
                  }

                  template< typename QP>
                  bool basic_commit< QP>::dispatch(  message_type& message, Transaction& transaction, Transaction::Resource& resource)
                  {
                     common::trace::internal::Scope trace{ "transaction::handle::domain::resource::commit reply"};

                     resource.result = Transaction::Resource::convert( message.state);
                     resource.state = Transaction::Resource::State::cCommitReplied;


                     if( common::range::all_of( transaction.resources,
                           Transaction::Resource::state::Filter{ Transaction::Resource::State::cCommitReplied}))
                     {

                        //
                        // All resources has committed, we're done with commit stage.
                        // We can remove the transaction.
                        //



                        using reply_type = common::message::transaction::resource::commit::Reply;
                        using sender_type = typename queue_policy::block_writer;


                        auto result = transaction.results();

                        common::log::internal::transaction << "domain committed: " << transaction.xid << " - state: " << result << std::endl;

                        //
                        // This TM does not own the transaction, so we don't need to store
                        // state.
                        //
                        // Send reply
                        //
                        internal::send::Reply<
                           sender_type,
                           reply_type> sender{ m_state};

                        sender( message, Transaction::Resource::convert( result));
                     }

                     //
                     // else we wait...

                     return false;
                  }

                  template< typename QP>
                  bool basic_rollback< QP>::dispatch(  message_type& message, Transaction& transaction, Transaction::Resource& resource)
                  {
                     common::trace::internal::Scope trace{ "transaction::handle::domain::resource::rollback reply"};

                     //using non_block_writer = typename queue_policy::non_block_writer;

                     return false;
                  }
               } // reply
            } // resource
         } // domain


      } // handle
   } // transaction

} // casual

#endif // HANDLE_HPP_
