//!
//! handle.cpp
//!
//! Created on: Jun 1, 2014
//!     Author: Lazan
//!

#include "transaction/manager/handle.h"

#include "transaction/manager/action.h"

#include "common/trace.h"


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
               namespace persistent
               {
                  template< typename R, typename M>
                  void reply( State& state, const M& request, int code, const common::process::Handle& target)
                  {
                     R message;
                     message.process = common::process::handle();
                     message.trid = request.trid;
                     message.state = code;

                     state.persistentReplies.emplace_back( target.queue, std::move( message));
                  }

                  template< typename R, typename M>
                  void reply( State& state, const M& request, int code)
                  {
                     reply< R>( state, request, code, request.process);
                  }

               } // persistent

               template< typename M>
               struct Reply : state::Base
               {
                  using state::Base::Base;

                  template< typename T>
                  void operator () ( const T& request, int code, const common::process::Handle& target)
                  {
                     M message;
                     message.process = common::process::handle();
                     message.trid = request.trid;
                     message.state = code;

                     state::pending::Reply reply{ target.queue, message};

                     action::persistent::Send send{ m_state};

                     if( ! send( reply))
                     {
                        common::log::internal::transaction << "failed to send reply directly to : " << target << " type: " << common::message::type( message) << " transaction: " << message.trid << " - action: pend reply\n";
                        m_state.persistentReplies.push_back( std::move( reply));
                     }

                  }

                  template< typename T>
                  void operator () ( const T& request, int code)
                  {
                     operator()( request, code, request.process);
                  }

               };


               namespace resource
               {

                  namespace persistent
                  {
                     template< typename M, typename F>
                     void request( State& state, Transaction& transaction, F&& filter, long flags = TMNOFLAGS)
                     {
                        M message;
                        message.process = common::process::handle();
                        message.trid = transaction.trid;
                        message.flags = flags;

                        state::pending::Request request{ message};

                        auto resources = common::range::partition(
                           transaction.resources,
                           filter);

                        typedef Transaction::Resource resource_type;

                        common::range::transform(
                           std::get< 0>( resources), request.resources,
                           std::mem_fn( &resource_type::id));


                        state.persistentRequests.push_back( std::move( request));
                     }

                  } // persistent

                  template< typename Q>
                  bool request( State& state, const common::ipc::message::Complete& message, state::resource::Proxy::Instance& instance)
                  {
                     Q queue{ instance.process.queue, state};

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
                        message.process = common::process::handle();
                        message.trid = transaction.trid;
                        message.flags = flags;

                        state::pending::Request request{ message};

                        auto resources = common::range::partition(
                              transaction.resources,
                              Transaction::Resource::state::Filter{ filter});

                        for( auto& resource : std::get< 0>( resources))
                        {
                           auto found = state::find::idle::instance( common::range::make( m_state.resources), resource.id);

                           if( ! found || ! resource::request< Q>( m_state, request.message, *found))
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
                              std::get< 0>( resources),
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
                        state.pendingRequests,
                        state::pending::filter::Request{ message.resource});


                  if( request)
                  {
                     //
                     // We got a pending request for this resource, let's oblige
                     //
                     Q queue{ message.process.queue, state};

                     if( queue.send( request->message))
                     {
                        request->resources.erase(
                           std::find(
                              std::begin( request->resources),
                              std::end( request->resources),
                              message.resource));

                        if( request.first->resources.empty())
                        {
                           state.pendingRequests.erase( request.first);
                        }
                     }
                     else
                     {
                        common::log::warning << "failed to send pending request to resource, although the instance (" << message.process <<  ") reported idle\n";

                        instance::state( state, message, state::resource::Proxy::Instance::State::idle);
                     }
                  }
                  else
                  {
                     instance::state( state, message, state::resource::Proxy::Instance::State::idle);
                  }
               }
            } // instance


            namespace transform
            {
               template< typename R, typename M>
               R message( M&& message)
               {
                  R result;

                  result.process = message.process;
                  result.trid = message.trid;

                  return result;
               }
            } // transform

         } // internal

         namespace resource
         {
            void Involved::dispatch( message_type& message)
            {
               common::trace::Scope trace{ "transaction::handle::resource::Involved", common::log::internal::transaction};

               auto transaction = common::range::find_if(
                     m_state.transactions, find::Transaction( message.trid));

               if( transaction)
               {
                  for( auto& resource : message.resources)
                  {
                     if( common::range::find( m_state.resources, resource))
                     {
                        transaction->resources.push_back( resource);
                     }
                     else
                     {
                        common::log::error << "invalid resource id: " << resource << " - involved with " << message.trid << " - action: discard\n";
                     }
                  }

                  common::range::trim( transaction->resources, common::range::unique( common::range::sort( transaction->resources)));

                  common::log::internal::transaction << "involved: " << *transaction << '\n';

               }
               else
               {
                  //
                  // We assume it's instigated from another domain, and that domain
                  // own's the transaction.
                  // TODO: keep track of domain-id?
                  //
                  //
                  // Note: We don't keep this transaction persistent.
                  //
                  m_state.transactions.emplace_back( message.trid);

                  common::log::internal::transaction << "inter-domain involved trid : " << message.trid << " resources: " << common::range::make( message.resources) << " process: " <<  message.process << '\n';
               }
            }

            namespace reply
            {

               template< typename H>
               void Wrapper< H>::dispatch( message_type& message)
               {

                  //
                  // The instance is done and ready for more work
                  //
                  internal::instance::done< queue::non_blocking::Writer>( this->m_state, message);

                  //
                  // Find the transaction
                  //
                  auto found = common::range::find_if(
                        common::range::make( m_state.transactions), find::Transaction{ message.trid});

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
                        common::log::error << "failed to locate resource: " <<  message.resource  << " for trid: " << message.trid << " - action: discard?\n";
                     }

                  }
                  else
                  {
                     // TODO: what to do? We have previously sent a prepare request, why do we not find the trid?
                     common::log::error << "failed to locate trid: " << message.trid << " - action: discard?\n";
                  }
               }



               void Connect::dispatch( message_type& message)
               {
                  common::trace::Scope trace{ "transaction::handle::resource::connect reply", common::log::internal::transaction};

                  common::log::internal::transaction << "resource (id: " << message.resource << ") " <<  message.process << " connected" << std::endl;

                  auto instanceRange = state::find::instance(
                        common::range::make( m_state.resources),
                        message);

                  if( ! instanceRange.empty())
                  {
                     if( message.state == XA_OK)
                     {
                        instanceRange.first->state = state::resource::Proxy::Instance::State::idle;
                        instanceRange.first->process = std::move( message.process);

                     }
                     else
                     {
                        common::log::error << "resource proxy: " <<  message.process << " startup error" << std::endl;
                        instanceRange.first->state = state::resource::Proxy::Instance::State::startupError;
                        //throw common::exception::signal::Terminate{};
                        // TODO: what to do?
                     }
                  }
                  else
                  {
                     common::log::error << "transaction manager - unexpected resource connecting - process: " << message.process << " - action: discard" << std::endl;
                  }


                  if( ! m_connected && common::range::all_of( common::range::make( m_state.resources), state::filter::Running{}))
                  {
                     //
                     // We now have enough resource proxies up and running to guarantee consistency
                     // notify broker
                     //

                     common::log::internal::transaction << "enough resources are connected - send connect to broker\n";

                     queue::blocking::Writer brokerQueue{ common::ipc::broker::id(), m_state};

                     common::message::transaction::manager::Ready running;
                     running.process = common::process::handle();
                     brokerQueue( running);

                     m_connected = true;
                  }
               }


               bool basic_prepare::dispatch( message_type& message, Transaction& transaction, Transaction::Resource& resource)
               {
                  common::trace::Scope trace{ "transaction::handle::resource::prepare reply", common::log::internal::transaction};

                  common::log::internal::transaction << "prepare reply - from: " << message.process << " rmid: " << message.resource << " result: " << common::error::xa::error( message.state) << '\n';

                  resource.state = Transaction::Resource::State::cPrepareReplied;
                  resource.result = Transaction::Resource::convert( message.state);

                  auto state = transaction.state();

                  //
                  // Are we in a prepared state?
                  //
                  if( state == Transaction::Resource::State::cPrepareReplied)
                  {

                     using reply_type = common::message::transaction::prepare::Reply;

                     //
                     // Normalize all the resources return-state
                     //
                     auto result = transaction.results();

                     switch( result)
                     {
                        case Transaction::Resource::Result::cXA_RDONLY:
                        {
                           common::log::internal::transaction << "prepare completed - " << transaction << " XA_RDONLY\n";

                           //
                           // Read-only optimazation. We can send the reply directly and
                           // discard the transaction
                           //
                           m_state.log.remove( transaction.trid);

                           //
                           // Send reply
                           //
                           internal::send::Reply<
                              reply_type> send{ m_state};

                           send( message, XA_RDONLY, transaction.trid.owner());

                           //
                           // Indicate that wrapper should remove the transaction from state
                           //
                           return true;
                        }
                        case Transaction::Resource::Result::cXA_OK:
                        {
                           common::log::internal::transaction << "prepare completed - " << transaction << " XA_OK\n";

                           //
                           // Prepare has gone ok. Log state
                           //
                           m_state.log.prepareCommit( transaction.trid);

                           //
                           // prepare send reply. Will be sent after persistent write to file
                           //
                           internal::send::persistent::reply< reply_type>( m_state, message, XA_OK, transaction.trid.owner());

                           //
                           // All XA_OK is to be committed, send commit to all
                           //

                           //
                           // We only want to send to resorces that has reported ok, and is in prepared state
                           // (could be that some has read-only)
                           //
                           auto filter = common::chain::And::link(
                                 Transaction::Resource::result::Filter{ Transaction::Resource::Result::cXA_OK},
                                 Transaction::Resource::state::Filter{ Transaction::Resource::State::cPrepareReplied});

                           internal::send::resource::persistent::request<
                              common::message::transaction::resource::commit::Request>( m_state, transaction, filter);


                           break;
                        }
                        default:
                        {
                           //
                           // Something has gone wrong.
                           //
                           common::log::error << "TODO: something has gone wrong - rollback...\n";


                           internal::send::resource::Requests<
                              queue::non_blocking::Writer,
                              common::message::transaction::resource::commit::Request> request{ m_state};

                           request( transaction, Transaction::Resource::State::cPrepareReplied, Transaction::Resource::State::cCommitRequested);

                           break;
                        }
                     }
                  }
                  //
                  // Else we wait for more replies...
                  //
                  return false;
               }


               bool basic_commit::dispatch( message_type& message, Transaction& transaction, Transaction::Resource& resource)
               {
                  common::trace::Scope trace{ "transaction::handle::resource::commit reply", common::log::internal::transaction};

                  common::log::internal::transaction << "commit reply - from: " << message.process << " rmid: " << message.resource << " result: " << common::error::xa::error( message.state) << '\n';

                  resource.state = Transaction::Resource::State::cCommitReplied;

                  auto state = transaction.state();


                  //
                  // Are we in a commited state?
                  //
                  if( state == Transaction::Resource::State::cCommitReplied)
                  {
                     //using non_block_writer = typename queue_policy::non_block_writer;
                     using reply_type = common::message::transaction::commit::Reply;

                     //
                     // Normalize all the resources return-state
                     //
                     auto result = transaction.results();

                     switch( result)
                     {
                        case Transaction::Resource::Result::cXA_OK:
                        {
                           common::log::internal::transaction << "commit completed - " << transaction << " XA_OK\n";

                           //
                           // Send reply
                           // TODO: We can't send reply directly without checking that the prepare-reply
                           // has been sent. For now, we do a delayed reply, so we're sure that the
                           // commit-reply arrives after the prepare-reply
                           /*
                           internal::send::Reply<
                              non_block_writer,
                              reply_type> send{ m_state};

                           send( message, XA_OK, transaction.owner);
                           */
                           internal::send::persistent::reply< reply_type>( m_state, message, XA_OK, transaction.trid.owner());

                           //
                           // Remove transaction
                           //
                           m_state.log.remove( message.trid);
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
                           internal::send::persistent::reply< reply_type>( m_state, message, Transaction::Resource::convert( result), transaction.trid.owner());

                           break;
                        }
                     }
                  }
                  return false;
               }

               bool basic_rollback::dispatch( message_type& message, Transaction& transaction, Transaction::Resource& resource)
               {
                  common::trace::Scope trace{ "transaction::handle::resource::rollback reply", common::log::internal::transaction};

                  common::log::internal::transaction << "rollback reply - from: " << message.process << " rmid: " << message.resource << " result: " << common::error::xa::error( message.state) << '\n';

                  resource.state = Transaction::Resource::State::cRollbackReplied;

                  auto state = transaction.state();


                  //
                  // Are we in a rolled back state?
                  //
                  if( state == Transaction::Resource::State::cRollbackReplied)
                  {

                     using reply_type = common::message::transaction::rollback::Reply;

                     //
                     // Normalize all the resources return-state
                     //
                     auto result = transaction.results();

                     switch( result)
                     {
                        case Transaction::Resource::Result::cXA_OK:
                        {
                           common::log::internal::transaction << "rollback completed - " << transaction << " XA_OK\n";

                           //
                           // Send reply
                           //
                           internal::send::Reply<
                              reply_type> sender{ m_state};

                           sender( message, XA_OK, transaction.trid.owner());


                           //
                           // Remove transaction
                           //
                           m_state.log.remove( message.trid);
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
                           internal::send::persistent::reply< reply_type>( m_state, message, Transaction::Resource::convert( result), transaction.trid.owner());

                           break;
                        }
                     }

                  }


                  return false;
               }
            } // reply
         } // resource


         void Begin::dispatch( message_type& message)
         {
            common::trace::Scope trace{ "transaction::handle::Begin", common::log::internal::transaction};
            ;

            if( ! message.trid)
            {
               message.trid = common::transaction::ID::create( message.process);
            }

            auto found = common::range::find_if( m_state.transactions, find::Transaction{ message.trid});

            if( ! found)
            {

               //
               // Add transaction
               //
               {
                  m_state.log.begin( message);

                  m_state.transactions.emplace_back( message.trid);
               }

               //
               // prepare send reply. Will be sent after persistent write to file
               //
               internal::send::persistent::reply< reply_type>( m_state, message, XA_OK);
            }
            else
            {
               common::log::error << "XAER_DUPID Attempt to start a transaction " << message.trid << ", which is already in progress" << std::endl;

               //
               // Send reply
               //
               internal::send::Reply<
                  reply_type> sender{ m_state};

               sender( message, XAER_DUPID);
            }


         }

         void Commit::dispatch( message_type& message)
         {
            common::trace::Scope trace{ "transaction::handle::Commit", common::log::internal::transaction};

            auto found = common::range::find_if( m_state.transactions, find::Transaction{ message.trid});

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
                     common::log::internal::transaction << "no resources involved - " << transaction << " XA_RDONLY\n";

                     //
                     // We can remove this transaction from the log.
                     //
                     m_state.log.remove( transaction.trid);

                     //
                     // Send reply
                     //
                     internal::send::Reply<
                        common::message::transaction::prepare::Reply> sender{ m_state};

                     sender( message, XA_RDONLY);
                     break;
                  }
                  case 1:
                  {
                     //
                     // Only one resource involved, we do a one-phase-commit optimization.
                     //
                     common::log::internal::transaction << "only one resource involved - " << transaction << " TMONEPHASE\n";


                     internal::send::resource::Requests<
                        queue::non_blocking::Writer,
                        common::message::transaction::resource::commit::Request> request{ m_state};

                     request( transaction, Transaction::Resource::State::cInvolved, Transaction::Resource::State::cCommitRequested, TMONEPHASE);

                     break;
                  }
                  default:
                  {
                     //
                     // More than one resource involved, we do the prepare stage
                     //
                     common::log::internal::transaction << "prepare " << transaction << "\n";


                     internal::send::resource::Requests<
                        queue::non_blocking::Writer,
                        common::message::transaction::resource::prepare::Request> request{ m_state};

                     request( transaction, Transaction::Resource::State::cInvolved, Transaction::Resource::State::cPrepareRequested);

                     break;
                  }
               }

            }
            else
            {
               common::log::error << "XAER_NOTA Attempt to commit transaction " << message.trid << ", which is not known to TM - action: XAER_NOTA reply" << std::endl;

               //
               // Send reply
               //
               internal::send::Reply<
                  reply_type> sender{ m_state};

               sender( message, XAER_NOTA);
            }
         }


         void Rollback::dispatch( message_type& message)
         {
            common::trace::Scope trace{ "transaction::handle::Rollback", common::log::internal::transaction};

            //
            // Find the transaction
            //
            auto found = common::range::find_if( m_state.transactions, find::Transaction{ message.trid});

            if( found)
            {
               auto& transaction = *found;

               if( transaction.resources.empty())
               {
                  common::log::internal::transaction << "no resources involved - " << transaction << " XA_OK\n";

                  //
                  // We can remove this transaction from the log.
                  //
                  m_state.log.remove( transaction.trid);

                  //
                  // Send reply
                  //
                  internal::send::Reply<
                     reply_type> sender{ m_state};

                  sender( message, XA_OK);
               }
               else
               {
                  internal::send::resource::Requests<
                     queue::non_blocking::Writer,
                     common::message::transaction::resource::rollback::Request> request{ m_state};

                  request( transaction, Transaction::Resource::State::cInvolved, Transaction::Resource::State::cRollbackRequested);
               }
            }
            else
            {
               common::log::error << "XAER_NOTA Attempt to rollback a transaction " << message.trid << ", which is not known to TM - action: error reply" << std::endl;

               //
               // Send reply
               //
               internal::send::Reply<
                  reply_type> sender{ m_state};

               sender( message, XAER_NOTA);
            }

         }


         namespace domain
         {

            void Prepare::dispatch( message_type& message)
            {
               common::trace::Scope trace{ "transaction::handle::domain::prepare request", common::log::internal::transaction};

               //
               // Find the transaction
               //
               auto found = common::range::find_if( m_state.transactions, find::Transaction{ message.trid});

               if( found && ! found->resources.empty())
               {
                  auto& transaction = *found;
                  common::log::internal::transaction << "prepare - trid:" << transaction.trid << " owner: " << transaction.trid.owner() << " resources: " << common::range::make( transaction.resources) << "\n";

                  internal::send::resource::Requests<
                     queue::non_blocking::Writer,
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

                  common::log::internal::transaction << "XA_RDONLY transaction (" << message.trid << ") either does not exists (longer) in this domain or there are no resources involved - action: send prepare-reply (read only)\n";

                  //
                  // Send reply
                  //
                  internal::send::Reply<
                     reply_type> sender{ m_state};

                  sender( message, XA_RDONLY);

               }
            }

            void Commit::dispatch( message_type& message)
            {
               common::trace::Scope trace{ "transaction::handle::domain::commit request", common::log::internal::transaction};

               //
               // Find the transaction
               //
               auto found = common::range::find_if( m_state.transactions, find::Transaction{ message.trid});

               if( found)
               {
                  auto& transaction = *found;
                  common::log::internal::transaction << "commit - trid:" << transaction.trid << " owner: " << transaction.trid.owner() << " resources: " << common::range::make( transaction.resources) << "\n";

                  internal::send::resource::Requests<
                     queue::non_blocking::Writer,
                     common::message::transaction::resource::domain::commit::Request> request{ m_state};

                  request( transaction, Transaction::Resource::State::cPrepareReplied, Transaction::Resource::State::cCommitRequested, message.flags);

               }
               else
               {

                  common::log::internal::transaction << "XAER_NOTA trid: " << message.trid << " is not known to this TM - action: send XAER_NOTA reply\n";

                  //
                  // Send reply
                  //
                  internal::send::Reply<
                     reply_type> sender{ m_state};

                  sender( message, XAER_NOTA);
               }
            }

            void Rollback::dispatch( message_type& message)
            {
               common::trace::Scope trace{ "transaction::handle::domain::rollback request", common::log::internal::transaction};

               //
               // Find the transaction
               //
               auto found = common::range::find_if( m_state.transactions, find::Transaction{ message.trid});

               if( found)
               {
                  auto& transaction = *found;
                  common::log::internal::transaction << "rollback - trid:" << transaction.trid << " owner: " << transaction.trid.owner() << " resources: " << common::range::make( transaction.resources) << "\n";

                  internal::send::resource::Requests<
                     queue::non_blocking::Writer,
                     common::message::transaction::resource::domain::commit::Request> request{ m_state};

                  request( transaction,
                        Transaction::Resource::State::cPrepareReplied,
                        Transaction::Resource::State::cRollbackRequested, message.flags);
               }
               else
               {
                  common::log::internal::transaction << "XAER_NOTA trid: " << message.trid << " is not known to this TM - action: send XAER_NOTA reply\n";

                  //
                  // Send reply
                  //
                  internal::send::Reply<
                     reply_type> sender{ m_state};

                  sender( message, XAER_NOTA);
               }
            }

            namespace resource
            {
               namespace reply
               {
                  bool basic_prepare::dispatch(  message_type& message, Transaction& transaction, Transaction::Resource& resource)
                  {
                     common::trace::Scope trace{ "transaction::handle::domain::resource::prepare reply", common::log::internal::transaction};

                     resource.result = Transaction::Resource::convert( message.state);
                     resource.state = Transaction::Resource::State::cPrepareReplied;


                     if( common::range::all_of( transaction.resources,
                           Transaction::Resource::state::Filter{ Transaction::Resource::State::cPrepareReplied}))
                     {

                        //
                        // All resources has replied, we're done with prepare stage.
                        //
                        using reply_type = common::message::transaction::resource::prepare::Reply;

                        auto result = transaction.results();

                        common::log::internal::transaction << "domain prepared: " << transaction.trid << " - state: " << result << std::endl;

                        //
                        // This TM does not own the transaction, so we don't need to store
                        // state.
                        //
                        // Send reply
                        //
                        internal::send::Reply<
                           reply_type> sender{ m_state};

                        sender( message, Transaction::Resource::convert( result));
                     }

                     //
                     // else we wait...
                     return false;
                  }


                  bool basic_commit::dispatch(  message_type& message, Transaction& transaction, Transaction::Resource& resource)
                  {
                     common::trace::Scope trace{ "transaction::handle::domain::resource::commit reply", common::log::internal::transaction};

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

                        auto result = transaction.results();

                        common::log::internal::transaction << "domain committed: " << transaction.trid << " - state: " << result << std::endl;

                        //
                        // This TM does not own the transaction, so we don't need to store
                        // state.
                        //
                        // Send reply
                        //
                        internal::send::Reply<
                           reply_type> sender{ m_state};

                        sender( message, Transaction::Resource::convert( result));
                     }

                     //
                     // else we wait...

                     return false;
                  }

                  bool basic_rollback::dispatch(  message_type& message, Transaction& transaction, Transaction::Resource& resource)
                  {
                     common::trace::Scope trace{ "transaction::handle::domain::resource::rollback reply", common::log::internal::transaction};

                     //using non_block_writer = typename queue_policy::non_block_writer;

                     return false;
                  }



               } // reply
            } // resource
         } // domain



         namespace resource
         {
            namespace reply
            {
               template struct Wrapper< basic_prepare>;
               template struct Wrapper< basic_commit>;
               template struct Wrapper< basic_rollback>;

               template struct Wrapper< handle::domain::resource::reply::basic_prepare>;
               template struct Wrapper< handle::domain::resource::reply::basic_commit>;
               template struct Wrapper< handle::domain::resource::reply::basic_rollback>;


            } // reply

         }


      } // handle
   } // transaction

} // casual
