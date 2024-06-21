//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "transaction/manager/handle.h"
#include "transaction/manager/action.h"
#include "transaction/common.h"
#include "transaction/manager/admin/server.h"

#include "casual/assert.h"

#include "common/message/dispatch/handle.h"
#include "common/event/listen.h"
#include "common/event/send.h"
#include "common/server/handle/call.h"

#include "common/code/raise.h"
#include "common/code/convert.h"
#include "common/communication/instance.h"
#include "common/communication/ipc/flush/send.h"

#include "configuration/message.h"


namespace casual
{
   namespace transaction::manager::ipc
   {
      common::communication::ipc::inbound::Device& device()
      {
         return common::communication::ipc::inbound::device();
      }
   }

   namespace transaction::manager::handle
   {
      namespace local
      {
         namespace
         {
            namespace detail
            {
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

                     if( auto branch = common::algorithm::find( transaction->branches, message.trid))
                        branch->involve( involved);
                     else
                        transaction->branches.emplace_back( message.trid).involve( involved);

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
                  state::transaction::Branch& find_or_add( State& state, M&& message)
                  {
                     // Find the transaction
                     if( auto transaction = common::algorithm::find( state.transactions, message.trid))
                     {
                        // try find the branch
                        auto branch = common::algorithm::find( transaction->branches, message.trid);

                        if( branch)
                           return *branch;

                        return transaction->branches.emplace_back( message.trid);
                     }

                     return state.transactions.emplace_back( message.trid).branches.back();
                  }

               } // branch

               namespace persist
               {
                  void send( State& state)
                  {
                     // persist transaction log   
                     state.persistent.log.persist();

                     state.persistent.replies.send( state.multiplex);
                  }


                  namespace batch
                  {
                     void send( State& state)
                     {  
                        // check if we've reach our "batch-limit", if so, persist and send replies
                        if( state.persistent.replies.size() >= platform::batch::transaction::persistence)
                           detail::persist::send( state);  
                     }
                     
                  } // batch


                  
               } // persist

               namespace send::persist
               {
                  template< typename M>
                  void reply( State& state, M&& message, const common::process::Handle& target)
                  {
                     Trace trace{ "transaction::manager::handle::local::detail::send::persist::reply"};
                     common::log::line( verbose::log, "message: ", message);

                     state.persistent.replies.add( target, std::forward< M>( message));
                     detail::persist::batch::send( state);
                  }
                  
               } // send::persist  

               namespace accumulate
               {
                  //! Tries to normalize the xa code based on the accumulated replies, what type the reply is, if any 
                  //! has failed.
                  template< typename R, typename O>
                  common::code::xa code( R&& replies, O&& outcome, common::code::xa code = common::code::xa::read_only)
                  {
                     using outcome_state = decltype( common::range::front( outcome).state);

                     static constexpr auto normalize_code = []( auto& replies, auto code)
                     {
                        return common::algorithm::accumulate( replies, code, []( auto code, auto& reply)
                        {
                           return common::code::severest( code, reply.state);
                        });
                     };

                     auto at_least_one_failed = common::algorithm::any_of( outcome, []( auto& outcome){ return outcome.state == outcome_state::failed;});

                     // If there is only one resource involved, it cant result in "hazard".
                     if( std::size( outcome) == 1 && at_least_one_failed)
                        return common::code::severest( common::code::xa::resource_fail, code);

                     if constexpr( concepts::range_with_value< R, common::message::transaction::resource::commit::Reply>)
                     {
                        // we don't know what the state of the transaction is
                        if( at_least_one_failed)
                           return normalize_code( replies, common::code::severest( common::code::xa::heuristic_commit, code));

                        return normalize_code( replies, code);
                     }

                     if constexpr( concepts::range_with_value< R, common::message::transaction::resource::rollback::Reply>)
                     {
                        // we don't know what the state of the transaction is
                        if( at_least_one_failed)
                           return normalize_code( replies, common::code::severest( common::code::xa::heuristic_rollback, code));

                        return normalize_code( replies, code);
                     }

                     if constexpr( concepts::range_with_value< R, common::message::transaction::resource::prepare::Reply>)
                     {
                        if( at_least_one_failed)
                           return normalize_code( replies, common::code::severest( code, common::code::xa::resource_fail));

                        return normalize_code( replies, code);
                     }

                  }

               } // accumulate

               namespace branch
               {
                  template< typename M, typename I>
                  auto involved( State& state, const M& message, I&& involved)
                  {
                     auto transaction = common::algorithm::find( state.transactions, message.trid);

                     if( ! transaction)
                     {
                        state.transactions.emplace_back( message.trid);
                        auto end = std::end( state.transactions);
                        transaction = common::range::make( std::prev( end), end);
                     }

                     if( auto branch = common::algorithm::find( transaction->branches, message.trid))
                        branch->involve( involved);
                     else
                        transaction->branches.emplace_back( message.trid).involve( involved);

                     // remove all branches that is not associated with resources, if any.
                     transaction->purge();

                     return transaction;
                  }

                  template< typename M>
                  auto involved( State& state, const M& message)
                  {
                     return branch::involved( state, message, message.involved);
                  }

               } // branch

               namespace push::resource::error
               {
                  template< typename R>
                  auto reply( const R& request)
                  {
                     auto reply = common::message::reverse::type( request);
                     reply.resource = request.resource;
                     reply.trid = request.trid;
                     reply.state = common::code::xa::resource_fail;

                     // push it to our inbound
                     return ipc::device().push( reply);
                  }

               } // push::resource::error

               namespace create::resource::error
               {

                  template< typename Request>
                  auto callback( const Request&)
                  {
                     return []( const common::strong::ipc::id& ipc, const common::communication::ipc::message::Complete& complete)
                     {
                        // deserialize the request message
                        auto request = common::serialize::native::complete< Request>( complete);

                        common::log::error( common::code::xa::resource_fail, "failed to send ", request.type(), " to resource: ", request.resource, " ( ipc: ", ipc, "), trid: ", request.trid, " - action: emulate reply with ", common::code::xa::resource_fail);
                        push::resource::error::reply( request);
                     };
                  }
               } // create::resource::error

               namespace remove
               {
                  void transaction( State& state, common::transaction::global::id::range global)
                  {
                     Trace trace{ "transaction::manager::handle::local::detail::remove::transaction"};

                     state.persistent.log.remove( global);
                     
                     if( auto found = common::algorithm::find( state.transactions, global))
                     {
                        common::log::line( verbose::log, "remove: ", *found);
                        common::algorithm::container::erase( state.transactions, std::begin( found));

                        // other "managers" might have state associated with the transaction, send an event
                        // to help them get rid of it.
                        common::message::event::transaction::Disassociate event{ common::process::handle()};
                        event.gtrid = global;
                        common::event::send( state.multiplex, event);
                     }
                  }
                  
               } // remove

               namespace send
               {
                  template< typename M, typename C>
                  void reply( State& state, const M& message, C custom)
                  {
                     auto reply = common::message::reverse::type( message);
                     reply.trid = message.trid;
                     custom( reply);

                     state.multiplex.send( message.process.ipc, reply);
                  }

                  namespace resource
                  {
                     template< typename M>
                     common::strong::correlation::id request( State& state, M&& request) requires std::is_rvalue_reference_v< decltype( request)>
                     {
                        Trace trace{ "transaction::manager::handle::local::detail::send::resource::request"};
                        common::log::line( verbose::log, "request: ", request);

                        casual::assertion( request.resource, "invalid resource id: ", request.resource);
                        
                        if( state::resource::id::local( request.resource))
                        {
                           if( auto reserved = state.try_reserve( request.resource))
                              return state.multiplex.send( reserved->process.ipc, request, create::resource::error::callback( request));

                           common::log::line( log, "could not send to resource: ", request.resource, " - action: try later");
                           // (we know message is an rvalue, so it's going to be a move)
                           return state.pending.requests.add( request.resource, std::forward< M>( request));
                        }

                        if( auto resource = state.find_external( request.resource))
                           return state.multiplex.send( resource->process.ipc, request, create::resource::error::callback( request));
                        else
                        {
                           common::log::error( common::code::xa::resource_fail, "failed to find resource: ", request.resource, " for trid: ", request.trid, " - action: emulate reply with ", common::code::xa::resource_fail);
                           return push::resource::error::reply( request);
                        }
                     }

                  } // resource
                     
               } // send

               namespace coordinate
               {
                  namespace pending
                  {
                     template< typename Request, typename Branches, typename P>
                     auto branches( State& state, const Branches& branches, P pending, common::flag::xa::Flag flags = common::flag::xa::Flag::no_flags)
                     {
                        return common::algorithm::accumulate( branches, std::move( pending), [ &state, flags]( auto result, const auto& branch)
                        {
                           for( auto& resource : branch.resources)
                           {
                              Request request{ common::process::handle()};
                              request.trid = branch.trid;
                              request.resource = resource.id;
                              request.flags = flags;

                              result.emplace_back( send::resource::request( state, std::move( request)), resource.id);
                           }

                           return result;
                        });
                     }

                     template< typename Request, typename Replies, typename P>
                     auto replies( State& state, Replies&& replies, P pending, common::flag::xa::Flag flags = common::flag::xa::Flag::no_flags)
                     {
                        return common::algorithm::accumulate( replies, std::move( pending), [ &state, flags]( auto result, const auto& reply)
                        {
                           Request request{ common::process::handle()};
                           request.trid = reply.trid;
                           request.resource = reply.resource;
                           request.flags = flags;

                           result.emplace_back( send::resource::request( state, std::move( request)), reply.resource);

                           return result;
                        });
                     }
                     
                  } // pending


                  struct Destination
                  {
                     common::process::Handle process;
                     common::strong::correlation::id correlation;
                     common::strong::execution::id execution;
                     common::strong::resource::id resource;
                  };

                  template< typename M>
                  auto destination( const M& message)
                  {
                     
                     if constexpr( concepts::any_of< M, common::message::transaction::commit::Request, common::message::transaction::rollback::Request>)
                        // local "user" commit/rollback request has no specific resource (of course)
                        return Destination{ message.process, message.correlation, message.execution, common::strong::resource::id{}};
                     else
                        // We need to keep the _origin resource_ for the reply later
                        return Destination{ message.process, message.correlation, message.execution, message.resource};
                  }

                  namespace create
                  {
                     template< typename Reply>
                     auto reply( const common::transaction::ID& trid, const detail::coordinate::Destination& destination, common::code::xa code)
                     {
                        // casual (right now) classifies _invalid_xid_ (no-tran) as _not an error_, and set read_only.
                        // We don't really know (?) how rm:s respond if they have been associated with a trid, and no work from the 
                        // user has been done.
                        if( code == decltype( code)::invalid_xid)
                           code = decltype( code)::read_only;

                        Reply result{ common::process::handle()};
                        result.trid = trid;
                        result.correlation = destination.correlation;
                        result.execution = destination.execution;
                        
                        // local transaction instigator replies need to convert to tx state
                        if constexpr( concepts::any_of< Reply, common::message::transaction::commit::Reply, common::message::transaction::rollback::Reply>)
                           result.state = common::code::convert::to::tx( code);
                        else
                        {
                           result.state = code;
                           result.resource = destination.resource;
                        }

                        return result;
                     }

                  } // create


                  template< typename Reply, typename Pending>
                  auto commit( State& state, Pending pending, const common::transaction::ID& origin, detail::coordinate::Destination destination, common::code::xa code = common::code::xa::read_only)
                  {
                     // note: everything captured needs to by value (besides State if used)
                     state.coordinate.commit( std::move( pending), [ &state, origin, destination, code]( auto&& replies, auto&& outcome)
                     {
                        Trace trace{ "transaction::manager::handle::local::detail::coordinate::commit"};
                        common::log::line( verbose::log, "replies: ", replies, ", outcome: ", outcome);

                        auto reply = create::reply< Reply>( origin, destination, detail::accumulate::code( replies, outcome, code));

                        if constexpr( concepts::any_of< Reply, common::message::transaction::commit::Reply>)
                           reply.stage = decltype( reply.stage)::commit;

                        common::log::line( verbose::log, "reply: ", reply);   
                        state.multiplex.send( destination.process.ipc, reply);

                        remove::transaction( state, common::transaction::id::range::global( origin));
                     });
                  }

                  template< typename Reply, typename Pending>
                  auto rollback( State& state, Pending pending, const common::transaction::ID& origin, detail::coordinate::Destination destination, common::code::xa code = common::code::xa::read_only)
                  {
                     // note: everything captured needs to by value (besides State if used)
                     state.coordinate.rollback( std::move( pending), [ &state, origin, destination, code]( auto&& replies, auto&& outcome)
                     {
                        Trace trace{ "transaction::manager::handle::local::detail::coordinate::rollback"};
                        common::log::line( verbose::log, "replies: ", replies, ", outcome: ", outcome, ", code: ", code);
                        
                        auto reply = create::reply< Reply>( origin, destination, detail::accumulate::code( replies, outcome, code));

                        // check if we are the instigator for rollback (a process has _died_, and such)
                        if( destination.process == common::process::handle())
                        {
                           common::log::line( verbose::log, "local rollback - reply: ", reply);
                           if( reply.state == decltype( reply.state)::ok)
                              remove::transaction( state, common::transaction::id::range::global( origin));
                           else
                              common::log::line( common::log::category::error, reply.state, " rollback from transaction-manager failed - action: keep transaction");
                           return;
                        }

                        if constexpr( concepts::any_of< Reply, common::message::transaction::commit::Reply>)
                           reply.stage = decltype( reply.stage)::rollback;

                        common::log::line( verbose::log, "reply: ", reply);
                        state.multiplex.send( destination.process.ipc, reply);

                        remove::transaction( state, common::transaction::id::range::global( origin));
                     });
                  }

                  template< typename Reply, typename Pending>
                  auto prepare( State& state, Pending pending, const common::transaction::ID& origin, detail::coordinate::Destination destination)
                  {
                     // note: everything captured needs to be by value (besides State if used)
                     state.coordinate.prepare( std::move( pending), [ &state, origin, destination]( auto&& replies, auto&& outcome)
                     {
                        Trace trace{ "transaction::manager::handle::local::detail::coordinate::prepare"};
                        common::log::line( verbose::log, "replies: ", replies, ", outcome: ", outcome);

                        auto transaction = common::algorithm::find( state.transactions, origin);

                        if( ! transaction)
                        {
                           using namespace common;
                           common::log::error( common::code::casual::invalid_semantics, " prepare callback - failed to find transaction 'context' for: ", origin, " - action: ignore");
                           common::log::verbose::error( common::code::casual::invalid_semantics, "replies: ", replies, ", outcome: ", outcome);

                           return;
                        }

                        // some sanity checks
                        {
                           if( transaction->stage != state::transaction::Stage::prepare)
                              common::log::error( common::code::casual::invalid_semantics, "transaction is not in prepare phase - actual: ", transaction->stage, " - action: ignore");
                        }

                        // we're done with the prepare phase
                        transaction->stage = state::transaction::Stage::post_prepare;

                        const auto code = detail::accumulate::code( replies, outcome);
                        common::log::line( verbose::log, "code: ", code);

                        // filter away all read-only replies
                        auto [ active, read_only] = common::algorithm::partition( replies, []( auto& reply){ return reply.state != common::code::xa::read_only;});
                        common::log::line( verbose::log, "read_only: ", read_only);

                        // purge/remove all resources that is read_only for each branch, and if a branch has 0 resources, remove it.
                        transaction->purge( read_only);
                        common::log::line( verbose::log, "transaction: ", *transaction);

                        if( code == common::code::xa::read_only)
                        {
                           // we can send the reply directly
                           auto reply = create::reply< Reply>( origin, destination, code);
                           
                           if constexpr( concepts::any_of< Reply, common::message::transaction::commit::Reply>)
                              reply.stage = decltype( reply.stage)::commit;
                           
                           state.multiplex.send( destination.process.ipc, reply);

                           // we don't need to keep state of this transaction any more
                           remove::transaction( state, transaction->global.range());
                        }
                        else if( code == common::code::xa::ok)
                        {
                           // Prepare has gone ok. Log persistent state
                           state.persistent.log.prepare( *transaction);
                           
                           // if we act as a resource, we just reply after persistent write and let upstream TM orchestrate.
                           if constexpr( concepts::any_of< Reply, common::message::transaction::resource::prepare::Reply>)
                           {
                              auto reply = create::reply< Reply>( origin, destination, code);
                              detail::send::persist::reply( state, std::move( reply), destination.process);
                              return;
                           }
                           
                           // add _commit-prepare_ message to persistent reply, if reply is the commit reply type
                           if constexpr( concepts::any_of< Reply, common::message::transaction::commit::Reply>)
                           {
                              auto reply = create::reply< Reply>( origin, destination, code);
                              reply.stage = decltype( reply.stage)::prepare;

                              detail::send::persist::reply( state, std::move( reply), destination.process);
                           }

                           // we start the commit stage
                           transaction->stage = state::transaction::Stage::commit;

                           // we try to commit all active resources.
                           auto pending = detail::coordinate::pending::replies< common::message::transaction::resource::commit::Request>( 
                              state, active, state.coordinate.commit.empty_pendings());

                           detail::coordinate::commit< Reply>( state, std::move( pending), origin, destination, code);
                        }
                        else
                        {
                           // if we act as a resource, we just reply the "error" and let upstream TM orchestrate.
                           if constexpr( concepts::any_of< Reply, common::message::transaction::resource::prepare::Reply>)
                           {
                              auto reply = create::reply< Reply>( origin, destination, code);
                              state.multiplex.send( destination.process.ipc, reply);
                              return;
                           }

                           // we start the rollback stage
                           transaction->stage = state::transaction::Stage::rollback;

                           // we try to rollback all active resources, if any.
                           auto pending = detail::coordinate::pending::replies< common::message::transaction::resource::rollback::Request>( 
                              state, active, state.coordinate.rollback.empty_pendings());

                           detail::coordinate::rollback< Reply>( state, std::move( pending), origin, destination, code);
                        }
                     });
                  }

               } // coordinate

            } // detail


            namespace commit
            {  
               auto request( State& state)
               {
                  return [&state]( const common::message::transaction::commit::Request& message)
                  {
                     Trace trace{ "transaction::manager::handle::local::commit::request"};
                     common::log::line( log, "message: ", message);

                     auto transaction = detail::branch::involved( state, message);
                     common::log::line( verbose::log, "transaction: ", *transaction);

                     if( transaction->stage > state::transaction::Stage::involved)
                     {
                        // transaction is not in the correct 'stage'
                        detail::send::reply( state, message, []( auto& reply)
                        { 
                           reply.state = decltype( reply.state)::protocol;
                           reply.stage = decltype( reply.stage)::commit;
                        });
                        return;
                     }

                     transaction->owner = message.process;

                     // Only the owner of the transaction can fiddle with the transaction ?

                     switch( transaction->resource_count())
                     {
                        case 0:
                        {
                           common::log::line( log, transaction->global, " no resources involved: ");

                           detail::send::reply( state, message, []( auto& reply)
                           { 
                              reply.state = decltype( reply.state)::ok;
                              reply.stage = decltype( reply.stage)::commit;
                           });

                           // we can remove this transaction
                           detail::remove::transaction( state, transaction->global.range());

                           break;
                        }
                        case 1:
                        {
                           // Only one resource involved, we do a one-phase-commit optimization.
                           common::log::line( log, "global: ", transaction->global, " - only one resource involved");

                           // start the commit phase directly
                           transaction->stage = state::transaction::Stage::commit;

                           auto pending = detail::coordinate::pending::branches< common::message::transaction::resource::commit::Request>( 
                              state, transaction->branches, state.coordinate.commit.empty_pendings(), common::flag::xa::Flag::one_phase);

                           detail::coordinate::commit< common::message::transaction::commit::Reply>( 
                              state, std::move( pending), message.trid, detail::coordinate::destination( message));

                           break;
                        }
                        default:
                        {
                           // More than one resource involved, we do the prepare stage
                           common::log::line( log, "global: ", transaction->global, " more than one resource involved");
                           
                           // start the prepare phase
                           transaction->stage = state::transaction::Stage::prepare;

                           auto pending = detail::coordinate::pending::branches< common::message::transaction::resource::prepare::Request>( 
                              state, transaction->branches, state.coordinate.prepare.empty_pendings());

                           detail::coordinate::prepare< common::message::transaction::commit::Reply>( 
                              state, std::move( pending), message.trid, detail::coordinate::destination( message));

                           break;
                        }
                     }

                  };
               }

            } // commit

            namespace rollback
            {
               auto request( State& state)
               {
                  return [&state]( const common::message::transaction::rollback::Request& message)
                  {
                     Trace trace{ "transaction::manager::handle::local::rollback::request"};
                     common::log::line( verbose::log, "message: ", message);

                     auto transaction = detail::branch::involved( state, message);
                     common::log::line( verbose::log, "transaction: ", *transaction);

                     // start the rollback phase, directly
                     transaction->stage = state::transaction::Stage::rollback;
                     transaction->owner = message.process;

                     auto pending = detail::coordinate::pending::branches< common::message::transaction::resource::rollback::Request>( 
                        state, transaction->branches, state.coordinate.rollback.empty_pendings());

                     detail::coordinate::rollback< common::message::transaction::rollback::Reply>( 
                        state, std::move( pending), message.trid, detail::coordinate::destination( message));
                  };
               }

            } // rollback

            namespace resource
            {
               namespace detail
               {
                  namespace instance
                  {
                     void ready( State& state, state::resource::proxy::Instance& instance)
                     {
                        Trace trace{ "transaction::manager::handle::local::resource::detail::instance::done"};

                        if( auto request = state.pending.requests.next( instance.id))
                        {
                           // We got a pending request for this resource, let's oblige
                           if( state.multiplex.send( instance.process.ipc, std::move( request.complete)))
                              instance.reserve( request.created);
                           else
                              common::log::line( common::log::category::error, "the instance: ", instance , " - does not seem to be running");
                        }
                     }

                     template< typename M>
                     void replied( State& state, const M& message)
                     {
                        if( state::resource::id::local( message.resource))
                        {
                           // The resource is a local resource proxy, and it's done, and ready for more work
                           auto& instance = state.get_instance( message.resource, message.process.pid);
                           {
                              instance.unreserve( message.statistics);
                              instance::ready( state, instance);
                           }
                        } 
                     }

                  } // instance
                  
               } // detail


               namespace involved
               {
                  auto request( State& state)
                  {
                     return [ &state]( common::message::transaction::resource::involved::Request& message)
                     {
                        Trace trace{ "transaction::manager::handle::local::resource::involved::request"};
                        common::log::line( verbose::log, "message: ",  message);

                        auto& branch = local::detail::branch::find_or_add( state, message);

                        auto branch_resources = common::algorithm::transform( branch.resources, []( auto& resource){ return resource.id;});

                        if( message.reply)
                        {
                           // prepare and send the reply
                           auto reply = common::message::reverse::type( message);
                           reply.involved = branch_resources;
                           state.multiplex.send( message.process.ipc, reply);
                        }

                        // partition what we don't got since before
                        auto involved = std::get< 1>( common::algorithm::intersection( message.involved, branch_resources));

                        // partition the new involved based on which we've got configured resources for
                        auto [ known, unknown] = common::algorithm::partition( involved, [&]( auto& resource)
                        {
                           return common::predicate::boolean( common::algorithm::find( state.resources, resource));
                        });

                        // add new involved resources, if any.
                        branch.involve( known);

                        // if we've got some resources that we don't know about.
                        // TODO: should we set the transaction to rollback only?
                        if( unknown)
                        {
                           common::log::line( common::log::category::error, "unknown resources: ", unknown, " - action: discard");
                           common::log::line( common::log::category::verbose::error, "trid: ", message.trid);
                        }
                     };
                  } 
                  
               } // involved

               namespace prepare
               {
                  auto reply( State& state)
                  {
                     return [&state]( const common::message::transaction::resource::prepare::Reply& message)
                     {
                        Trace trace{ "transaction::manager::handle::local::resource::prepare::reply"};
                        common::log::line( verbose::log, "message: ", message);

                        detail::instance::replied( state, message);
                        state.coordinate.prepare( message);
                     };
                  }

               } // prepare

               namespace commit
               {
                  auto reply( State& state)
                  {
                     return [&state]( const common::message::transaction::resource::commit::Reply& message)
                     {
                        Trace trace{ "transaction::manager::handle::local::resource::commit::reply"};
                        common::log::line( verbose::log, "message: ", message);

                        detail::instance::replied( state, message);
                        state.coordinate.commit( message);
                     };
                  }
                  
               } // commit

               namespace rollback
               {
                  auto reply( State& state)
                  {
                     return [&state]( const common::message::transaction::resource::rollback::Reply& message)
                     {
                        Trace trace{ "transaction::manager::handle::local::resource::rollback::reply"};
                        common::log::line( verbose::log, "message: ", message);

                        detail::instance::replied( state, message);
                        state.coordinate.rollback( message);
                     };
                  }

               } // rollback

               namespace external
               {
                  namespace detail
                  {
                     namespace send
                     {
                        template< typename M, typename C>
                        void reply( State& state, const M& message, C custom)
                        {
                           auto reply = common::message::reverse::type( message);
                           reply.trid = message.trid;
                           reply.resource = message.resource;
                           custom( reply);

                           state.multiplex.send( message.process.ipc, reply);
                        }

                        namespace read::only
                        {
                           template< typename M>
                           auto reply( State& state, const M& message)
                           {
                              detail::send::reply( state, message, []( auto& reply)
                              {
                                 reply.statistics.start = platform::time::clock::type::now();
                                 reply.statistics.end = reply.statistics.start;
                                 reply.state = decltype( reply.state)::read_only;
                              });
                           }
                        } // read::only
                     } // send
                  } // detail


                  auto instance( State& state)
                  {
                     return [ &state]( common::message::transaction::resource::external::Instance& message)
                     {
                        Trace trace{ "transaction::manager::handle::local::resource::external::instance"};
                        common::log::line( log, "message: ", message);

                        state::resource::external::instance::add( state, std::move( message));
                     };
                  }


                  auto involved( State& state)
                  {
                     return [ &state]( common::message::transaction::resource::external::Involved& message)
                     {
                        Trace trace{ "transaction::manager::handle::local::resource::external::involved"};
                        common::log::line( log, "message: ", message);

                        auto id = state::resource::external::instance::id( state, message.process);

                        auto& transaction = *local::detail::transaction::find_or_add_and_involve( state, message, id);
                        common::log::line( verbose::log, "transaction: ", transaction);
                     };
                  }

                  namespace prepare
                  {
                     auto request( State& state)
                     {
                        return [&state]( const common::message::transaction::resource::prepare::Request& message)
                        {
                           Trace trace{ "transaction::manager::handle::local::resource::external::prepare::request"};
                           common::log::line( log, "message: ", message);

                           auto transaction = common::algorithm::find( state.transactions, message.trid);

                           if( ! transaction)
                           {
                              common::log::line( log, "failed to find trid: ", message.trid, " - action: reply with read-only");
                              detail::send::read::only::reply( state, message);
                              return;
                           }

                           common::log::line( log, "transaction: ", *transaction);

                           if( transaction->stage > state::transaction::Stage::involved)
                           {
                              common::log::line( log, "transaction stage is passed the _involved_ - stage: ", transaction->stage, " - action: reply with read-only");
                              detail::send::read::only::reply( state, message);
                              return;
                           }

                           if( transaction->resource_count() == 0)
                           {
                              common::log::line( log, transaction->global, " no resources involved: ");

                              detail::send::read::only::reply( state, message);

                              // We can remove this transaction
                              local::detail::remove::transaction( state, transaction->global.range());
                              return;
                           }

                           // we can't do any _one phase commit optimization_, since we're not the on in charge.
                           common::log::line( log, "global: ", transaction->global, " preparing");

                           // Start the prepare phase
                           transaction->stage = state::transaction::Stage::prepare;

                           auto pending = local::detail::coordinate::pending::branches< common::message::transaction::resource::prepare::Request>( 
                              state, transaction->branches, state.coordinate.prepare.empty_pendings());
                           
                           local::detail::coordinate::prepare< common::message::transaction::resource::prepare::Reply>( 
                              state, std::move( pending), message.trid, local::detail::coordinate::destination( message));

                        };
                     }
                  } // prepare

                  namespace commit
                  {
                     auto request( State& state)
                     {
                        return [&state]( const common::message::transaction::resource::commit::Request& message)
                        {
                           Trace trace{ "transaction::manager::handle::local::resource::external::commit::request"};
                           common::log::line( log, "message: ", message);

                           auto transaction = common::algorithm::find( state.transactions, message.trid);

                           if( ! transaction)
                           {
                              common::log::line( log, "failed to find trid: ", message.trid, " - action: reply with read-only");
                              detail::send::read::only::reply( state, message);
                              return;
                           }

                           common::log::line( log, "transaction: ", *transaction);
                           
                           // Three possibilities:
                           //  * We're in the post_prepare stage of the transaction ->  we start commit.
                           //  * We're in involved and the upstream TM request a one-phase optimization -> we start prepare.
                           //  * We're already in an active stage and the commit request is an "artifact" of topology complexity
                           if( transaction->stage == state::transaction::Stage::post_prepare)
                           {
                              // start the commit phase
                              transaction->stage = state::transaction::Stage::commit;

                              auto pending = local::detail::coordinate::pending::branches< common::message::transaction::resource::commit::Request>( 
                                 state, transaction->branches, state.coordinate.commit.empty_pendings());

                              local::detail::coordinate::commit< common::message::transaction::resource::commit::Reply>( 
                                 state, std::move( pending), message.trid, local::detail::coordinate::destination( message));
                           }
                           else if( transaction->stage == state::transaction::Stage::involved)
                           {
                              // this should be a one-phase optimization, if not, we log error and still commit.
                              // TODO: this should not matter, but does it?
                              if( ! common::flag::contains( message.flags, common::flag::xa::Flag::one_phase))
                                 common::log::error( common::code::casual::invalid_semantics, " resource commit request without TMONEPHASE, gtrid: ", transaction->global);

                              // start the prepare phase
                              transaction->stage = state::transaction::Stage::prepare;

                              auto pending = local::detail::coordinate::pending::branches< common::message::transaction::resource::prepare::Request>( 
                                 state, transaction->branches, state.coordinate.prepare.empty_pendings());
                           
                              local::detail::coordinate::prepare< common::message::transaction::resource::commit::Reply>( 
                                 state, std::move( pending), message.trid, local::detail::coordinate::destination( message));
                           }
                           else
                           {
                              // The transaction is already in "process", we assume this request is due to topology "complexity".
                              common::log::line( log, "transaction already in commit progress: ", *transaction, " - action: reply with read-only");
                              detail::send::read::only::reply( state, message);
                           }

                        };
                     }
                     
                  } // commit

                  namespace rollback
                  {
                     auto request( State& state)
                     {
                        return [&state]( const common::message::transaction::resource::rollback::Request& message)
                        {
                           Trace trace{ "transaction::manager::handle::local::resource::external::rollback::request"};
                           common::log::line( verbose::log, "message: ", message);

                           auto transaction = common::algorithm::find( state.transactions, message.trid);

                           if( ! transaction)
                           {
                              common::log::line( log, "failed to find trid: ", message.trid, " - action: reply with read-only");
                              detail::send::read::only::reply( state, message);
                              return;
                           }

                           common::log::line( verbose::log, "transaction: ", *transaction);

                           if( ! common::algorithm::compare::any( transaction->stage, state::transaction::Stage::involved, state::transaction::Stage::post_prepare))
                           {
                              common::log::line( verbose::log, "transaction is not in involved or post_prepare phase: ", *transaction, " - action: reply with read-only");
                              detail::send::read::only::reply( state, message);
                              return;
                           }
                           
                           transaction->owner = message.process;
                           
                           // start the rollback phase
                           transaction->stage = state::transaction::Stage::rollback; 
                           
                           auto pending = local::detail::coordinate::pending::branches< common::message::transaction::resource::rollback::Request>( 
                              state, transaction->branches, state.coordinate.rollback.empty_pendings());

                           local::detail::coordinate::rollback< common::message::transaction::resource::rollback::Reply>( 
                              state, std::move( pending), message.trid, local::detail::coordinate::destination( message));

                        };
                     }
                  } // rollback
               } // external


               namespace configuration
               {
                  auto request( State& state)
                  {
                     return [&state]( const common::message::transaction::resource::configuration::Request& message)
                     {
                        Trace trace{ "transaction::manager::handle::local::resource::configuration::request"};
                        common::log::line( log, "message: ", message);

                        auto reply = common::message::reverse::type( message);

                        {
                           auto& resource = state.get_resource( message.id);
                           reply.resource.id = message.id;
                           reply.resource.key = resource.configuration.key;
                           reply.resource.openinfo = resource.configuration.openinfo;
                           reply.resource.closeinfo = resource.configuration.closeinfo;
                        }

                        state.multiplex.send( message.process.ipc, reply);
                     };

                  }
               } // configuration

               auto ready( State& state)
               {
                  return [&state]( const common::message::transaction::resource::Ready& message)
                  {
                     Trace trace{ "transaction::manager::handle::local::resource::ready"};
                     common::log::line( log, "message: ", message);

                     auto& instance = state.get_instance( message.id, message.process.pid);
                     instance.process = message.process;
                     instance.state( state::resource::proxy::instance::State::idle);
                     detail::instance::ready( state, instance);
                  };
               }

            } // resource

            namespace inbound::branch
            {
               auto request( State& state)
               {
                  return [ &state]( const common::message::transaction::inbound::branch::Request& message)
                  {
                     Trace trace{ "transaction::manager::handle::local::inbound::branch::request"};
                     common::log::line( log, "message: ", message);

                     auto reply = common::message::reverse::type( message);

                     if( auto found = common::algorithm::find( state.transactions, message.gtrid))
                     {
                        common::log::line( log, "found: ", *found);

                        CASUAL_ASSERT( ! found->branches.empty());
                        reply.trid = common::range::front( found->branches).trid;
                     }
                     else
                     {
                        reply.trid = common::transaction::ID{ message.gtrid.range()};
                        auto& transaction = state.transactions.emplace_back( reply.trid);

                        common::log::line( log, "added: ", transaction);
                     }

                     state.multiplex.send( message.process.ipc, reply);
                  };
               }
               
            } // inbound::branch

            namespace event::process
            {
               auto exit( State& state)
               {
                  return [ &state]( const common::message::event::process::Exit& message)
                  {
                     Trace trace{ "transaction::manager::handle::local::process::exit"};
                     common::log::line( verbose::log, "message: ", message);

                     // Check if it's a resource proxy instance
                     if( state.remove_instance( message.state.pid))
                     {
                        state.multiplex.send( common::communication::instance::outbound::domain::manager::device(), message);
                        return;
                     }

                     // check external
                     common::algorithm::container::erase_if( state.externals, [ &state, pid = message.state.pid]( auto& resource)
                     {
                        if( resource.process.pid != pid)
                           return false;

                        common::log::line( verbose::log, "found external process exit: ", resource);
                        
                        state.coordinate.failed( resource.id);
                        state.multiplex.failed( resource.process.ipc);

                        return true;
                     });

                     auto trids = common::algorithm::accumulate( state.transactions, std::vector< common::transaction::ID>{}, [ pid = message.state.pid]( auto result, auto& transaction)
                     {
                        if( ! transaction.branches.empty() && transaction.branches.back().trid.owner().pid == pid)
                           result.push_back( transaction.branches.back().trid);

                        return result;
                     });

                     common::log::line( verbose::log, "trids: ", trids);

                     for( auto& trid : trids)
                     {
                        common::message::transaction::rollback::Request request;
                        request.process = common::process::handle();
                        request.trid = trid;

                        // This could change the state, that's why we don't do it directly in the loop above.
                        local::rollback::request( state)( request);
                     }


                  };
               }

            } // event::process

            namespace event::ipc
            {
               auto destroyed( State& state)
               {
                  return [ &state]( const common::message::event::ipc::Destroyed& message)
                  {
                     Trace trace{ "transaction::manager::handle::local::event::ipc::destroyed"};
                     common::log::line( verbose::log, "message: ", message);
                     
                     // this can't come from our own resource proxies, only external

                     if( auto found = common::algorithm::find( state.externals, message.process.ipc))
                     {
                        common::log::line( verbose::log, "failed: ", *found);

                        // fail associated transaction resource state, if any
                        for( auto& transaction : state.transactions)
                           transaction.failed( found->id);

                        // fail ongoing fan-outs for the rm, if any.
                        state.coordinate.failed( found->id);

                        common::algorithm::container::erase( state.externals, std::begin( found));
                     }

                     // fail pending sends to the destroyed destination
                     state.multiplex.failed( message.process.ipc);
                  };
               }
            } // event::ipc

            namespace configuration
            {
               namespace alias
               {
                  auto request( State& state)
                  {
                     return [&state]( const common::message::transaction::configuration::alias::Request& message)
                     {
                        Trace trace{ "transaction::manager::handle::local::configuration::alias::request"};
                        common::log::line( verbose::log, "message: ", message);

                        auto reply = state.configuration( message);
                        common::log::line( verbose::log, "reply: ", reply);

                        state.multiplex.send( message.process.ipc, reply);
                     };
                  }
               } // alias

               auto request( State& state)
               {
                  return [&state]( const casual::configuration::message::Request& message)
                  {
                     Trace trace{ "transaction::manager::handle::local::configuration::request"};
                     common::log::line( verbose::log, "message: ", message);

                     auto reply = common::message::reverse::type( message);
                     reply.model.transaction = state.configuration();
                     state.multiplex.send( message.process.ipc, reply);
                  };
               }

            } // configuration

            namespace active 
            {
               auto request( State& state)
               {
                  return [ &state]( common::message::transaction::active::Request& message)
                  {
                     Trace trace{ "transaction::manager::handle::local::active::request"};
                     common::log::line( verbose::log, "message: ", message);

                     auto reply = common::message::reverse::type( message);
                     reply.gtrids = std::move( message.gtrids);

                     // Keep all that is found/is active
                     common::algorithm::container::erase_if( reply.gtrids, [ &state]( auto& gtrid)
                     {
                        return ! common::algorithm::contains( state.transactions, gtrid);
                     });

                     common::log::line( verbose::log, "reply: ", reply);
                     if( ! std::empty( reply.gtrids))
                        common::log::line( verbose::log, "state.transactions: ", state.transactions);


                     state.multiplex.send( message.process.ipc, reply);
                  };
               }
            } // active

         } // <unnamed>
      } // local

      namespace process
      {
         void exit( const common::process::lifetime::Exit& exit)
         {
            Trace trace{ "transaction::manager::handle::process::exit"};
            common::log::line( verbose::log, "exit: ", exit);

            // push it to handle it later together with other process exit events
            ipc::device().push( common::message::event::process::Exit{ exit});
         }
      }

      namespace persist
      {
         void send( State& state)
         {
            Trace trace{ "transaction::manager::handle::persist:send"};

            // if we don't have any persistent replies, we don't need to persist
            if( ! state.persistent.replies.empty())
               local::detail::persist::send( state);
         }
            
      } // persist

      namespace startup
      {
         dispatch_type handlers( State& state)
         {
            return common::message::dispatch::handler( ipc::device(),
               common::message::dispatch::handle::defaults(),
               local::event::process::exit( state),
               local::resource::configuration::request( state),
               local::resource::ready( state)
               );
         }

      } // startup

      dispatch_type handlers( State& state)
      {
         return common::message::dispatch::handler( ipc::device(),
            common::message::dispatch::handle::defaults( state),
            common::event::listener( 
               local::event::process::exit( state),
               local::event::ipc::destroyed( state)),
            local::commit::request( state),
            local::rollback::request( state),
            local::resource::involved::request( state),
            local::configuration::alias::request( state),
            local::configuration::request( state),
            local::resource::configuration::request( state),
            local::resource::ready( state),
            local::resource::prepare::reply( state),
            local::resource::commit::reply( state),
            local::resource::rollback::reply( state),
            local::resource::external::instance( state),
            local::resource::external::involved( state),
            local::resource::external::prepare::request( state),
            local::resource::external::commit::request( state),
            local::resource::external::rollback::request( state),
            local::inbound::branch::request( state),
            local::active::request( state),
            common::server::handle::admin::Call{
               manager::admin::services( state)}
         );
      }

      void abort( State& state)
      {
         Trace trace{ "transaction::manager::handle::abort"};

         auto scale_down = [&]( auto& resource)
         { 
            common::exception::guard( [&](){
               resource.configuration.instances = 0;
               manager::action::resource::scale::instances( state, resource);
            }); 
         };

         common::algorithm::for_each( state.resources, scale_down);

         auto processes = state.processes();
         common::process::lifetime::wait( processes, std::chrono::milliseconds( processes.size() * 100));

      }
   } // transaction::manager::handle
} // casual
