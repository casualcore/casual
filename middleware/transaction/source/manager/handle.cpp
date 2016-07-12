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
      namespace ipc
      {

         const common::communication::ipc::Helper& device()
         {
            static common::communication::ipc::Helper ipc{
               common::communication::error::handler::callback::on::Terminate
               {
                  []( const common::process::lifetime::Exit& exit){
                     //
                     // We put a dead process event on our own ipc device, that
                     // will be handled later on.
                     //
                     common::message::domain::process::termination::Event event{ exit};
                     common::communication::ipc::inbound::device().push( std::move( event));
                  }
               }
            };
            return ipc;

         }

      } // ipc

      namespace handle
      {
         namespace local
         {
            namespace
            {

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


                  template< typename M>
                  auto reply( M&& message) -> decltype( common::message::reverse::type( message))
                  {
                     auto result = common::message::reverse::type( message);

                     result.process = common::process::handle();
                     result.trid = message.trid;

                     return result;
                  }

               } // transform

               namespace send
               {
                  namespace persistent
                  {

                     template< typename R, typename M>
                     void reply( State& state, const M& request, int code, const common::process::Handle& target)
                     {
                        R message;
                        message.correlation = request.correlation;
                        message.process = common::process::handle();
                        message.trid = request.trid;
                        message.state = code;

                        state.persistent.replies.emplace_back( target.queue, std::move( message));
                     }

                     template< typename R, typename M>
                     void reply( State& state, const M& request, int code)
                     {
                        reply< R>( state, request, code, request.process);
                     }

                     template< typename M>
                     void reply( State& state, M&& message, const common::process::Handle& target)
                     {
                        state.persistent.replies.emplace_back( target.queue, std::move( message));
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
                        message.correlation = request.correlation;
                        message.process = common::process::handle();
                        message.trid = request.trid;
                        message.state = code;

                        state::pending::Reply reply{ target.queue, message};

                        action::persistent::Send send{ m_state};

                        if( ! send( reply))
                        {
                           common::log::internal::transaction << "failed to send reply directly to : " << target << " type: " << common::message::type( message) << " transaction: " << message.trid << " - action: pend reply\n";
                           m_state.persistent.replies.push_back( std::move( reply));
                        }

                     }

                     template< typename T>
                     void operator () ( const T& request, int code)
                     {
                        operator()( request, code, request.process);
                     }

                  };


                  template< typename R>
                  void reply( State& state, R&& message, const common::process::Handle& target)
                  {
                     state::pending::Reply reply{ target.queue, message};

                     action::persistent::Send send{ state};

                     if( ! send( reply))
                     {
                        common::log::internal::transaction << "failed to send reply directly to : " << target << " type: " << common::message::type( message) << " transaction: " << message.trid << " - action: pend reply\n";
                        state.persistent.replies.push_back( std::move( reply));
                     }
                  }


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


                           state.persistent.requests.push_back( std::move( request));
                        }

                     } // persistent


                     template< typename M, typename F>
                     void request( State& state, Transaction& transaction, F&& filter, Transaction::Resource::Stage newStage, long flags = TMNOFLAGS)
                     {
                        common::trace::Scope trace{ "transaction::handle::send::resource::request", common::log::internal::transaction};

                        M message;
                        message.process = common::process::handle();
                        message.trid = transaction.trid;
                        message.flags = flags;

                        auto resources = std::get< 0>( common::range::partition(
                           transaction.resources,
                           filter));

                        //
                        // Update state on transaction-resources
                        //
                        common::range::for_each(
                           resources,
                           Transaction::Resource::update::Stage{ newStage});

                        state::pending::Request request{ message};

                        common::range::transform(
                           resources,
                           request.resources,
                           std::mem_fn( &Transaction::Resource::id));

                        if( ! action::resource::request( state, request))
                        {
                           //
                           // Could not send to all RM-proxy-instances. We put the request
                           // in pending.
                           //
                           common::log::internal::transaction << "could not send to all RM-proxy-instances - action: try later\n";

                           state.pending.requests.push_back( std::move( request));
                        }
                     }
                  } // resource
               } // send

               namespace instance
               {

                  void done( State& state, state::resource::Proxy::Instance& instance)
                  {
                     auto request = common::range::find_if(
                           state.pending.requests,
                           state::pending::filter::Request{ instance.id});

                     instance.state( state::resource::Proxy::Instance::State::idle);


                     if( request)
                     {
                        //
                        // We got a pending request for this resource, let's oblige
                        //
                        if( ipc::device().non_blocking_push( instance.process.queue, request->message))
                        {
                           instance.state( state::resource::Proxy::Instance::State::busy);

                           request->resources.erase(
                              std::find(
                                 std::begin( request->resources),
                                 std::end( request->resources),
                                 instance.id));

                           if( std::begin( request)->resources.empty())
                           {
                              state.pending.requests.erase( std::begin( request));
                           }
                        }
                        else
                        {
                           common::log::error << "failed to send pending request to resource, although the instance (" << instance <<  ") reported idle\n";
                        }
                     }
                  }

                  template< typename M>
                  void statistics( state::resource::Proxy::Instance& instance, M&& message, const common::platform::time_point& now)
                  {
                     instance.statistics.resource.time( message.statistics.start, message.statistics.end);
                     instance.statistics.roundtrip.end( now);
                  }

               } // instance

               namespace resource
               {

                  template< typename M>
                  void involved( State& state, Transaction& transaction, M&& message)
                  {
                     for( auto& resource : message.resources)
                     {
                        if( common::range::find( state.resources, resource))
                        {
                           transaction.resources.emplace_back( resource);
                        }
                        else
                        {
                           common::log::error << "invalid resource id: " << resource << " - involved with " << message.trid << " - action: discard\n";
                        }
                     }

                     common::range::trim( transaction.resources, common::range::unique( common::range::sort( transaction.resources)));

                     common::log::internal::transaction << "involved: " << transaction << '\n';

                  }

               } // resource

            } // <unnamed>
         } // local

         namespace process
         {


            void Exit::operator () ( message_type& message)
            {
               apply( message.death);
            }

            void Exit::apply( const common::process::lifetime::Exit& exit)
            {
               common::trace::Scope trace{ "transaction::handle::dead::Process", common::log::internal::transaction};

               // TODO: check if it's one of spawned resource proxies, if so, restart?

               //
               // Check if the now dead process is owner to any transactions, if so, roll'em back...
               //
               std::vector< common::transaction::ID> trids;

               for( auto& trans : m_state.transactions)
               {
                  if( trans.trid.owner().pid == exit.pid)
                  {
                     trids.push_back( trans.trid);
                  }
               }

               for( auto& trid : trids)
               {
                  common::message::transaction::rollback::Request request;
                  request.process = common::process::handle();
                  request.trid = trid;

                  //
                  // This could change the state, that's why we don't do it directly in the loop above.
                  //
                  handle::Rollback{ m_state}( request);
               }
            }
         }

         namespace resource
         {
            /*
            namespace id
            {
               void Allocation::operator () ( message_type& message)
               {
                  common::trace::Scope trace{ "transaction::handle::resource::id::Allocation", common::log::internal::transaction};

               }

            } // id
            */

            void Involved::operator () ( common::message::transaction::resource::Involved& message)
            {
               common::trace::Scope trace{ "transaction::handle::resource::Involved", common::log::internal::transaction};

               common::log::internal::transaction << "involved message: " << message << '\n';


               auto transaction = common::range::find_if(
                     m_state.transactions, find::Transaction( message.trid));

               if( transaction)
               {
                  local::resource::involved( m_state, *transaction, message);

               }
               else
               {
                  //
                  // First time this transaction is involved with a resource, we
                  // add it...
                  //
                  m_state.transactions.emplace_back( message.trid);
                  local::resource::involved( m_state, m_state.transactions.back(), message);
               }
            }

            namespace reply
            {

               template< typename H>
               void Wrapper< H>::operator () ( message_type& message)
               {

                  auto now = common::platform::clock_type::now();
                  //
                  // The instance is done and ready for more work
                  //

                  auto& instance = m_state.get_instance( message.resource, message.process.pid);

                  {
                     local::instance::done( this->m_state, instance);
                     local::instance::statistics( instance, message, now);
                  }

                  //
                  // Find the transaction
                  //
                  auto found = common::range::find_if(
                        common::range::make( m_state.transactions), find::Transaction{ message.trid});

                  if( found)
                  {
                     auto& transaction = *found;

                     auto resource = common::range::find_if(
                           transaction.resources,
                           Transaction::Resource::filter::ID{ message.resource});

                     if( resource)
                     {
                        //
                        // We found all the stuff, let the real handler handle the message
                        //
                        if( m_handler( message, transaction, *resource))
                        {
                           //
                           // We remove the transaction from our state
                           //
                           m_state.transactions.erase( std::begin( found));
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



               void Connect::operator () ( message_type& message)
               {
                  common::trace::Scope trace{ "transaction::handle::resource::connect reply", common::log::internal::transaction};

                  common::log::internal::transaction << "resource connected: " << message << std::endl;

                  try
                  {
                     auto& instance = m_state.get_instance( message.resource, message.process.pid);

                     if( message.state == XA_OK)
                     {
                        instance.process = std::move( message.process);

                        local::instance::done( m_state, instance);

                     }
                     else
                     {
                        common::log::error << "resource proxy: " <<  message.process << " startup error" << std::endl;
                        instance.state( state::resource::Proxy::Instance::State::error);
                        //throw common::exception::signal::Terminate{};
                        // TODO: what to do?
                     }

                  }
                  catch( common::exception::invalid::Argument&)
                  {
                     common::log::error << "unexpected resource connecting: " << message << " - action: discard" << std::endl;

                     common::log::internal::transaction << "resources: " << common::range::make( m_state.resources) << std::endl;
                  }


                  if( ! m_connected && common::range::all_of( common::range::make( m_state.resources), state::filter::Running{}))
                  {
                     //
                     // We now have enough resource proxies up and running to guarantee consistency
                     // notify broker
                     //
                     /*

                     common::log::internal::transaction << "enough resources are connected - send connect to broker\n";

                     common::message::transaction::manager::Ready running;
                     running.process = common::process::handle();
                     ipc::device().blocking_send( common::communication::ipc::broker::device(), running);
                     */

                     m_connected = true;
                  }
               }


               bool basic_prepare::operator () ( message_type& message, Transaction& transaction, Transaction::Resource& resource)
               {
                  common::trace::Scope trace{ "transaction::handle::resource::prepare reply", common::log::internal::transaction};

                  common::log::internal::transaction << "prepare reply - from: " << message.process << " rmid: " << message.resource << " result: " << common::error::xa::error( message.state) << '\n';


                  //
                  // If the resource only did a read only, we 'promote' it to 'not involved'
                  //
                  {
                     resource.result = Transaction::Resource::convert( message.state);

                     if( resource.result == Transaction::Resource::Result::xa_RDONLY)
                     {
                        resource.stage = Transaction::Resource::Stage::not_involved;
                     }
                     else
                     {
                        resource.stage = Transaction::Resource::Stage::prepare_replied;
                     }
                  }


                  auto stage = transaction.stage();

                  //
                  // Are we in a prepared state?
                  //
                  if( stage == Transaction::Resource::Stage::prepare_replied)
                  {

                     using reply_type = common::message::transaction::commit::Reply;

                     //
                     // Normalize all the resources return-state
                     //
                     auto result = transaction.results();

                     switch( result)
                     {
                        case Transaction::Resource::Result::xa_RDONLY:
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
                           {
                              auto reply = local::transform::message< reply_type>( message);
                              reply.correlation = transaction.correlation;
                              reply.stage = reply_type::Stage::commit;
                              reply.state = XA_RDONLY;

                              local::send::reply( m_state, std::move( reply), transaction.trid.owner());
                           }

                           //
                           // Indicate that wrapper should remove the transaction from state
                           //
                           return true;
                        }
                        case Transaction::Resource::Result::xa_OK:
                        {
                           common::log::internal::transaction << "prepare completed - " << transaction << " XA_OK\n";

                           //
                           // Prepare has gone ok. Log state
                           //
                           m_state.log.prepare( transaction.trid);

                           //
                           // prepare send reply. Will be sent after persistent write to file
                           //
                           {
                              auto reply = local::transform::message< reply_type>( message);
                              reply.correlation = transaction.correlation;
                              reply.stage = reply_type::Stage::prepare;
                              reply.state = XA_OK;

                              local::send::persistent::reply( m_state, std::move( reply), transaction.trid.owner());
                           }

                           //
                           // All XA_OK is to be committed, send commit to all
                           //

                           //
                           // We only want to send to resorces that has reported ok, and is in prepared state
                           // (could be that some has read-only)
                           //
                           auto filter = common::chain::And::link(
                                 Transaction::Resource::filter::Result{ Transaction::Resource::Result::xa_OK},
                                 Transaction::Resource::filter::Stage{ Transaction::Resource::Stage::prepare_replied});

                           local::send::resource::request<
                              common::message::transaction::resource::commit::Request>( m_state, transaction, filter, Transaction::Resource::Stage::prepare_requested);


                           break;
                        }
                        default:
                        {
                           //
                           // Something has gone wrong.
                           //
                           common::log::error << "TODO: something has gone wrong - rollback...\n";

                           local::send::resource::request< common::message::transaction::resource::rollback::Request>(
                              m_state,
                              transaction,
                              Transaction::Resource::filter::Stage{ Transaction::Resource::Stage::prepare_replied},
                              Transaction::Resource::Stage::rollback_requested);

                           break;
                        }
                     }
                  }
                  //
                  // Else we wait for more replies...
                  //
                  return false;
               }


               bool basic_commit::operator () ( message_type& message, Transaction& transaction, Transaction::Resource& resource)
               {
                  common::trace::Scope trace{ "transaction::handle::resource::commit reply", common::log::internal::transaction};

                  common::log::internal::transaction << "commit reply - from: " << message.process << " rmid: " << message.resource << " result: " << common::error::xa::error( message.state) << '\n';

                  resource.stage = Transaction::Resource::Stage::commit_replied;

                  auto stage = transaction.stage();


                  //
                  // Are we in a commited state?
                  //
                  if( stage == Transaction::Resource::Stage::commit_replied)
                  {

                     using reply_type = common::message::transaction::commit::Reply;

                     //
                     // Normalize all the resources return-state
                     //
                     auto result = transaction.results();

                     switch( result)
                     {
                        case Transaction::Resource::Result::xa_OK:
                        {
                           common::log::internal::transaction << "commit completed - " << transaction << " XA_OK\n";

                           auto reply = local::transform::message< reply_type>( message);
                           reply.correlation = transaction.correlation;
                           reply.stage = reply_type::Stage::commit;
                           reply.state = XA_OK;

                           if( transaction.resources.size() <= 1)
                           {
                              local::send::reply( m_state, reply, transaction.trid.owner());
                           }
                           else
                           {
                              //
                              // Send reply
                              // TODO: We can't send reply directly without checking that the prepare-reply
                              // has been sent (prepare-reply could be pending for persistent write).
                              // For now, we do a delayed reply, so we're sure that the
                              // commit-reply arrives after the prepare-reply
                              //
                              local::send::persistent::reply( m_state, std::move( reply), transaction.trid.owner());
                           }

                           //
                           // Remove transaction
                           //
                           m_state.log.remove( message.trid);
                           return true;
                        }
                        default:
                        {
                           //
                           // Something has gone wrong.
                           //
                           common::log::error << "TODO: something has gone wrong...\n";

                           //
                           // prepare send reply. Will be sent after persistent write to file.
                           //
                           // TOOD: we do have to save the state of the transaction?
                           //
                           {
                              auto reply = local::transform::message< reply_type>( message);
                              reply.correlation = transaction.correlation;
                              reply.stage = reply_type::Stage::commit;
                              reply.state = Transaction::Resource::convert( result);

                              local::send::persistent::reply( m_state, std::move( reply), transaction.trid.owner());
                           }
                           break;
                        }
                     }
                  }
                  return false;
               }

               bool basic_rollback::operator () ( message_type& message, Transaction& transaction, Transaction::Resource& resource)
               {
                  common::trace::Scope trace{ "transaction::handle::resource::rollback reply", common::log::internal::transaction};

                  common::log::internal::transaction << "rollback reply - from: " << message.process << " rmid: " << message.resource << " result: " << common::error::xa::error( message.state) << '\n';

                  resource.stage = Transaction::Resource::Stage::rollback_replied;

                  auto stage = transaction.stage();


                  //
                  // Are we in a rolled back state?
                  //
                  if( stage == Transaction::Resource::Stage::rollback_replied)
                  {

                     using reply_type = common::message::transaction::rollback::Reply;

                     //
                     // Normalize all the resources return-state
                     //
                     auto result = transaction.results();

                     switch( result)
                     {
                        case Transaction::Resource::Result::xa_OK:
                        case Transaction::Resource::Result::xaer_NOTA:
                        case Transaction::Resource::Result::xa_RDONLY:
                        {
                           common::log::internal::transaction << "rollback completed - " << transaction << " XA_OK\n";

                           //
                           // Send reply
                           //
                           {
                              auto reply = local::transform::message< reply_type>( message);
                              reply.correlation = transaction.correlation;
                              reply.state = XA_OK;

                              local::send::reply( m_state, std::move( reply), transaction.trid.owner());
                           }

                           //
                           // Remove transaction
                           //
                           m_state.log.remove( message.trid);
                           return true;
                        }
                        default:
                        {
                           //
                           // Something has gone wrong.
                           //
                           common::log::error << "TODO: resource rollback - something has gone wrong...\n";

                           //
                           // prepare send reply. Will be sent after persistent write to file
                           //
                           //
                           // TOOD: we do have to save the state of the transaction?
                           //
                           {
                              auto reply = local::transform::message< reply_type>( message);
                              reply.correlation = transaction.correlation;
                              reply.state = Transaction::Resource::convert( result);

                              local::send::persistent::reply( m_state, std::move( reply), transaction.trid.owner());
                           }
                           break;
                        }
                     }

                  }
                  return false;
               }
            } // reply
         } // resource


         template< typename Handler>
         void user_reply_wrapper< Handler>::operator () ( typename Handler::message_type& message)
         {
            try
            {
               Handler::operator() ( message);
            }
            catch( const user::error& exception)
            {
               common::error::handler();

               auto reply = local::transform::reply( message);
               reply.stage = decltype( reply)::Stage::error;
               reply.state = exception.code();

               local::send::reply( Handler::m_state, std::move( reply), message.process);
            }
            catch( const common::exception::signal::Terminate&)
            {
               throw;
            }
            catch( ...)
            {
               common::log::error << "unexpected error - action: send reply XAER_RMERR\n";

               common::error::handler();

               auto reply = local::transform::reply( message);
               reply.stage = decltype( reply)::Stage::error;
               reply.state = XAER_RMERR;

               local::send::reply( Handler::m_state, std::move( reply), message.process);
            }
         }


         void basic_commit::operator () ( message_type& message)
         {
            common::Trace trace{ "transaction::handle::Commit", common::log::internal::transaction};

            auto found = common::range::find_if( m_state.transactions, find::Transaction{ message.trid});

            if( ! found)
            {
               //
               // transaction is not known to TM, hence no resources has been involved
               // up to this point. We add the transaction and
               //

               m_state.transactions.emplace_back( message.trid);

               //
               // We now have the transaction, we call recursive...
               //
               (*this)( message);
            }
            else
            {
               auto& transaction = *found;

               //
               // Make sure we add the involved resources from the commit message (if any)
               //
               local::resource::involved( m_state, transaction, message);

               switch( transaction.stage())
               {
                  case Transaction::Resource::Stage::involved:
                  case Transaction::Resource::Stage::not_involved:
                  {
                     break;
                  }
                  default:
                  {
                     throw user::error{ XAER_PROTO, "Attempt to commit transaction, which is not in a state for commit", CASUAL_NIP( message.trid)};
                  }
               }

               //
               // Only the owner of the transaction can fiddle with the transaction ?
               //


               switch( transaction.resources.size())
               {
                  case 0:
                  {
                     common::log::internal::transaction << "no resources involved - " << transaction << " XA_RDONLY\n";

                     //
                     // We can remove this transaction
                     //
                     m_state.transactions.erase( std::begin( found));

                     //
                     // Send reply
                     //
                     {
                        auto reply = local::transform::reply( message);
                        reply.state = XA_RDONLY;
                        reply.stage = reply_type::Stage::commit;

                        local::send::reply( m_state, std::move( reply), message.process);
                     }

                     break;
                  }
                  case 1:
                  {
                     //
                     // Only one resource involved, we do a one-phase-commit optimization.
                     //
                     common::log::internal::transaction << "only one resource involved - " << transaction << " TMONEPHASE\n";

                     //
                     // Keep the correlation so we can send correct reply
                     //
                     transaction.correlation = message.correlation;

                     local::send::resource::request< common::message::transaction::resource::commit::Request>(
                        m_state,
                        transaction,
                        Transaction::Resource::filter::Stage{ Transaction::Resource::Stage::involved},
                        Transaction::Resource::Stage::commit_requested,
                        TMONEPHASE
                     );

                     break;
                  }
                  default:
                  {
                     //
                     // More than one resource involved, we do the prepare stage
                     //
                     common::log::internal::transaction << "prepare " << transaction << "\n";

                     //
                     // Keep the correlation so we can send correct reply
                     //
                     transaction.correlation = message.correlation;

                     local::send::resource::request< common::message::transaction::resource::prepare::Request>(
                        m_state,
                        transaction,
                        Transaction::Resource::filter::Stage{ Transaction::Resource::Stage::involved},
                        Transaction::Resource::Stage::prepare_requested
                     );

                     break;
                  }
               }

            }
         }

         template struct user_reply_wrapper< basic_commit>;


         void basic_rollback::operator () ( message_type& message)
         {
            common::trace::Scope trace{ "transaction::handle::Rollback", common::log::internal::transaction};

            //
            // Find the transaction
            //
            auto found = common::range::find_if( m_state.transactions, find::Transaction{ message.trid});

            if( ! found)
            {
               //
               // transaction is not known to TM, hence no resources has been involved
               // up to this point. We add the transaction.
               //

               m_state.transactions.emplace_back( message.trid);

               //
               // We now have the transaction, we call recursive...
               //
               (*this)( message);
            }
            else
            {
               auto& transaction = *found;

               //
               // Make sure we add the involved resources from the rollback message (if any)
               //
               local::resource::involved( m_state, transaction, message);

               if( transaction.resources.empty())
               {
                  common::log::internal::transaction << "no resources involved - " << transaction << " XA_OK\n";

                  //
                  // We can remove this transaction.
                  //
                  m_state.transactions.erase( std::begin( found));

                  //
                  // Send reply
                  //
                  {
                     auto reply = local::transform::reply( message);
                     reply.state = XA_OK;

                     local::send::reply( m_state, std::move( reply), message.process);
                  }
               }
               else
               {
                  //
                  // Keep the correlation so we can send correct reply
                  //
                  transaction.correlation = message.correlation;

                  local::send::resource::request< common::message::transaction::resource::rollback::Request>(
                     m_state,
                     transaction,
                     Transaction::Resource::filter::Stage{ Transaction::Resource::Stage::involved},
                     Transaction::Resource::Stage::rollback_requested
                  );
               }
            }
         }

         template struct user_reply_wrapper< basic_rollback>;


         namespace domain
         {
            void Involved::operator () (common::message::transaction::resource::domain::Involved& message)
            {
               common::trace::Scope trace{ "transaction::handle::domain::Involved", common::log::internal::transaction};

               common::log::internal::transaction << "involved message: " << message << '\n';

            }

            void Prepare::operator () ( message_type& message)
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

                  local::send::resource::request< common::message::transaction::resource::domain::prepare::Request>(
                     m_state,
                     transaction,
                     Transaction::Resource::filter::Stage{ Transaction::Resource::Stage::involved},
                     Transaction::Resource::Stage::prepare_requested,
                     message.flags
                  );
               }
               else
               {
                  //
                  // We don't have the transaction. This could be for three reasons:
                  // 1) We had it, but it was already prepared from another domain, and XA
                  // optimization kicked in ("read only") and the transaction was done
                  // 2) casual have made some optimizations.
                  // 3) no resources involved
                  // Either way, we don't have it, so we just reply that it has been prepared with "read only"
                  //

                  common::log::internal::transaction << "XA_RDONLY transaction (" << message.trid << ") either does not exists (longer) in this domain or there are no resources involved - action: send prepare-reply (read only)\n";

                  //
                  // Send reply
                  //
                  {
                     auto reply = local::transform::reply( message);
                     reply.state = XA_RDONLY;

                     local::send::reply( m_state, std::move( reply), message.process);
                  }

               }
            }

            void Commit::operator () ( message_type& message)
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

                  local::send::resource::request< common::message::transaction::resource::domain::commit::Request>(
                     m_state,
                     transaction,
                     Transaction::Resource::filter::Stage{ Transaction::Resource::Stage::prepare_replied},
                     Transaction::Resource::Stage::commit_requested,
                     message.flags
                  );
               }
               else
               {

                  common::log::internal::transaction << "XAER_NOTA trid: " << message.trid << " is not known to this TM - action: send XAER_NOTA reply\n";

                  //
                  // Send reply
                  //
                  {
                     auto reply = local::transform::reply( message);
                     reply.state = XAER_NOTA;

                     local::send::reply( m_state, std::move( reply), message.process);
                  }
               }
            }

            void Rollback::operator () ( message_type& message)
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

                  local::send::resource::request< common::message::transaction::resource::domain::commit::Request>(
                     m_state,
                     transaction,
                     Transaction::Resource::filter::Stage{ Transaction::Resource::Stage::prepare_replied},
                     Transaction::Resource::Stage::rollback_requested,
                     message.flags
                  );
               }
               else
               {
                  common::log::internal::transaction << "XAER_NOTA trid: " << message.trid << " is not known to this TM - action: send XAER_NOTA reply\n";

                  //
                  // Send reply
                  //
                  {
                     auto reply = local::transform::reply( message);
                     reply.state = XAER_NOTA;

                     local::send::reply( m_state, std::move( reply), message.process);
                  }
               }
            }

            namespace resource
            {
               namespace reply
               {
                  bool basic_prepare::operator () (  message_type& message, Transaction& transaction, Transaction::Resource& resource)
                  {
                     common::trace::Scope trace{ "transaction::handle::domain::resource::prepare reply", common::log::internal::transaction};

                     resource.result = Transaction::Resource::convert( message.state);
                     resource.stage = Transaction::Resource::Stage::prepare_replied;


                     if( common::range::all_of( transaction.resources,
                           Transaction::Resource::filter::Stage{ Transaction::Resource::Stage::prepare_replied}))
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
                        {
                           auto reply = local::transform::message< reply_type>( message);
                           reply.state = Transaction::Resource::convert( result);

                           local::send::reply( m_state, std::move( reply), message.process);
                        }
                     }

                     //
                     // else we wait...
                     return false;
                  }


                  bool basic_commit::operator () (  message_type& message, Transaction& transaction, Transaction::Resource& resource)
                  {
                     common::trace::Scope trace{ "transaction::handle::domain::resource::commit reply", common::log::internal::transaction};

                     resource.result = Transaction::Resource::convert( message.state);
                     resource.stage = Transaction::Resource::Stage::commit_replied;


                     if( common::range::all_of( transaction.resources,
                           Transaction::Resource::filter::Stage{ Transaction::Resource::Stage::commit_replied}))
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
                        {
                           auto reply = local::transform::message< reply_type>( message);
                           reply.state = Transaction::Resource::convert( result);

                           local::send::reply( m_state, std::move( reply), message.process);
                        }

                        //
                        // We remove the transaction
                        //
                        return true;
                     }

                     //
                     // else we wait...

                     return false;
                  }

                  bool basic_rollback::operator () (  message_type& message, Transaction& transaction, Transaction::Resource& resource)
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
