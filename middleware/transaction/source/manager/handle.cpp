//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "transaction/manager/handle.h"
#include "transaction/manager/action.h"
#include "transaction/common.h"
#include "transaction/manager/admin/server.h"


#include "common/message/handle.h"
#include "common/event/listen.h"
#include "common/server/handle/call.h"
#include "common/code/convert.h"

#include "common/communication/instance.h"

#include "domain/pending/message/send.h"


namespace casual
{
   namespace transaction
   {
      namespace manager
      {
         namespace ipc
         {

            const common::communication::ipc::Helper& device()
            {
               static common::communication::ipc::Helper ipc{
                  common::communication::error::handler::callback::on::Terminate
                  {
                     []( const common::process::lifetime::Exit& exit)
                     {
                        // We put a dead process event on our own ipc device, that
                        // will be handled later on.
                        common::message::event::process::Exit event{ exit};
                        common::communication::ipc::inbound::device().push( std::move( event));
                     }
                  }
               };
               return ipc;
            }

            namespace
            {
               namespace pending
               {
                  template< typename M>
                  void send( M&& message, const common::process::Handle& target)
                  {
                     try
                     {
                        if( ! ipc::device().non_blocking_send( target.ipc, message))
                        {
                           common::log::line( log, "failed to send reply directly to : ", target,  " - action: pend reply");
                           casual::domain::pending::message::send( target, message, ipc::device().error_handler());
                        }
                     }
                     catch( const common::exception::system::communication::Unavailable&)
                     {
                        common::log::line( log, "failed to send message to queue: ", device);
                     }

                  }
               } // pending

               namespace optional
               {
                  
                  template< typename D, typename M>
                  void send( D&& device, M&& message)
                  {
                     try
                     {
                        ipc::device().blocking_send( device, message);
                        common::log::line( verbose::log, "sent message: ", message);
                     }
                     catch( const common::exception::system::communication::Unavailable&)
                     {
                        common::log::line( log, "failed to send message to queue: ", device);
                     }
                  }
               } // optional


            } //
         } // ipc

         namespace handle
         {
            namespace persist
            {

               namespace local
               {
                  namespace
                  {
                     void send( State& state)
                     {
                        // persist transaction log   
                        state.persistent.log.persist();

                        // we'll consume every message either way
                        auto persistent = std::exchange( state.persistent.replies, {});

                        common::log::line( verbose::log, "persistent: ", persistent);

                        auto direct_send = []( auto& message)
                        {
                           return common::message::pending::non::blocking::send( message, ipc::device().error_handler());
                        };

                        // try send directly
                        auto pending = common::algorithm::remove_if( persistent, direct_send);
                     
                        common::log::line( verbose::log, "pending: ", pending);


                        auto pending_send = []( auto& message)
                        {
                           casual::domain::pending::message::send( message, ipc::device().error_handler());
                        };
                        
                        // send failed non-blocking sends to pending
                        common::algorithm::for_each( pending, pending_send);
                     }
                  } // <unnamed>
               } // local


               void send( State& state)
               {
                  Trace trace{ "transaction::handle::persist:send"};

                  // if we don't have any persistent replies, we don't need to persist
                  if( ! state.persistent.replies.empty())
                     local::send( state);
                 
               }
                  
               namespace batch
               {
                  void send( State& state)
                  {  
                     // check if we've reach our "batch-limit", if so, persit and send replies
                     if( state.persistent.replies.size() >= platform::batch::transaction::persistence)
                        local::send( state);  
                  }
                  
               } // batch
            } // persist

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

                        // TODO: remove, is not needed everywhere
                        result.trid = message.trid;

                        return result;
                     }

                     template< typename M>
                     auto reply( M&& message)
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
                        
                        template< typename M>
                        void reply( State& state, M&& message, const common::process::Handle& target)
                        {
                           state.persistent.replies.emplace_back( std::move( message), target);
                           handle::persist::batch::send( state);
                        }

                        template< typename R, typename M>
                        void reply( State& state, const M& request, int code, const common::process::Handle& target)
                        {
                           R message;
                           message.correlation = request.correlation;
                           message.process = common::process::handle();
                           message.trid = request.trid;
                           message.state = code;

                           reply( state, std::move( message), target);
                        }

                        template< typename R, typename M>
                        void reply( State& state, const M& request, int code)
                        {
                           reply< R>( state, request, code, request.process);
                        }

                     } // persistent

   
                     template< typename R>
                     void reply( R&& message, const common::process::Handle& target)
                     {
                        ipc::pending::send( std::forward< R>( message), target);
                     }

                     template< typename R>
                     void xa_result( R&& message, common::code::xa result)
                     {
                        auto reply = local::transform::reply( message);
                        reply.state = result;
                        reply.trid = message.trid;
                        reply.resource = message.resource;

                        send::reply( std::move( reply), message.process);
                     }

                     template< typename R>
                     void read_only( R&& message)
                     {
                        xa_result( message, common::code::xa::read_only);
                     }


                     namespace resource
                     {
                        template< typename M, typename F>
                        void request( State& state, 
                           Transaction& transaction, 
                           F&& filter, 
                           Transaction::Resource::Stage new_stage, 
                           common::flag::xa::Flags flags = common::flag::xa::Flag::no_flags)
                        {
                           Trace trace{ "transaction::handle::send::resource::request"};

                           auto branch_request = [&]( auto& branch)
                           {
                              auto resource_request = [&]( auto& resource)
                              {
                                 auto create_message = [&]()
                                 {
                                    M message;
                                    message.process = common::process::handle();
                                    message.trid = branch.trid;
                                    message.flags = flags;
                                    message.resource = resource.id;
                                    return message;
                                 };

                                 state::pending::Request request{ resource.id, create_message()};

                                 if( ! action::resource::request( state, request))
                                 {
                                    // Could not send to resource. We put the request
                                    // in pending.
                                    common::log::line( log, "could not send to resource: ", resource.id, " - action: try later");

                                    state.pending.requests.push_back( std::move( request));
                                 }
                              };

                              auto resources = common::algorithm::filter( branch.resources, filter);

                              // Update state on resources
                              common::algorithm::for_each(
                                 resources,
                                 Transaction::Resource::update::stage( new_stage));

                              common::log::line( verbose::log, "resources: ", resources);

                              //! send request to all resources associated with this branch
                              common::algorithm::for_each( branch.resources, resource_request);
                           };

                           //! send request to all resources associated to all branches
                           common::algorithm::for_each( transaction.branches, branch_request);
                        }

                     } // resource
                  } // send

                  namespace instance
                  {
                     void done( State& state, state::resource::Proxy::Instance& instance)
                     {
                        auto request = common::algorithm::find( state.pending.requests, instance.id);

                        instance.state( state::resource::Proxy::Instance::State::idle);


                        if( request)
                        {
                           // We got a pending request for this resource, let's oblige
                           if( ipc::device().non_blocking_push( instance.process.ipc, request->message))
                           {
                              instance.state( state::resource::Proxy::Instance::State::busy);

                              state.pending.requests.erase( std::begin( request));
                           }
                           else
                           {
                              common::log::line( common::log::category::error, "failed to send pending request to resource, although the instance (", instance , ") reported idle");
                           }
                        }
                     }

                     template< typename M>
                     void metric( state::resource::Proxy::Instance& instance, M&& message, const platform::time::point::type& now)
                     {
                        instance.metrics.resource += message.statistics.end - message.statistics.start;
                        instance.metrics.roundtrip += now - instance.metrics.requested;
                     }

                  } // instance

                  namespace resource
                  {
                     template< typename M>
                     void involve( State& state, Transaction::Branch& branch, M&& message)
                     {
                        auto involve_resource = [&]( auto& resource)
                        {
                           if( common::algorithm::find( state.resources, resource))
                           {
                              branch.involve( resource);
                              return true;
                           }
                           return false;
                        };

                        auto partition = common::algorithm::partition( branch.resources, involve_resource);

                        auto unknown = std::get< 1>( partition);

                        if( unknown)
                        {
                           common::log::line( common::log::category::error, "invalid resources: ", unknown, " - action: discard");
                           common::log::line( common::log::category::verbose::error, "trid: ", message.trid);
                        }

                        common::log::line( log, "branch: ", branch);
                     }
                  } // resource

                  namespace transaction
                  {

                     template< typename M, typename I>
                     auto find_or_add_and_involve( State& state, M&& message, I&& involved)
                     {
                        // Find the transaction
                        auto transaction = common::algorithm::find( state.transactions, message.trid);

                        if( ! transaction)
                        {
                           state.transactions.emplace_back( message.trid);
                           auto end = std::end( state.transactions);
                           transaction = common::range::make( std::prev( end), end);
                        }

                        auto branch = common::algorithm::find( transaction->branches, message.trid);

                        if( ! branch)
                        {
                           transaction->branches.emplace_back( message.trid);
                           auto end = std::end( transaction->branches);
                           branch = common::range::make( std::prev( end), end);
                        }
                           
                        branch->involve( involved);

                        return transaction;
                     }

                     template< typename M>
                     auto find_or_add_and_involve( State& state, M&& message)
                     {
                        return find_or_add_and_involve( state, std::forward< M>( message), message.involved);
                     }

                  } // transaction

                  namespace branch
                  {
                     template< typename M>
                     Transaction::Branch& find_or_add( State& state, M&& message)
                     {
                        // Find the transaction
                        auto transaction = common::algorithm::find( state.transactions, message.trid);

                        if( transaction)
                        {
                           // try find the branch
                           auto branch = common::algorithm::find( transaction->branches, message.trid);

                           if( branch)
                              return *branch;

                           transaction->branches.emplace_back( message.trid);
                           return transaction->branches.back();
                        }

                        state.transactions.emplace_back( message.trid);

                        return state.transactions.back().branches.back();
                     }

                  } // branch

                  namespace dispatch
                  {
                     namespace localized
                     {
                        bool prepare( State& state, common::message::transaction::resource::prepare::Reply& message, Transaction& transaction)
                        {
                           Trace trace{ "transaction::handle::local::dispatch::localized::prepare"};

                           using reply_type = common::message::transaction::commit::Reply;

                           // Normalize all the resources return-state
                           auto result = transaction.results();

                           switch( result)
                           {
                              using xa = common::code::xa;

                              case xa::read_only:
                              {
                                 common::log::line( log, result, " prepare completed - ", transaction);

                                 // Read-only optimization. We can send the reply directly and
                                 // discard the transaction
                                 state.persistent.log.remove( transaction.global);

                                 // Send reply
                                 {
                                    auto reply = local::transform::message< reply_type>( message);
                                    reply.correlation = transaction.correlation;
                                    reply.trid = transaction.global.trid;
                                    reply.stage = reply_type::Stage::commit;
                                    reply.state = common::code::tx::ok;

                                    local::send::reply( std::move( reply), transaction.owner());
                                 }

                                 // Indicate that wrapper should remove the transaction from state
                                 return true;
                              }
                              case xa::ok:
                              {
                                 common::log::line( log, result, " prepare completed - ", transaction);

                                 // Prepare has gone ok. Log state
                                 state.persistent.log.prepare( transaction);

                                 // prepare send reply. Will be sent after persistent write to file
                                 {
                                    auto reply = local::transform::message< reply_type>( message);
                                    reply.correlation = transaction.correlation;
                                    reply.trid = transaction.global.trid;
                                    reply.stage = reply_type::Stage::prepare;
                                    reply.state = common::code::tx::ok;

                                    local::send::persistent::reply( state, std::move( reply), transaction.owner());
                                 }

                                 // All XA_OK is to be committed, send commit to all

                                 // We only want to send to resources that has reported ok, and is in prepared state
                                 // (could be that some has read-only)
                                 auto filter = common::predicate::make_and(
                                       Transaction::Resource::filter::result( Transaction::Resource::Result::xa_OK),
                                       Transaction::Resource::filter::stage( Transaction::Resource::Stage::prepare_replied));

                                 local::send::resource::request<
                                    common::message::transaction::resource::commit::Request>( state, transaction, filter, Transaction::Resource::Stage::prepare_requested);

                                 break;
                              }
                              default:
                              {
                                 // Something has gone wrong.
                                 common::log::line( common::log::category::error, result, "prepare failed for: ", transaction.global, " - action: rollback");
                                 common::log::line( common::log::category::verbose::error, "transaction: ", transaction);

                                 local::send::resource::request< common::message::transaction::resource::rollback::Request>(
                                    state,
                                    transaction,
                                    Transaction::Resource::filter::stage( Transaction::Resource::Stage::prepare_replied),
                                    Transaction::Resource::Stage::rollback_requested);

                                 break;
                              }
                           }
                           // Transaction is not done
                           return false;
                        }

                        bool commit( State& state, common::message::transaction::resource::commit::Reply& message, Transaction& transaction)
                        {
                           Trace trace{ "transaction::handle::local::dispatch::localized::commit"};

                           using reply_type = common::message::transaction::commit::Reply;

                           // Normalize all the resources return-state
                           auto result = transaction.results();

                           switch( result)
                           {
                              using xa = common::code::xa;
                              
                              case xa::ok:
                              case xa::read_only:
                              {
                                 common::log::line( log, result, " commit completed ", transaction);

                                 auto reply = local::transform::message< reply_type>( message);
                                 reply.correlation = transaction.correlation;
                                 reply.trid = transaction.global.trid;
                                 reply.stage = reply_type::Stage::commit;
                                 reply.state = common::code::tx::ok;

                                 if( transaction.resource_count() <= 1)
                                 {
                                    local::send::reply( reply, transaction.owner());
                                 }
                                 else
                                 {
                                    // Send reply
                                    // TODO: We can't send reply directly without checking that the prepare-reply
                                    // has been sent (prepare-reply could be pending for persistent write).
                                    // For now, we do a delayed reply, so we're sure that the
                                    // commit-reply arrives after the prepare-reply
                                    local::send::persistent::reply( state, std::move( reply), transaction.owner());
                                 }

                                 // Remove transaction
                                 state.persistent.log.remove( message.trid);
                                 return true;
                              }
                              default:
                              {
                                 // Something has gone wrong.
                                 common::log::line( common::log::category::error, result, " commit gone wrong for: ", transaction.global);
                                 common::log::line( common::log::category::verbose::error, "transaction: ", transaction);
                                 
                                 // prepare send reply. Will be sent after persistent write to file.
                                 // TOOD: we do have to save the state of the transaction?
                                 {
                                    auto reply = local::transform::message< reply_type>( message);
                                    reply.correlation = transaction.correlation;
                                    reply.trid = transaction.global.trid;
                                    reply.stage = reply_type::Stage::commit;
                                    reply.state = common::code::convert::to::tx( result);

                                    local::send::persistent::reply( state, std::move( reply), transaction.owner());
                                 }
                                 break;
                              }
                           }

                           return false;
                        }

                        bool rollback( State& state, common::message::transaction::resource::rollback::Reply& message, Transaction& transaction)
                        {
                           Trace trace{ "transaction::handle::local::dispatch::localized::rollback"};

                           using reply_type = common::message::transaction::rollback::Reply;

                           // Normalize all the resources return-state
                           auto result = transaction.results();

                           switch( result)
                           {
                              using xa = common::code::xa;

                              case xa::ok:
                              case xa::invalid_xid:
                              case xa::read_only:
                              {
                                 common::log::line( log, result, " rollback completed ", transaction.global);

                                 // Send reply
                                 {
                                    auto reply = local::transform::message< reply_type>( message);
                                    reply.correlation = transaction.correlation;
                                    reply.trid = transaction.global.trid;
                                    reply.state = common::code::tx::ok;

                                    local::send::reply( std::move( reply), transaction.owner());
                                 }

                                 // Remove transaction
                                 state.persistent.log.remove( message.trid);
                                 return true;
                              }
                              default:
                              {
                                 // Something has gone wrong.
                                 common::log::line( common::log::category::error, result, " rollback gone wrong for: ", transaction.global);
                                 common::log::line( common::log::category::verbose::error, "transaction: ", transaction);

                                 // prepare send reply. Will be sent after persistent write to file
                                 // TOOD: we do have to save the state of the transaction?
                                 {
                                    auto reply = local::transform::message< reply_type>( message);
                                    reply.correlation = transaction.correlation;
                                    reply.trid = transaction.global.trid;
                                    reply.state = common::code::convert::to::tx( result);

                                    local::send::persistent::reply( state, std::move( reply), transaction.owner());
                                 }
                                 break;
                              }
                           }
                           return false;
                        }

                        auto get()
                        {
                           return Transaction::Dispatch{
                              &prepare,
                              &commit,
                              &rollback
                           };
                        }
                     } // localized

                     namespace remote
                     {
                        bool prepare( State& state, common::message::transaction::resource::prepare::Reply& message, Transaction& transaction) 
                        {
                           Trace trace{ "transaction::handle::implementation::remote::prepare"};

                           // Transaction is owned by another domain, so we just act as a resource.
                           // This TM does not own the transaction, so we don't need to store
                           // state.

                           using reply_type = common::message::transaction::resource::prepare::Reply;

                           {
                              auto reply = local::transform::message< reply_type>( message);
                              reply.state = transaction.results();
                              reply.trid = transaction.global.trid;
                              reply.correlation = transaction.correlation;
                              reply.resource = transaction.resource;

                              local::send::reply( std::move( reply), transaction.owner());
                           }

                           return false;
                        }

                        bool commit( State& state, common::message::transaction::resource::commit::Reply& message, Transaction& transaction)
                        {
                           Trace trace{ "transaction::handle::implementation::remote::commit"};

                           // Transaction is owned by another domain, so we just act as a resource.
                           // This TM does not own the transaction, so we don't need to store
                           // state.

                           using reply_type = common::message::transaction::resource::commit::Reply;

                           {
                              auto reply = local::transform::message< reply_type>( message);
                              reply.state = transaction.results();
                              reply.trid = transaction.global.trid;
                              reply.correlation = transaction.correlation;
                              reply.resource = transaction.resource;

                              local::send::reply( std::move( reply), transaction.owner());
                           }

                           return true;
                        }

                        bool rollback( State& state, common::message::transaction::resource::rollback::Reply& message, Transaction& transaction)
                        {
                           Trace trace{ "transaction::handle::implementation::remote::rollback"};

                           // Transaction is owned by another domain, so we just act as a resource.
                           // This TM does not own the transaction, so we don't need to store
                           // state.

                           using reply_type = common::message::transaction::resource::rollback::Reply;

                           {
                              auto reply = local::transform::message< reply_type>( message);
                              reply.state = transaction.results();
                              reply.trid = transaction.global.trid;
                              reply.correlation = transaction.correlation;
                              reply.resource = transaction.resource;

                              local::send::reply( std::move( reply), transaction.owner());
                           }

                           return true;
                        }

                        auto get()
                        {
                           return Transaction::Dispatch{
                              &prepare,
                              &commit,
                              &rollback
                           };
                        }
                     } // remote


                     namespace one
                     {
                        namespace phase
                        {
                           namespace commit
                           {
                              namespace remote
                              {
                                 bool prepare( State& state, common::message::transaction::resource::prepare::Reply& message, Transaction& transaction)
                                 {
                                    Trace trace{ "transaction::handle::local::dispatch::one::phase::commit::remote::prepare"};

                                    // We're done with the prepare phase, start with commit or rollback

                                    using reply_type = common::message::transaction::resource::commit::Reply;

                                    // Normalize all the resources return-state
                                    auto result = transaction.results();

                                    switch( result)
                                    {
                                       using xa = common::code::xa;

                                       case xa::read_only:
                                       {
                                          common::log::line( log, result, " prepare completed - ", transaction);

                                          // Read-only optimization. We can send the reply directly and
                                          // discard the transaction
                                          state.persistent.log.remove( transaction.global);

                                          // Send reply
                                          {
                                             auto reply = local::transform::message< reply_type>( message);
                                             reply.correlation = transaction.correlation;
                                             reply.trid = transaction.global.trid;
                                             reply.resource = transaction.resource;
                                             reply.state = result;

                                             local::send::reply( std::move( reply), transaction.owner());
                                          }

                                          // Indicate that wrapper should remove the transaction from state
                                          return true;
                                       }
                                       case xa::ok:
                                       {
                                          common::log::line( log, result, " prepare completed - ", transaction);

                                          // Prepare has gone ok. Log state
                                          state.persistent.log.prepare( transaction);

                                          // All XA_OK is to be committed, send commit to all

                                          // We only want to send to resources that has reported ok, and is in prepared state
                                          // (could be that some has read-only)
                                          auto filter = common::predicate::make_and(
                                                Transaction::Resource::filter::result( Transaction::Resource::Result::xa_OK),
                                                Transaction::Resource::filter::stage( Transaction::Resource::Stage::prepare_replied));

                                          local::send::resource::request<
                                             common::message::transaction::resource::commit::Request>(
                                                   state, transaction, filter, Transaction::Resource::Stage::prepare_requested);


                                          break;
                                       }
                                       default:
                                       {
                                          // Something has gone wrong.
                                          common::log::line( common::log::category::error, result, " prepare failed for: ", transaction.global, " - action: rollback");
                                          common::log::line( common::log::category::error, "transaction: ", transaction);

                                          local::send::resource::request< common::message::transaction::resource::rollback::Request>(
                                             state,
                                             transaction,
                                             Transaction::Resource::filter::stage( Transaction::Resource::Stage::prepare_replied),
                                             Transaction::Resource::Stage::rollback_requested);

                                          break;
                                       }
                                    }
                                    return false;
                                 }

                                 bool rollback( State& state, common::message::transaction::resource::rollback::Reply& message, Transaction& transaction)
                                 {
                                    Trace trace{ "transaction::handle::local::dispatch::one::phase::commit::remote::rollback"};

                                    using reply_type = common::message::transaction::resource::commit::Reply;

                                    // Send reply
                                    {
                                       auto reply = local::transform::message< reply_type>( message);
                                       reply.correlation = transaction.correlation;
                                       reply.trid = transaction.global.trid;
                                       reply.resource = transaction.resource;
                                       reply.state = common::code::xa::rollback_other;

                                       local::send::reply( std::move( reply), transaction.owner());
                                    }
                                    return true;
                                 }

                                 auto get()
                                 {
                                    return Transaction::Dispatch{
                                       &prepare,
                                       &dispatch::remote::commit, // we use the ordinary remote commit
                                       &rollback
                                    };
                                 }
                              } // remote
                           } // commit
                        } // phase
                     } // one
                  } // implementation
               } // <unnamed>
            } // local


            namespace process
            {
               void Exit::operator () ( message_type& message)
               {

                  Trace trace{ "transaction::handle::process::Exit"};

                  // Check if it's a resource proxy instance
                  if( m_state.remove_instance( message.state.pid))
                  {
                     ipc::device().blocking_send( common::communication::instance::outbound::domain::manager::device(), message);
                     return;
                  }

                  std::vector< common::transaction::ID> trids;

                  for( auto& transaction : m_state.transactions)
                  {
                     if( ! transaction.branches.empty() && transaction.branches.back().trid.owner().pid == message.state.pid)
                     {
                        trids.push_back( transaction.branches.back().trid);
                     }
                  }

                  common::log::line( verbose::log, "trids: ", trids);

                  for( auto& trid : trids)
                  {
                     common::message::transaction::rollback::Request request;
                     request.process = common::process::handle();
                     request.trid = trid;

                     // This could change the state, that's why we don't do it directly in the loop above.
                     handle::Rollback{ m_state}( request);
                  }
               }
            }

            namespace resource
            {
               void Lookup::operator () ( common::message::transaction::resource::lookup::Request& message)
               {
                  Trace trace{ "transaction::handle::resource::Lookup"};

                  auto reply = common::message::reverse::type( message);


                  for( auto& proxy : m_state.resources)
                  {
                     if( common::algorithm::find( message.resources, proxy.name))
                     {
                        common::message::transaction::resource::Resource resource;

                        resource.id = proxy.id;
                        resource.key = proxy.key;
                        resource.name = proxy.name;
                        resource.openinfo = proxy.openinfo;
                        resource.closeinfo = proxy.closeinfo;

                        reply.resources.push_back( std::move( resource));
                     }
                  }

                  ipc::optional::send( message.process.ipc, reply);
               }

               void Involved::operator () ( common::message::transaction::resource::involved::Request& message)
               {
                  Trace trace{ "transaction::handle::resource::Involved"};

                  common::log::line( verbose::log, "message: ",  message);

                  auto& branch = local::branch::find_or_add( m_state, message);

                  // prepare and send the reply
                  auto reply = common::message::reverse::type( message);
                  reply.involved = branch.involved();
                  ipc::optional::send( message.process.ipc, reply);

                  // partition what we don't got since before
                  auto involved = std::get< 1>( common::algorithm::intersection( message.involved, reply.involved));

                  // partition the new involved based on which we've got configured resources for
                  auto split = common::algorithm::partition( involved, [&]( auto& resource)
                  {
                     return ! common::algorithm::find( m_state.resources, resource).empty();
                  });

                  // add new involved resources, if any.
                  branch.involve( std::get< 0>( split));

                  // if we've got some resources that we don't know about.
                  // TODO: should we set the transaction to rollback only?
                  if( std::get< 1>( split))
                  {
                     common::log::line( common::log::category::error, "invalid resources: ", std::get< 1>( split), " - action: discard");
                     common::log::line( common::log::category::verbose::error, "trid: ", message.trid);
                  }
               }

               namespace reply
               {

                  template< typename H>
                  void Wrapper< H>::operator () ( message_type& message)
                  {
                     Trace trace{ "transaction::handle::resource::reply::Wrapper"};

                     common::log::line( verbose::log, "message: ", message);

                     if( state::resource::id::local( message.resource))
                     {
                        // The resource is a local resource proxy, and it's done
                        // and ready for more work

                        auto& instance = m_state.get_instance( message.resource, message.process.pid);

                        {
                           local::instance::done( this->m_state, instance);
                           local::instance::metric( instance, message, platform::time::clock::type::now());
                        }
                     }

                     // Find the transaction
                     auto found = common::algorithm::find( m_state.transactions, message.trid);

                     if( found)
                     {
                        auto& transaction = *found;

                        auto branch = common::algorithm::find( transaction.branches, message.trid);

                        auto resource = common::algorithm::find(
                              branch->resources,
                              message.resource);

                        if( resource)
                        {
                           // We found all the stuff

                           // check if we're done
                           resource->set_result( message.state);

                           if( resource->done())
                              branch->resources.erase( std::begin( resource));
                           else
                              resource->stage = handler_type::stage();

                           common::log::line( verbose::log, "transaction: ", transaction);

                           //  let the real handler handle the message
                           if( m_handler( message, transaction))
                           {
                              // We remove the transaction from our state
                              m_state.transactions.erase( std::begin( found));
                           }
                        }
                        else
                        {
                           // TODO: what to do? We have previously sent a prepare request, why do we not find the resource?
                           common::log::line( common::log::category::error, "failed to locate resource: ", message.resource, " for trid: ", message.trid, " - action: discard?");
                        }
                     }
                     else
                     {
                        // TODO: what to do? We have previously sent a prepare request, why do we not find the trid?
                        common::log::line( common::log::category::error, "failed to locate trid: ", message.trid, " - action: discard?");
                     }
                  }


                  bool basic_prepare::operator () ( message_type& message, Transaction& transaction)
                  {
                     Trace trace{ "transaction::handle::resource::prepare reply"};

                     // Are we in a prepared state? If not, we wait for more replies...
                     if( transaction.stage() < Transaction::Resource::Stage::prepare_replied)
                        return false;

                     return transaction.implementation.prepare( m_state, message, transaction);
                  }


                  bool basic_commit::operator () ( message_type& message, Transaction& transaction)
                  {
                     Trace trace{ "transaction::handle::resource::commit reply"};

                     // Are we in a committed state? If not, we wait for more replies...
                     if( transaction.stage() < Transaction::Resource::Stage::commit_replied)
                        return false;

                     return transaction.implementation.commit( m_state, message, transaction);
                  }


                  bool basic_rollback::operator () ( message_type& message, Transaction& transaction)
                  {
                     Trace trace{ "transaction::handle::resource::rollback reply"};

                     // Are we in a rolled back stage?
                     if( transaction.stage() < Transaction::Resource::Stage::rollback_replied)
                        return false;

                     return transaction.implementation.rollback( m_state, message, transaction);
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
                  common::exception::handle();

                  auto reply = local::transform::reply( message);
                  reply.stage = decltype( reply)::Stage::error;
                  reply.state = exception.type();

                  local::send::reply( std::move( reply), message.process);
               }
               catch( const common::exception::signal::Terminate&)
               {
                  throw;
               }
               catch( ...)
               {
                  auto fail = common::code::tx::fail;
                  common::log::line( common::log::category::error, fail, " unexpected error - action: send reply ");

                  common::exception::handle();

                  auto reply = local::transform::reply( message);
                  reply.stage = decltype( reply)::Stage::error;
                  reply.state = fail;

                  local::send::reply( std::move( reply), message.process);
               }
            }


            void basic_commit::operator () ( message_type& message)
            {
               Trace trace{ "transaction::handle::Commit"};

               common::log::line( verbose::log, "message: ", message);

               auto position = local::transaction::find_or_add_and_involve( m_state, message);
               auto& transaction = *position;

               common::log::line( verbose::log, "transaction: ", transaction);
               
               switch( transaction.stage())
               {
                  case Transaction::Resource::Stage::involved:
                  case Transaction::Resource::Stage::not_involved:
                  {
                     break;
                  }
                  default:
                  {
                     throw user::error{ common::code::tx::protocol, common::string::compose( "Attempt to commit transaction, which is not in a state for commit - trid: ",message.trid)};
                  }
               }

               // Local normal commit phase
               transaction.implementation = local::dispatch::localized::get();

               // Only the owner of the transaction can fiddle with the transaction ?

               switch( transaction.resource_count())
               {
                  case 0:
                  {
                     common::log::line( log, transaction.global, " no resources involved: ");

                     // We can remove this transaction
                     m_state.transactions.erase( std::begin( position));

                     // Send reply
                     {
                        auto reply = local::transform::reply( message);
                        //reply.state = common::code::xa::read_only;
                        reply.state = common::code::tx::ok;
                        reply.stage = reply_type::Stage::commit;

                        local::send::reply( std::move( reply), message.process);
                     }

                     break;
                  }
                  case 1:
                  {
                     // Only one resource involved, we do a one-phase-commit optimization.
                     common::log::line( log, transaction.global, " only one resource involved");

                     // Keep the correlation so we can send correct reply
                     transaction.correlation = message.correlation;

                     local::send::resource::request< common::message::transaction::resource::commit::Request>(
                        m_state,
                        transaction,
                        Transaction::Resource::filter::stage( Transaction::Resource::Stage::involved),
                        Transaction::Resource::Stage::commit_requested,
                        common::flag::xa::Flag::one_phase
                     );

                     break;
                  }
                  default:
                  {
                     // Keep the correlation so we can send correct reply
                     transaction.correlation = message.correlation;

                     // More than one resource involved, we do the prepare stage
                     common::log::line( log, transaction.global, " more than one resource involved");

                     local::send::resource::request< common::message::transaction::resource::prepare::Request>(
                        m_state,
                        transaction,
                        Transaction::Resource::filter::stage( Transaction::Resource::Stage::involved),
                        Transaction::Resource::Stage::prepare_requested
                     );

                     break;
                  }
               }
            }

            template struct user_reply_wrapper< basic_commit>;


            void basic_rollback::operator () ( message_type& message)
            {
               Trace trace{ "transaction::handle::Rollback"};

               common::log::line( log, "message: ", message);

               auto location = local::transaction::find_or_add_and_involve( m_state, message);
               auto& transaction = *location;

               // Local normal rollback phase
               transaction.implementation = local::dispatch::localized::get();

               if( transaction.resource_count() == 0)
               {
                  common::log::line( log, transaction.global, " no resources involved");

                  // We can remove this transaction.
                  m_state.transactions.erase( std::begin( location));

                  // Send reply
                  {
                     auto reply = local::transform::reply( message);
                     reply.state = common::code::tx::ok;

                     local::send::reply( std::move( reply), message.process);
                  }
               }
               else
               {
                  common::log::line( log, transaction.global, " resources involved");

                  // Keep the correlation so we can send correct reply
                  transaction.correlation = message.correlation;

                  local::send::resource::request< common::message::transaction::resource::rollback::Request>(
                     m_state,
                     transaction,
                     []( auto& r){ return r.stage < Transaction::Resource::Stage::not_involved;},
                     Transaction::Resource::Stage::rollback_requested
                  );
               }
            }

            template struct user_reply_wrapper< basic_rollback>;

            namespace external
            {
               void Involved::operator () ( common::message::transaction::resource::external::Involved& message)
               {
                  Trace trace{ "transaction::handle::external::Involved"};

                  common::log::line( log, "message: ", message);

                  auto id = state::resource::external::proxy::id( m_state, message.process);

                  auto& transaction = *local::transaction::find_or_add_and_involve( m_state, message, id);

                  common::log::line( verbose::log, "transaction: ", transaction);
               }
            } // external

            namespace domain
            {

               template< typename M>
               void Base::prepare_remote_owner( Transaction& transaction, M& message)
               {
                  transaction.correlation = message.correlation;
                  transaction.resource = message.resource;

                  // make sure we keep track of the transaction from this message,
                  // so that we reply with the same trid (including branch).
                  transaction.global.trid = message.trid;
                  transaction.global.trid.owner( message.process);

                  auto remove_obsolete_resources = [&]( auto& branch)
                  {  
                     auto obsolete_resources = [&]( auto& resource)
                     {
                        return state::resource::id::remote( resource.id) 
                           && m_state.get_external( resource.id).process.pid == message.process.pid;
                     };

                     // Remove resource from transaction if it's the same as the instigator for the
                     // prepare, commit or rollback
                     common::algorithm::trim( branch.resources, common::algorithm::remove_if( branch.resources, obsolete_resources));
                  };

                  common::algorithm::for_each( transaction.branches, remove_obsolete_resources);
               }


               void Prepare::operator () ( message_type& message)
               {
                  Trace trace{ "transaction::handle::domain::prepare request"};

                  common::log::line( log, "message: ", message);

                  // Find the transaction
                  auto found = common::algorithm::find( m_state.transactions, message.trid);

                  if( found)
                  {
                     if( handle( message, *found) == Directive::remove_transaction)
                     {
                        // We remove the transaction
                        m_state.transactions.erase( std::begin( found));
                     }
                  }
                  else
                  {
                     // We don't have the transaction. This could be for two reasons:
                     // 1) We had it, but it was already prepared from another domain, and XA
                     // optimization kicked in ("read only") and the transaction was done
                     // 2) casual have made some optimizations.
                     // Either way, we don't have it, so we just reply that it has been prepared with "read only"

                     common::log::line( log, message.trid, " either does not exists (longer) in this domain or there are no resources involved - action: send prepare-reply (read only)");

                     local::send::read_only( message);
                  }
               }

               Directive Prepare::handle( message_type& message, Transaction& transaction)
               {
                  Trace trace{ "transaction::handle::domain::Prepare::handle"};

                  common::log::line( log, "message: ", message);
                  common::log::line( verbose::log, "transaction: ", transaction);

                  // We can only get this message if a 'user commit' has
                  // been invoked somewhere.
                  //
                  // The transaction can only be in one of two states.
                  //
                  // 1) The prepare phase has already started, either by this domain
                  //    or another domain, either way the work has begun, and we don't
                  //    need to do anything for this particular request.
                  //
                  // 2) Some other domain has received a 'user commit' and started the
                  //    prepare phase. This domain should carry out the request on behalf
                  //    of the other domain.

                  if( transaction.implementation)
                  {
                     // state 1:
                     //
                     // The commit/rollback phase has already started, we send
                     // read only and let the phase take it's course
                     local::send::read_only( message);
                  }
                  else
                  {
                     // state 2:

                     transaction.implementation = local::dispatch::remote::get();

                     switch( transaction.stage())
                     {
                        case Transaction::Resource::Stage::involved:
                        {
                           // We're in the second state. We change the owner to the inbound gateway that
                           // sent the request, so we know where to send the accumulated reply.

                           prepare_remote_owner( transaction, message);

                           if( transaction.resource_count() == 0)
                           {
                              local::send::read_only( message);
                              return Directive::remove_transaction;
                           }

                           local::send::resource::request< common::message::transaction::resource::prepare::Request>(
                              m_state,
                              transaction,
                              Transaction::Resource::filter::stage( Transaction::Resource::Stage::involved),
                              Transaction::Resource::Stage::prepare_requested,
                              message.flags
                           );

                           break;
                        }
                        case Transaction::Resource::Stage::not_involved:
                        {
                           local::send::read_only( message);
                           return Directive::remove_transaction;
                        }
                        default:
                        {
                           common::log::line( common::log::category::error,  transaction.stage(), " global: ", transaction.global, " unexpected transaction stage");
                           common::log::line( common::log::category::verbose::error, "transaction: ", transaction);

                           break;
                        }
                     }
                  }
                  return Directive::keep_transaction;
               }

               void Commit::operator () ( message_type& message)
               {
                  Trace trace{ "transaction::handle::domain::commit request"};

                  common::log::line( log, "message: ", message);

                  // Find the transaction
                  auto found = common::algorithm::find( m_state.transactions, message.trid);

                  if( found)
                  {
                     if( Commit::handle( message, *found) == Directive::remove_transaction)
                     {
                        // We remove the transaction
                        m_state.transactions.erase( std::begin( found));
                     }
                  }
                  else
                  {
                     // It has to be a one phase commit optimization.
                     if( ! message.flags.exist( common::flag::xa::Flag::one_phase))
                     {
                        auto reply = local::transform::reply( message);
                        reply.state = common::code::xa::protocol;
                        reply.trid = message.trid;
                        reply.resource = message.resource;

                        local::send::reply( std::move( reply), message.process);
                        return;
                     }

                     common::log::line( log, message.trid, " either does not exists (longer) in this domain or there are no resources involved - action: send commit-reply (read only)");
                     local::send::read_only( message);
                  }
               }

               Directive Commit::handle( message_type& message, Transaction& transaction)
               {
                  Trace trace{ "transaction::handle::domain::Commit::handle"};

                  transaction.correlation = message.correlation;

                  common::log::line( log, "message: ", message);
                  common::log::line( verbose::log, "transaction: ", transaction);

                  if( transaction.implementation)
                  {
                     // We've completed the prepare stage, now it's time for the commit stage

                     local::send::resource::request< common::message::transaction::resource::commit::Request>(
                        m_state,
                        transaction,
                        Transaction::Resource::filter::stage( Transaction::Resource::Stage::prepare_replied),
                        Transaction::Resource::Stage::commit_requested,
                        message.flags
                     );
                  }
                  else
                  {
                     // It has to be a one phase commit optimization.
                     if( ! message.flags.exist( common::flag::xa::Flag::one_phase))
                     {
                        auto reply = local::transform::reply( message);
                        reply.state = common::code::xa::protocol;
                        reply.resource = message.resource;

                        local::send::reply( std::move( reply), message.process);
                        return Directive::keep_transaction;
                     }

                     transaction.implementation = local::dispatch::one::phase::commit::remote::get();

                     prepare_remote_owner( transaction, message);

                     switch( transaction.resource_count())
                     {
                        case 0:
                        {
                           common::log::line( log, transaction.global, " - no relevant resources involved");
                           local::send::read_only( message);

                           return Directive::remove_transaction;
                        }
                        case 1:
                        {
                           common::log::line( log, transaction.global, " - one resource involved");
                           local::send::resource::request< common::message::transaction::resource::commit::Request>(
                              m_state,
                              transaction,
                              Transaction::Resource::filter::stage( Transaction::Resource::Stage::involved),
                              Transaction::Resource::Stage::commit_requested,
                              message.flags
                           );
                           break;
                        }
                        default:
                        {
                           common::log::line( log, transaction.global, " - more than one resource involved");
                           local::send::resource::request< common::message::transaction::resource::prepare::Request>(
                              m_state,
                              transaction,
                              Transaction::Resource::filter::stage( Transaction::Resource::Stage::involved),
                              Transaction::Resource::Stage::prepare_requested
                           );
                           break;
                        }
                     }
                  }
                  return Directive::keep_transaction;;
               }

               void Rollback::operator () ( message_type& message)
               {
                  Trace trace{ "transaction::handle::domain::rollback request"};

                  common::log::line( log, "message: ", message);

                  // Find the transaction
                  auto found = common::algorithm::find( m_state.transactions, message.trid);

                  if( found)
                  {
                     if( Rollback::handle( message, *found) == Directive::remove_transaction)
                     {
                        // We remove the transaction
                        m_state.transactions.erase( std::begin( found));
                     }
                  }
                  else
                  {
                     common::log::line( log, message.trid, " either does not exists (longer) in this domain or there are no resources involved - action: send rollback-reply (XA_OK)");
                     local::send::xa_result( message, common::code::xa::ok);
                  }
               }

               Directive Rollback::handle( message_type& message, Transaction& transaction)
               {
                  Trace trace{ "transaction::handle::domain::Rollback::handle"};

                  using Stage = Transaction::Resource::Stage;

                  switch( transaction.stage())
                  {
                     case Stage::involved:
                     {
                        // remote 'explicit' rollback

                        transaction.implementation = local::dispatch::remote::get();

                        prepare_remote_owner( transaction, message);

                        if( transaction.resource_count() == 0)
                        {
                           local::send::read_only( message);
                           return Directive::remove_transaction;
                        }

                        local::send::resource::request< common::message::transaction::resource::rollback::Request>(
                           m_state,
                           transaction,
                           []( auto& r){ return r.stage == Stage::involved;},
                           Stage::rollback_requested,
                           message.flags
                        );

                        break;
                     }
                     case Stage::prepare_replied:
                     {
                        // We are in a prepare phase, and owner TM has decided to rollback.

                        local::send::resource::request< common::message::transaction::resource::rollback::Request>(
                           m_state,
                           transaction,
                           []( auto& r){ return r.stage == Stage::prepare_replied;},
                           Stage::rollback_requested,
                           message.flags
                        );

                        break;
                     }
                     case Transaction::Resource::Stage::rollback_requested:
                     {
                        local::send::read_only( message);
                        break;
                     }
                     case Transaction::Resource::Stage::not_involved:
                     {
                        local::send::read_only( message);
                        return Directive::remove_transaction;
                     }
                     default:
                     {
                        common::log::line( common::log::category::error, common::code::xa::protocol, ' ', transaction.global, " unexpected transaction stage");
                        common::log::line( common::log::category::verbose::error, "transaction: ", transaction);
                        local::send::xa_result( message, common::code::xa::protocol);

                     }

                  }

                  return Directive::keep_transaction;
               }

            } // domain

            namespace resource
            {
               namespace reply
               {
                  template struct Wrapper< basic_prepare>;
                  template struct Wrapper< basic_commit>;
                  template struct Wrapper< basic_rollback>;

               } // reply
            }

            namespace local
            {
               // this is a better _idiom_ to create handlers. Less boilerplate
               // and local scope.
               // TODO: maintainence - convert all the above to this _idiom_,
               namespace
               {
                  namespace resource
                  {
                     namespace configuration
                     {
                        auto request( State& state)
                        {
                           return [&state]( const common::message::transaction::resource::configuration::Request& message)
                           {
                              Trace trace{ "transaction::handle::local::resource::configuration::request"};
                              common::log::line( log, "message: ", message);

                              auto reply = common::message::reverse::type( message);

                              {
                                 auto& resource = state.get_resource( message.id);
                                 reply.resource.id = message.id;
                                 reply.resource.key = resource.key;
                                 reply.resource.openinfo = resource.openinfo;
                                 reply.resource.closeinfo = resource.closeinfo;
                              }

                              ipc::optional::send( message.process.ipc, reply);
                           };

                        }
                     } // configuration

                     auto ready( State& state)
                     {
                        return [&state]( const common::message::transaction::resource::Ready& message)
                        {
                           Trace trace{ "transaction::handle::local::resource::ready"};
                           common::log::line( log, "message: ", message);

                           auto& instance = state.get_instance( message.id, message.process.pid);
                           instance.process = message.process;
                           local::instance::done( state, instance);
                        };
                     }

                  } // resource
               } // <unnamed>
            } // local

            namespace startup
            {
               dispatch_type handlers( State& state)
               {
                  return ipc::device().handler(
                     common::message::handle::defaults( ipc::device()),
                     manager::handle::process::Exit{ state},
                     local::resource::configuration::request( state),
                     local::resource::ready( state)
                   );
               }

            } // startup

            dispatch_type handlers( State& state)
            {
               return ipc::device().handler(
                  common::message::handle::defaults( ipc::device()),
                  common::event::listener( handle::process::Exit{ state}),
                  handle::Commit{ state},
                  handle::Rollback{ state},
                  handle::resource::Involved{ state},
                  handle::resource::Lookup{ state},
                  local::resource::configuration::request( state),
                  local::resource::ready( state),
                  handle::resource::reply::Prepare{ state},
                  handle::resource::reply::Commit{ state},
                  handle::resource::reply::Rollback{ state},
                  handle::external::Involved{ state},
                  handle::domain::Prepare{ state},
                  handle::domain::Commit{ state},
                  handle::domain::Rollback{ state},
                  common::server::handle::admin::Call{
                     manager::admin::services( state),
                     ipc::device().error_handler()}
               );
            }
         } // handle
      } // manager
   } // transaction
} // casual
