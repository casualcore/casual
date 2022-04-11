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

#include "common/message/handle.h"
#include "common/event/listen.h"
#include "common/server/handle/call.h"

#include "common/code/raise.h"
#include "common/code/convert.h"
#include "common/communication/instance.h"
#include "common/communication/ipc/flush/send.h"

#include "configuration/message.h"

#include "domain/pending/message/send.h"


namespace casual
{
   namespace transaction
   {
      namespace manager
      {
         namespace ipc
         {
            common::communication::ipc::inbound::Device& device()
            {
               return common::communication::ipc::inbound::device();
            }
            
         } // ipc

         namespace handle
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

                           // we'll consume every message either way
                           auto persistent = std::exchange( state.persistent.replies, {});

                           common::log::line( verbose::log, "persistent: ", persistent);

                           auto direct_send = []( auto& message)
                           {
                              return common::message::pending::non::blocking::send( message);
                           };

                           // try send directly
                           auto pending = common::algorithm::remove_if( persistent, direct_send);
                        
                           common::log::line( verbose::log, "pending: ", pending);


                           auto pending_send = []( auto& message)
                           {
                              casual::domain::pending::message::send( message);
                           };
                           
                           // send failed non-blocking sends to pending
                           common::algorithm::for_each( pending, pending_send);
                        }


                        namespace batch
                        {
                           void send( State& state)
                           {  
                              // check if we've reach our "batch-limit", if so, persit and send replies
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
                           state.persistent.replies.emplace_back( std::move( message), target);
                           detail::persist::batch::send( state);
                        }
                        
                     } // send::persist  
                     namespace accumulate
                     {
                        template< typename R, typename O>
                        auto code( R&& range, O&& outcome, common::code::xa code = common::code::xa::read_only)
                        {  
                           // if there are 'failed' in outcome we have (??) to classify this as heuristic_hazard,
                           // since we don't know what could have happend...
                           auto is_failed = []( auto& outcome){ return outcome.state == decltype( outcome.state)::failed;};
                           code = common::algorithm::any_of( outcome, is_failed) ? common::code::xa::heuristic_hazard : code;

                           common::log::line( verbose::log, "code: ", code);

                           return state::code::priority::convert( common::algorithm::accumulate( range, state::code::priority::convert( code), []( auto result, auto& reply)
                           {
                              return std::min( result, state::code::priority::convert( reply.state));
                           }));
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

                     namespace send
                     {
                        template< typename M, typename C>
                        bool reply( const M& message, C custom)
                        {
                           auto reply = common::message::reverse::type( message);
                           reply.trid = message.trid;
                           custom( reply);

                           return common::predicate::boolean( common::communication::ipc::flush::optional::send( message.process.ipc, reply));
                        }

                        namespace resource
                        {
                           template< typename M>
                           auto request( State& state, M&& request)
                           {
                              casual::assertion( request.resource, "invalid resource id: ", request.resource);

                              if( state::resource::id::local( request.resource))
                              {
                                 if( auto found = state.idle( request.resource))
                                 {
                                    auto correlation = common::communication::ipc::flush::send( found->process.ipc, request);
                                    found->state( state::resource::Proxy::Instance::State::busy);
                                    found->metrics.requested = platform::time::clock::type::now();
                                    return correlation;
                                 }

                                 common::log::line( log, "could not send to resource: ", request.resource, " - action: try later");
                                 return state.pending.requests.emplace_back( request.resource, std::move( request)).message.correlation();
                              }

                              auto& resource = state.get_external( request.resource);
                              return common::communication::ipc::flush::send( resource.process.ipc, request);
                           }

                        } // resource
                           
                     } // send

                     namespace coordinate
                     {
                        namespace pending
                        {
                           template< typename Request, typename Branches, typename P>
                           auto branches( State& state, const Branches& branches, P pending, common::flag::xa::Flags flags = common::flag::xa::Flag::no_flags)
                           {
                              return common::algorithm::accumulate( branches, std::move( pending), [ &state, flags]( auto result, const auto& branch)
                              {
                                 for( auto& resource : branch.resources)
                                 {
                                    Request request{ common::process::handle()};
                                    request.trid = branch.trid;
                                    request.resource = resource.id;
                                    request.flags = flags;

                                    result.emplace_back( send::resource::request( state, request), resource.id);
                                 }

                                 return result;
                              });
                           }

                           template< typename Request, typename Replies, typename P>
                           auto replies( State& state, Replies&& replies, P pending, common::flag::xa::Flags flags = common::flag::xa::Flag::no_flags)
                           {
                              return common::algorithm::accumulate( replies, std::move( pending), [ &state, flags]( auto result, const auto& reply)
                              {
                                 Request request{ common::process::handle()};
                                 request.trid = reply.trid;
                                 request.resource = reply.resource;
                                 request.flags = flags;

                                 result.emplace_back( send::resource::request( state, request), reply.resource);

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
                           
                           if constexpr( common::traits::is::any_v< M, common::message::transaction::commit::Request, common::message::transaction::rollback::Request>)
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
                              if constexpr( common::traits::is::any_v< Reply, common::message::transaction::commit::Reply, common::message::transaction::rollback::Reply>)
                                 result.state = common::code::convert::to::tx( code);
                              else
                              {
                                 result.state = code;
                                 result.resource = destination.resource;
                              }

                              return result;
                           }

                        } // create

                        namespace remove
                        {
                           void transaction( State& state, common::transaction::id::range::range_type global)
                           {
                              state.persistent.log.remove( global);
                              if( auto found = common::algorithm::find( state.transactions, global))
                                 state.transactions.erase( std::begin( found));
                           }
                           
                        } // remove


                        template< typename Reply, typename Pending>
                        auto commit( State& state, Pending pending, const common::transaction::ID& origin, detail::coordinate::Destination destination, common::code::xa code = common::code::xa::read_only)
                        {
                           // note: everything captured needs to by value (besides State if used)
                           state.coordinate.commit( std::move( pending), [ &state, origin, destination, code]( auto replies, auto outcome)
                           {
                              Trace trace{ "transaction::manager::handle::local::detail::coordinate::commit"};
                              common::log::line( verbose::log, "replies: ", replies, ", outcome: ", outcome);

                              auto reply = create::reply< Reply>( origin, destination, detail::accumulate::code( replies, outcome, code));

                              if constexpr( common::traits::is::any_v< Reply, common::message::transaction::commit::Reply>)
                                 reply.stage = decltype( reply.stage)::commit;

                              common::log::line( verbose::log, "reply: ", reply);   
                              common::communication::ipc::flush::optional::send( destination.process.ipc, reply);

                              remove::transaction( state, common::transaction::id::range::global( origin));
                           });
                        }

                        template< typename Reply, typename Pending>
                        auto rollback( State& state, Pending pending, const common::transaction::ID& origin, detail::coordinate::Destination destination, common::code::xa code = common::code::xa::read_only)
                        {
                           // note: everything captured needs to by value (besides State if used)
                           state.coordinate.rollback( std::move( pending), [ &state, origin, destination, code]( auto replies, auto outcome)
                           {
                              Trace trace{ "transaction::manager::handle::local::detail::coordinate::rollback"};
                              common::log::line( verbose::log, "replies: ", replies, ", outcome: ", outcome);
                              
                              auto reply = create::reply< Reply>( origin, destination, detail::accumulate::code( replies, outcome, code));

                              if constexpr( common::traits::is::any_v< Reply, common::message::transaction::commit::Reply>)
                                 reply.stage = decltype( reply.stage)::rollback;

                              common::log::line( verbose::log, "reply: ", reply);
                              common::communication::ipc::flush::optional::send( destination.process.ipc, reply);

                              remove::transaction( state, common::transaction::id::range::global( origin));
                           });
                        }

                        template< typename Reply, typename Pending>
                        auto prepare( State& state, Pending pending, const common::transaction::ID& origin, detail::coordinate::Destination destination)
                        {
                           // note: everything captured needs to be by value (besides State if used)
                           state.coordinate.prepare( std::move( pending), [ &state, origin, destination]( auto replies, auto outcome)
                           {
                              Trace trace{ "transaction::manager::handle::local::detail::coordinate::prepare"};
                              common::log::line( verbose::log, "replies: ", replies, ", outcome: ", outcome);

                              auto transaction = common::algorithm::find( state.transactions, origin);
                              casual::assertion( transaction, "failed to find transaction 'context' for: ", origin);

                              // filter away all read-only replies
                              auto [ active, read_only] = common::algorithm::partition( replies, []( auto& reply){ return reply.state != decltype( reply.state)::read_only;});

                              // purge all read_only resources for each branch, and if a branch has 0 resources, remove it.
                              transaction->purge( read_only);

                              auto code = detail::accumulate::code( active, outcome);

                              if( code == decltype( code)::read_only)
                              {
                                 state.transactions.erase( std::begin( transaction));

                                 // we can send the reply directly
                                 auto reply = create::reply< Reply>( origin, destination, code);
                                 
                                 if constexpr( common::traits::is::any_v< Reply, common::message::transaction::commit::Reply>)
                                    reply.stage = decltype( reply.stage)::commit;
                                 
                                 common::communication::ipc::flush::optional::send( destination.process.ipc, reply);
                              }
                              else if( code == decltype( code)::ok)
                              {
                                 // Prepare has gone ok. Log persistant state
                                 state.persistent.log.prepare( *transaction);
                                 
                                 // add _commit-prepare_ message to persistent reply, if reply is the commit reply type
                                 if constexpr( common::traits::is::any_v< Reply, common::message::transaction::commit::Reply>)
                                 {
                                    auto reply = create::reply< Reply>( origin, destination, code);
                                    reply.stage = decltype( reply.stage)::prepare;

                                    detail::send::persist::reply( state, std::move( reply), destination.process);
                                 }

                                 // we try to commit all active resources.
                                 auto pending = detail::coordinate::pending::replies< common::message::transaction::resource::commit::Request>( 
                                    state, active, state.coordinate.commit.empty_pendings());

                                 detail::coordinate::commit< Reply>( state, std::move( pending), origin, destination, code);
                              }
                              else
                              {
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

                           if( transaction->stage > decltype( transaction->stage())::involved)
                           {
                              // transaction is not in the correct 'stage'
                              detail::send::reply( message, []( auto& reply)
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

                                 // We can remove this transaction
                                 state.transactions.erase( std::begin( transaction));

                                 detail::send::reply( message, []( auto& reply)
                                 { 
                                    reply.state = decltype( reply.state)::ok;
                                    reply.stage = decltype( reply.stage)::commit;
                                 });

                                 break;
                              }
                              case 1:
                              {
                                 // Only one resource involved, we do a one-phase-commit optimization.
                                 common::log::line( log, "global: ", transaction->global, " - only one resource involved");
                                 transaction->stage = decltype( transaction->stage())::commit;

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
                                 transaction->stage = decltype( transaction->stage())::prepare;

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

                           transaction->stage = decltype( transaction->stage())::rollback;
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
                           void done( State& state, state::resource::Proxy::Instance& instance)
                           {
                              Trace trace{ "transaction::manager::handle::local::resource::detail::instance::done"};

                              instance.state( state::resource::Proxy::Instance::State::idle);

                              if( auto request = common::algorithm::find( state.pending.requests, instance.id))
                              {
                                 // We got a pending request for this resource, let's oblige
                                 if( common::communication::ipc::flush::optional::send( instance.process.ipc, request->message))
                                 {
                                    instance.state( state::resource::Proxy::Instance::State::busy);
                                    state.pending.requests.erase( std::begin( request));
                                 }
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
                                    instance::done( state, instance);
                                    instance.metrics.add( message);
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

                              // prepare and send the reply
                              auto reply = common::message::reverse::type( message);
                              reply.involved = common::algorithm::transform( branch.resources, []( auto& resource){ return resource.id;});
                              common::communication::device::blocking::optional::send( message.process.ipc, reply);

                              // partition what we don't got since before
                              auto involved = std::get< 1>( common::algorithm::intersection( message.involved, reply.involved));

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
                              bool reply( const M& message, C custom)
                              {
                                 auto reply = common::message::reverse::type( message);
                                 reply.trid = message.trid;
                                 reply.resource = message.resource;
                                 custom( reply);

                                 return common::predicate::boolean( common::communication::ipc::flush::optional::send( message.process.ipc, reply));
                              }
                              
                           } // send

                           
                           namespace prepare::commit
                           {
                              //! This phase are 'exactly' the same for external prepare and commit, except for the
                              //! reply message/type (of course), which we deduce from the request message.
                              template< typename M>
                              void phase( State& state, M&& message)
                              {
                                 Trace trace{ "transaction::manager::handle::local::resource::external::detail::prepare::commit::phase"};
                                 common::log::line( verbose::log, "message: ", message);

                                 // reply type, will be different for prepare and commit
                                 using Reply = decltype( common::message::reverse::type( message));

                                 auto transaction = common::algorithm::find( state.transactions, message.trid);

                                 if( ! transaction || transaction->stage > decltype( transaction->stage())::involved)
                                 {
                                    // Either the transaction is absent (???) or we're already in at least the prepare stage.
                                    // Eitherway we reply with read_only
                                    detail::send::reply( message, []( auto& reply)
                                    {
                                       reply.statistics.start = platform::time::clock::type::now();
                                       reply.statistics.end = reply.statistics.start;
                                       reply.state = decltype( reply.state)::read_only;
                                    });
                                    return;
                                 }

                                 // external TM is running the show, let's oblige
                                 transaction->owner = message.process;

                                 switch( transaction->resource_count())
                                 {
                                    case 0:
                                    {
                                       common::log::line( log, transaction->global, " no resources involved: ");

                                       // We can remove this transaction
                                       state.transactions.erase( std::begin( transaction));

                                       detail::send::reply( message, [ transaction]( auto& reply)
                                       { 
                                          reply.statistics.start = transaction->started;
                                          reply.statistics.end = platform::time::clock::type::now();
                                          reply.state = decltype( reply.state)::read_only;
                                          
                                       });

                                       break;
                                    }
                                    case 1:
                                    {
                                       // Only one resource involved, we do a one-phase-commit optimization.
                                       common::log::line( log, "global: ", transaction->global, " - only one resource involved");
                                       transaction->stage = decltype( transaction->stage())::commit;

                                       auto pending = local::detail::coordinate::pending::branches< common::message::transaction::resource::commit::Request>( 
                                          state, transaction->branches, state.coordinate.commit.empty_pendings(), common::flag::xa::Flag::one_phase);

                                       // different reply, depending on prepare or commit phase, below
                                       local::detail::coordinate::commit< Reply>( 
                                          state, std::move( pending), message.trid, local::detail::coordinate::destination( message));

                                       break;
                                    }
                                    default:
                                    {
                                       // More than one resource involved, we do the prepare stage
                                       common::log::line( log, "global: ", transaction->global, " more than one resource involved");
                                       transaction->stage = decltype( transaction->stage())::prepare;

                                       auto pending = local::detail::coordinate::pending::branches< common::message::transaction::resource::prepare::Request>( 
                                          state, transaction->branches, state.coordinate.prepare.empty_pendings());
                                       
                                       // different reply, depending on prepare or commit phase, below
                                       local::detail::coordinate::prepare< Reply>( 
                                          state, std::move( pending), message.trid, local::detail::coordinate::destination( message));

                                       break;
                                    }
                                 }
                              }
                              
                           } // prepare::commit


                        } // detail

                        namespace involved
                        {
                           auto request( State& state)
                           {
                              return [ &state]( common::message::transaction::resource::external::Involved& message)
                              {
                                 Trace trace{ "transaction::manager::handle::local::resource::external::involved::request"};
                                 common::log::line( log, "message: ", message);

                                 auto id = state::resource::external::proxy::id( state, message.process);

                                 auto& transaction = *local::detail::transaction::find_or_add_and_involve( state, message, id);
                                 common::log::line( verbose::log, "transaction: ", transaction);
                              };
                           }
                        } // involved

                        namespace prepare
                        {
                           auto request( State& state)
                           {
                              return [&state]( const common::message::transaction::resource::prepare::Request& message)
                              {
                                 // we use the generic prepare/commit phase
                                 detail::prepare::commit::phase( state, message);
                              };
                           }
                        } // prepare

                        namespace commit
                        {
                           auto request( State& state)
                           {
                              return [&state]( const common::message::transaction::resource::commit::Request& message)
                              {
                                 // we use the generic prepare/commit phase
                                 detail::prepare::commit::phase( state, message);
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

                                 if( ! transaction || transaction->stage > decltype( transaction->stage())::involved)
                                 {
                                    // Either the transaction is absent (???) or we're already in at least the prepare stage.
                                    // Eitherway we reply with read_only
                                    detail::send::reply( message, []( auto& reply)
                                    {
                                       reply.statistics.start = platform::time::clock::type::now();
                                       reply.statistics.end = reply.statistics.start;
                                       reply.state = decltype( reply.state)::read_only;
                                    });
                                    return;
                                 }

                                 // We dont 'optimize' the rollback phase. We could check if there are 0 resources involved and
                                 // possible gain a few us.

                                 transaction->stage = decltype( transaction->stage())::rollback;
                                 transaction->owner = message.process;

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

                              common::communication::device::blocking::optional::send( message.process.ipc, reply);
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
                           detail::instance::done( state, instance);
                        };
                     }

                  } // resource

                  namespace process
                  {
                     auto exit( State& state)
                     {
                        return [ &state]( common::message::event::process::Exit& message)
                        {
                           Trace trace{ "transaction::manager::handle::local::process::exit"};
                           common::log::line( verbose::log, "message: ", message);

                           // Check if it's a resource proxy instance
                           if( state.remove_instance( message.state.pid))
                           {
                              common::communication::device::blocking::send( 
                                 common::communication::instance::outbound::domain::manager::device(), message);
                              return;
                           }

                           auto trids = common::algorithm::accumulate( state.transactions, std::vector< common::transaction::ID>{}, [ &message]( auto result, auto& transaction)
                           {
                              if( ! transaction.branches.empty() && transaction.branches.back().trid.owner().pid == message.state.pid)
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

                  } // process

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

                              common::communication::device::blocking::optional::send( message.process.ipc, reply);
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
                           common::communication::device::blocking::optional::send( message.process.ipc, reply);
                        };
                     }

                  } // configuration
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
                     common::message::handle::defaults( ipc::device()),
                     local::process::exit( state),
                     local::resource::configuration::request( state),
                     local::resource::ready( state)
                   );
               }

            } // startup

            dispatch_type handlers( State& state)
            {
               return common::message::dispatch::handler( ipc::device(),
                  common::message::handle::defaults( ipc::device()),
                  common::event::listener( local::process::exit( state)),
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
                  local::resource::external::involved::request( state),
                  local::resource::external::prepare::request( state),
                  local::resource::external::commit::request( state),
                  local::resource::external::rollback::request( state),
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
         } // handle
      } // manager
   } // transaction
} // casual
