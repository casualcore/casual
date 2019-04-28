//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/transaction/context.h"
#include "common/service/call/context.h"

#include "common/communication/ipc.h"
#include "common/communication/instance.h"
#include "common/environment.h"
#include "common/process.h"
#include "common/log.h"
#include "common/algorithm.h"
#include "common/exception/xatmi.h"
#include "common/exception/tx.h"
#include "common/exception/xa.h"
#include "common/exception/system.h"
#include "common/code/convert.h"

#include "common/message/domain.h"
#include "common/event/send.h"

// std
#include <map>
#include <algorithm>

namespace casual
{
   namespace common
   {
      namespace transaction
      {
         struct Trace : common::log::Trace
         {
            template< typename T>
            Trace( T&& value) : common::log::Trace( std::forward< T>( value), log::category::transaction) {}
         };


         Context& Context::instance()
         {
            static Context singleton;
            return singleton;
         }


         Context::Context()
         = default;


         Transaction& Context::current()
         {
            if( m_transactions.empty())
            {
               static Transaction singleton;
               return singleton;
            }

            return m_transactions.back();
         }


         bool Context::associated( const Uuid& correlation)
         {
            for( auto& transaction : m_transactions)
            {
               if( transaction.state == Transaction::State::active && transaction.associated( correlation))
               {
                  return true;
               }
            }
            return false;
         }

         namespace local
         {
            namespace
            {
               namespace resource
               {

                  message::transaction::resource::lookup::Reply configuration( std::vector< std::string> names)
                  {
                     Trace trace{ "transaction::local::resource::configuration"};


                     message::transaction::resource::lookup::Request request;
                     request.process = process::handle();
                     request.resources = std::move( names);

                     auto reply = communication::ipc::call( communication::instance::outbound::transaction::manager::device(), request);

                     common::algorithm::for_each( reply.resources, []( auto& r){
                        r.openinfo = common::environment::string( r.openinfo);
                        r.closeinfo = common::environment::string( r.closeinfo);
                     });

                     return reply;
                  }

               } // resource

            } // <unnamed>
         } // local

         void Context::configure( std::vector< resource::Link> resources, std::vector< std::string> names)
         {
            Trace trace{ "transaction::Context::configure"};


            if( ! resources.empty())
            {
               // there are different semantics if the resource has a specific name
               // if so, we strictly correlate to that name, if not we go with the more general key

               auto splitted = common::algorithm::stable_partition( resources, []( auto& r){ return ! r.name.empty();});

               auto named = std::get< 0>( splitted);
               auto unnamed = std::get< 1>( splitted);

               common::algorithm::for_each( named, [&names]( auto& r){ common::algorithm::push_back_unique( r.name, names);});
               
               auto configuration = local::resource::configuration( std::move( names)).resources;

               // take care of named, if any
               {
                  auto transform_named = [&configuration]( auto& resource)
                  {
                     auto found = algorithm::find_if( configuration, [&resource]( auto& c){ return c.name == resource.name;});

                     if( ! found)
                     {
                        auto message = "missing configuration for linked named RM: " + resource.name + " - check domain configuration";
                        common::event::error::send( message);

                        throw exception::system::invalid::Argument( message);
                     }

                     return Resource{
                        resource,
                        found->id,
                        found->openinfo,
                        found->closeinfo,
                     };
                  };

                  algorithm::transform( named, m_resources.all, transform_named);
               }

               // take care of unnamed, if any
               for( auto& resource : unnamed)
               {
                  // It could be several RM-configuration for one linked RM.

                  auto partition = algorithm::filter( configuration, [&]( auto& rm){ return resource.key == rm.key;});

                  if( ! partition)
                  {
                     auto message = "missing configuration for linked RM: " + resource.key + " - check domain configuration";
                     common::event::error::send( message);

                     throw exception::system::invalid::Argument( message);
                  }

                  for( auto& rm : partition)
                  {
                     m_resources.all.emplace_back(
                        resource,
                        rm.id,
                        std::move( rm.openinfo),
                        std::move( rm.closeinfo));
                  }
               };
            }

            // create the views
            std::tie( m_resources.dynamic, m_resources.fixed) =
                  algorithm::partition( m_resources.all, []( auto& r){ return r.dynamic();});

            log::line( log::category::transaction, "static resources: ", m_resources.fixed);
            log::line( log::category::transaction, "dynamic resources: ", m_resources.dynamic);

            // Open the resources...
            // TODO: Not sure if we can do this, or if users has to call tx_open by them self...
            open();

         }


         bool Context::pending() const
         {
            const auto process = process::handle();

            return ! algorithm::find_if( m_transactions, [&]( const Transaction& transaction){
               return ! transaction.trid.null() && transaction.trid.owner() != process;
            }).empty();
         }

         namespace local
         {
            namespace
            {
               namespace pop
               {
                  struct Guard
                  {
                     Guard( std::vector< Transaction>& transactions) : m_transactions( transactions) {}
                     ~Guard() { m_transactions.erase( std::end( m_transactions));}

                     std::vector< Transaction>& m_transactions;
                  };
               } // pop


               namespace start
               {
                  Transaction transaction( const platform::time::point::type& start, TRANSACTION_TIMEOUT timeout)
                  {
                     Transaction transaction{ common::transaction::id::create( process::handle())};
                     transaction.state = Transaction::State::active;
                     transaction.timout.start = start;
                     transaction.timout.timeout = std::chrono::seconds{ timeout};

                     return transaction;
                  }
               } // start

               namespace resource
               {
                  auto involved( const transaction::ID& trid, std::vector< strong::resource::id> resources)
                  {
                     Trace trace{ "transaction::local::resource::involved"};

                     // we don't bother the TM if there's no resources involved...
                     if( resources.empty())
                        return resources;

                     message::transaction::resource::involved::Request message;
                     message.process = process::handle();
                     message.trid = trid;
                     message.involved = std::move( resources);

                     log::line( log::category::transaction, "involved message: ", message);

                     auto reply = communication::ipc::call( communication::instance::outbound::transaction::manager::device(), message);

                     return std::move( reply.involved);
                  }
               } // resource

            } // <unnamed>
         } // local

         std::vector< strong::resource::id> Context::resources() const
         {
            std::vector< strong::resource::id> result;

            for( auto& resource : m_resources.fixed)
            {
               result.push_back( resource.id());
            }
            return result;
         }


         void Context::join( const transaction::ID& trid)
         {
            Trace trace{ "transaction::Context::join"};

            Transaction transaction( trid);

            if( trid)
               resources_start( transaction, flag::xa::Flag::no_flags);

            m_transactions.push_back( std::move( transaction));
         }

         void Context::start( const platform::time::point::type& start)
         {
            Trace trace{ "transaction::Context::start"};

            auto transaction = local::start::transaction( start, m_timeout);

            resources_start( transaction, flag::xa::Flag::no_flags);

            m_transactions.push_back( std::move( transaction));
         }

         void Context::branch( const transaction::ID& trid)
         {
            Trace trace{ "transaction::Context::branch"};

            m_transactions.emplace_back( id::branch( trid));

            auto& transaction = m_transactions.back();

            if( transaction)
               resources_start( transaction, flag::xa::Flag::no_flags);
         }


         void Context::update( message::service::call::Reply& reply)
         {
            if( reply.transaction.trid)
            {
               auto found = algorithm::find( m_transactions, reply.transaction.trid);

               if( ! found)
               {
                  throw exception::xatmi::System{ string::compose( "failed to find transaction: ", reply.transaction)};
               }

               auto& transaction = *found;

               auto state = Transaction::State( reply.transaction.state);

               if( transaction.state < state)
               {
                  transaction.state = state;
               }

               // this descriptor is done, and we can remove the association to the transaction
               transaction.replied( reply.correlation);

               log::line( log::category::transaction, "updated state: ", transaction);
            }
            else
            {
               // TODO: if call was made in transaction but the service has 'none', the
               // trid is not replied, but the descriptor is still associated with the transaction.
               // Don't know what is the best way to solve this, but for now we just go through all
               // transaction and discard the descriptor, just in case.
               // TODO: Look this over when we redesign 'call/transaction-context'
               for( auto& transaction : m_transactions)
               {
                  transaction.replied( reply.correlation);
               }
            }
         }

         message::service::Transaction Context::finalize( bool commit)
         {
            Trace trace{ "transaction::Context::finalize"};

            // Regardless, we will consume every transaction.
            auto transactions = std::exchange( m_transactions, {});

            log::line( log::category::transaction, "transactions: ", transactions);

            const auto process = process::handle();


            message::service::Transaction result;
            result.trid = std::move( caller);
            result.state = message::service::Transaction::State::active;


            auto pending_check = [&]( Transaction& transaction)
            {
               if( transaction.pending())
               {
                  if( transaction.trid)
                  {
                     log::line( log::category::error, "pending replies associated with transaction - action: discard pending and set transaction state to rollback only");
                     log::line( log::category::transaction, transaction);

                     transaction.state = Transaction::State::rollback;
                     result.state = message::service::Transaction::State::error;
                  }

                  // Discard pending
                  algorithm::for_each( transaction.correlations(), []( const auto& correlation){ 
                     communication::ipc::inbound::device().discard( correlation);
                  });
               }
            };


            auto trans_rollback = [&]( const Transaction& transaction)
            {
               try
               {
                  Context::rollback( transaction);
               }
               catch( ...)
               {
                  exception::tx::handle();
                  log::line( log::category::transaction, "failed to rollback transaction: ", transaction.trid);
                  result.state = message::service::Transaction::State::error;
               }
            };

            auto trans_commit_rollback = [&]( const Transaction& transaction)
            {
               if( commit && transaction.state == Transaction::State::active)
               {
                  try
                  {
                    Context::commit( transaction); 
                  }
                  catch( ...)
                  {
                     exception::tx::handle();
                     log::line( log::category::transaction, "failed to commit transaction: ", transaction.trid);
                     result.state = message::service::Transaction::State::error;
                  } 
               }
               else
               {
                  trans_rollback( transaction);
               }
            };


            // Check pending calls
            algorithm::for_each( transactions, pending_check);

            // Ignore 'null trid':s
            auto actual_transactions = std::get< 0>( algorithm::stable_partition( transactions, [&]( const Transaction& transaction){
               return ! transaction.trid.null();
            }));


            auto owner_split = algorithm::stable_partition( actual_transactions, [&]( const Transaction& transaction){
               return transaction.trid.owner() != process;
            });

            // take care of owned transactions
            {
               auto owner = std::get< 1>( owner_split);
               algorithm::for_each( owner, trans_commit_rollback);
            }

            // Take care of not-owned transaction(s)
            {
               auto not_owner = std::get< 0>( owner_split);

               // should be 0..1
               assert( not_owner.size() <= 1);

               auto transform_state = []( Transaction::State state){
                  switch( state)
                  {
                     case Transaction::State::active: return message::service::Transaction::State::active;
                     case Transaction::State::rollback: return message::service::Transaction::State::rollback;
                     case Transaction::State::timeout: return message::service::Transaction::State::timeout;
                     default: return message::service::Transaction::State::error;
                  }
               };

               if( not_owner)
               {
                  log::line( log::category::transaction, "not_owner: ", *not_owner);

                  result.state = transform_state( not_owner->state);

                  if( ! commit && result.state == message::service::Transaction::State::active)
                  {
                     result.state = message::service::Transaction::State::rollback;
                  }


                  // Notify TM about resources involved in this transaction
                  /*
                  {
                     auto& transaction = found.front();
                     auto involved = resources();
                     algorithm::append( transaction.resources, involved);

                     if( transaction && ! involved.empty())
                     {
                        Context::involved( transaction.trid, std::move( involved));
                     }
                  }
                  */

                  // end resource
                  resources_end( *not_owner, flag::xa::Flag::success);
               }


               return result;
            }
         }

         code::ax Context::resource_registration( strong::resource::id rmid, XID* xid)
         {
            Trace trace{ "transaction::Context::resourceRegistration"};

            // Verify that rmid is known and is dynamic
            if( ! common::algorithm::find( m_resources.dynamic, rmid))
            {
               throw exception::ax::exception{ code::ax::argument, string::compose( "resource id: ", rmid)};
            }
            
            auto& transaction = current();

            // XA-spec - RM can't reg when it's already regged... Why?
            // We'll interpret this as the transaction has been suspended, and
            // then resumed.
            if( ! transaction.associate_dynamic( rmid))
            {
               throw exception::ax::exception{ code::ax::resume};
            }

            // Let the resource know the xid (if any)
            *xid = transaction.trid.xid;

            if( transaction)
            {
               // if the rm is already involved vi use join directly
               if( algorithm::find( transaction.involved(), rmid))
                  return code::ax::join;

               transaction.involve( rmid);

               // if local, we know that this rm has never been associated with this transaction.
               if( transaction.local())
                  return code::ax::ok;

               // we need to correlate with TM
               auto involved = local::resource::involved( transaction.trid, { rmid});
               return algorithm::find( involved, rmid).empty() ? code::ax::ok : code::ax::join;
            }

            return code::ax::ok;
         }

         void Context::resource_unregistration( strong::resource::id rmid)
         {
            Trace trace{ "transaction::Context::resource_unregistration"};

            auto&& transaction = current();

            // RM:s can only unregister if we're outside global
            // transactions, and the rm is registered before
            if( transaction.trid || ! transaction.disassociate_dynamic( rmid))
               throw exception::ax::exception{ code::ax::protocol};
         }


         void Context::begin()
         {
            Trace trace{ "transaction::Context::begin"};

            auto&& transaction = current();

            if( transaction.trid)
            {
               if( m_control != Control::stacked)
                  throw exception::tx::Protocol{ string::compose( "begin - already in transaction mode - ", transaction)};

               // Tell the RM:s to suspend
               resources_end( transaction, flag::xa::Flag::suspend);

            }
            else if( ! transaction.dynamic().empty())
               throw exception::tx::Outside{ "begin - dynamic resources not done with work outside global transaction"};

            auto trans = local::start::transaction( platform::time::clock::type::now(), m_timeout);

            resources_start( trans, flag::xa::Flag::no_flags);

            m_transactions.push_back( std::move( trans));

            log::line( log::category::transaction, "transaction: ", m_transactions.back().trid, " started");
         }


         void Context::open()
         {
            Trace trace{ "transaction::Context::open"};


            // XA spec: if one, or more of resources opens ok, then it's not an error...
            //   seams really strange not to notify user that some of the resources has
            //   failed to open...
            algorithm::for_each( m_resources.all, []( auto& r){
               auto result = r.open();
               if( result != code::xa::ok)
               {
                  common::event::error::send( string::compose( "failed to open resource: ", r.key(), " - error: ", std::error_code( result)));
               }
            });
         }

         void Context::close()
         {
            Trace trace{ "transaction::Context::close"};

            auto results = algorithm::transform( m_resources.all, []( auto& r){
               return r.close();
            });

            if( ! algorithm::all_of( results, []( auto r){ return r == code::xa::ok;}))
            {
               log::line( log::category::error, "failed to close one or more resource");
            }
         }


         void Context::commit( const Transaction& transaction)
         {
            Trace trace{ "transaction::Context::commit - transaction"};

            const auto process = process::handle();

            if( ! transaction.trid)
               throw exception::tx::no::Begin{ "commit - no ongoing transaction"};

            if( transaction.trid.owner() != process)
               throw exception::tx::Protocol{ string::compose( "commit - not owner of transaction: ", transaction)};

            if( transaction.state != Transaction::State::active)
               throw exception::tx::Protocol{ string::compose( "commit - transaction is in rollback only mode - ", transaction)};

            if( transaction.pending())
               throw exception::tx::Protocol{ string::compose(  "commit - pending replies associated with transaction: ", transaction)};

            // end resources
            resources_end( transaction, flag::xa::Flag::success);


            if( transaction.local() && transaction.involved().size() + m_resources.fixed.size() <= 1)
            {
               Trace trace{ "transaction::Context::commit - local"};

               // transaction is local, and at most one resource is involved.
               // We do the commit directly against the resource (if any).
               // TODO: we could do a two-phase-commit local if the transaction is 'local'

               if( ! transaction.involved().empty())
                  return resource_commit( transaction.involved().front(), transaction, flag::xa::Flag::one_phase);

               if( ! m_resources.fixed.empty())
                  return resource_commit( m_resources.fixed.front().id(), transaction, flag::xa::Flag::one_phase);

               // No resources associated to this transaction, hence the commit is successful.
               return;
            }
            else
            {
               Trace trace{ "transaction::Context::commit - distributed"};

               message::transaction::commit::Request request;
               request.trid = transaction.trid;
               request.process = process;
               request.involved = transaction.involved();

               // Get reply
               {
                  auto reply = communication::ipc::call( communication::instance::outbound::transaction::manager::device(), request);

                  // We could get commit-reply directly in an one-phase-commit

                  switch( reply.stage)
                  {
                     case message::transaction::commit::Reply::Stage::prepare:
                     {
                        log::line( log::category::transaction, "prepare reply: ", reply.state);

                        if( m_commit_return == Commit_Return::logged)
                        {
                           log::line( log::category::transaction, "decision logged directive");

                           // Discard the coming commit-message
                           communication::ipc::inbound::device().discard( reply.correlation);
                        }
                        else
                        {
                           // Wait for the commit
                           communication::ipc::blocking::receive( communication::ipc::inbound::device(), reply, reply.correlation);

                           log::line( log::category::transaction, "commit reply: ", reply.state);
                        }

                        break;
                     }
                     case message::transaction::commit::Reply::Stage::commit:
                     {
                        log::line( log::category::transaction, "commit reply: ", reply.state);
                        break;
                     }
                     case message::transaction::commit::Reply::Stage::error:
                     {
                        log::line( log::category::error, "commit error: ", reply.state);
                        break;
                     }
                  }

                  exception::tx::handle( reply.state, "commit");
               }
            }
         }

         void Context::commit()
         {
            Trace trace{ "transaction::Context::commit"};

            commit( current());

            // We only remove/consume transaction if commit succeed
            // TODO: any other situation we should remove?
            pop_transaction();
         }

         void Context::rollback( const Transaction& transaction)
         {
            Trace trace{ "transaction::Context::rollback"};

            log::line( log::category::transaction, "transaction: ", transaction);

            if( ! transaction)
               throw exception::tx::Protocol{ "no ongoing transaction"};

            const auto process = process::handle();

            if( transaction.trid.owner() != process)
               throw exception::tx::Protocol{ string::compose( "current process not owner of transaction: ", transaction.trid)};

            // end resources
            resources_end( transaction, flag::xa::Flag::success);

            if( transaction.local())
            {
               log::line( log::category::transaction, "rollback is local");

               auto involved = transaction.involved();
               algorithm::transform( m_resources.fixed, involved, []( auto& r){ return r.id();});

               algorithm::for_each( algorithm::unique( algorithm::sort( involved)), [&]( auto id)
               {
                  resource_rollback( id, transaction);
               });
            }
            else 
            {
               log::line( log::category::transaction, "rollback is distributed");

               message::transaction::rollback::Request request;
               request.trid = transaction.trid;
               request.process = process;
               algorithm::append( transaction.involved(), request.involved);

               auto reply = communication::ipc::call( communication::instance::outbound::transaction::manager::device(), request);

               log::line( log::category::transaction, "rollback reply tx: ", reply.state);

               exception::tx::handle( reply.state, "rollback");
            }
         }

         void Context::rollback()
         {
            rollback( current());
            pop_transaction();
         }



         void Context::set_commit_return( COMMIT_RETURN value)
         {
            switch( value)
            {
               case TX_COMMIT_COMPLETED:
               case TX_COMMIT_DECISION_LOGGED:
               {
                  m_commit_return = static_cast< Commit_Return>( value);
                  break;
               }
               default:
               {
                  throw exception::tx::Argument{};
               }
            }
         }

         COMMIT_RETURN Context::get_commit_return()
         {
            return static_cast< COMMIT_RETURN>( m_commit_return);
         }

         void Context::set_transaction_control( TRANSACTION_CONTROL control)
         {
            switch( control)
            {
               case TX_UNCHAINED:
               {
                  m_control = Control::unchained;
                  break;
               }
               case TX_CHAINED:
               {
                  m_control = Control::chained;
                  break;
               }
               case TX_STACKED:
               {
                  m_control = Control::stacked;
                  break;
               }
               default:
               {
                  throw exception::tx::Argument{ string::compose( "argument control has invalid value: ", control)};
               }
            }
         }

         void Context::set_transaction_timeout( TRANSACTION_TIMEOUT timeout)
         {
            if( timeout < 0)
            {
               throw exception::tx::Argument{ "timeout value has to be 0 or greater"};
            }
            m_timeout = timeout;
         }

         bool Context::info( TXINFO* info)
         {
            auto&& transaction = current();

            if( info)
            {
               info->xid = transaction.trid.xid;
               info->transaction_state = static_cast< decltype( info->transaction_state)>( transaction.state);
               info->transaction_timeout = m_timeout;
               info->transaction_control = static_cast< control_type>( m_control);
            }
            return static_cast< bool>( transaction);
         }


         void Context::suspend( XID* xid)
         {
            Trace trace{ "transaction::Context::suspend"};

            if( xid == nullptr)
            {
               throw exception::tx::Argument{ "argument xid is null"};
            }

            auto& ongoing = current();

            if( id::null( ongoing.trid))
            {
               throw exception::tx::Protocol{ "attempt to suspend a null xid"};
            }

            // We don't check if current transaction is aborted. This differs from Tuxedo's semantics

            // Tell the RM:s to suspend
            resources_end( ongoing, flag::xa::Flag::suspend);

            *xid = ongoing.trid.xid;

            // Push a null-xid to indicate suspended transaction
            m_transactions.emplace_back();
         }



         void Context::resume( const XID* xid)
         {
            Trace trace{ "transaction::Context::resume"};

            if( xid == nullptr)
               throw exception::tx::Argument{ "argument xid is null"};

            if( id::null( *xid))
               throw exception::tx::Argument{ "attempt to resume a 'null xid'"};

            auto& ongoing = current();

            if( ongoing.trid)
               throw exception::tx::Protocol{ "active transaction"};

            if( ! ongoing.dynamic().empty())
               throw exception::tx::Outside{ string::compose( "ongoing work outside global transaction: ", ongoing)};

            auto found = algorithm::find( m_transactions, *xid);

            if( ! found)
               throw exception::tx::Argument{ "transaction not known"};


            // All precondition are met, let's set wanted transaction as current
            {
               // Tell the RM:s to resume
               resources_start( *found, flag::xa::Flag::resume);

               // We pop the top null-xid that represent a suspended transaction
               m_transactions.pop_back();

               // We rotate the wanted to end;
               algorithm::rotate( m_transactions, ++found);
            }
         }

         void Context::resources_start( Transaction& transaction, flag::xa::Flags flags)
         {
            Trace trace{ "transaction::Context::resources_start"};

            if( ! transaction)
               return; // nothing to do


            auto deduce_flag = []( auto id, auto flags, auto& involved)
            {
               if( ! flags.exist( flag::xa::Flag::resume))
                  return algorithm::find( involved, id).empty() ? flags : flags | flag::xa::Flag::join;

               return flags;
            };

            auto transform_rmid = []( auto& resource){ return resource.id();};

            if( transaction.local())
            {
               log::line( log::category::transaction, "local transaction: ", transaction, " - flags: ", flags);
               
               // local transaction, we don't need to coralate with TM
               auto resource_start = [&]( auto& resource)
               {
                  auto result = resource.start( transaction.trid, deduce_flag( resource.id(), flags, transaction.involved())); 
                  if( result != code::xa::ok)
                     log::line( code::stream( result), "failed to start resource: ", resource, " - error: ", result);
               };

               algorithm::for_each( m_resources.fixed, resource_start);
            }
            else 
            {
               log::line( log::category::transaction, "distributed transaction: ", transaction, " - flags: ", flags);

               // coordinate with TM
               auto involved = local::resource::involved( transaction.trid, 
                  algorithm::transform( m_resources.fixed, transform_rmid));
               
               auto resource_start = [&]( auto& resource)
               {
                  auto result = resource.start( transaction.trid, deduce_flag( resource.id(), flags, involved));
                  if( result != code::xa::ok)  
                     log::line( code::stream( result), "failed to start resource: ", resource, " - error: ", result);
               };

               algorithm::for_each( m_resources.fixed, resource_start);
            }

            // involve all static resources
            transaction.involve( algorithm::transform( m_resources.fixed, transform_rmid));


            // TODO: throw if some of the rm:s report an error?
            
         }

         void Context::resources_end( const Transaction& transaction, flag::xa::Flags flags)
         {
            Trace trace{ "transaction::Context::resources_end"};

            if( transaction)
            {
               log::line( log::category::transaction, "transaction: ", transaction, " - flags: ", flags);

               // We call end on all resources
               for( auto& r : m_resources.all)
               {
                  auto result = r.end( transaction.trid, flags);
                  if( result != code::xa::ok)
                  {
                     log::line( code::stream( result), "failed to end resource: ", r, " - error: ", result);
                  }
               }
               // TODO: throw if some of the rm:s report an error?
            }
         }

         void Context::resource_commit( strong::resource::id rm, const Transaction& transaction, flag::xa::Flags flags)
         {
            Trace trace{ "transaction::Context::resources_commit"};

            log::line( log::category::transaction, "transaction: ", transaction, " - rm: ", rm, " - flags: ", flags);

            auto found = algorithm::find( m_resources.all, rm);

            if( found)
            {
               auto code = common::code::convert::to::tx( found->commit( transaction.trid, flags));
               exception::tx::handle( code, "resource commit");
            }
            else
               throw exception::tx::Error{ string::compose( "resource id not known - rm: ", rm, " transaction: ", transaction)};
         }

         void Context::resource_rollback( strong::resource::id rm, const Transaction& transaction)
         {
            Trace trace{ "transaction::Context::resource_rollback"};
            
            log::line( log::category::transaction, "transaction: ", transaction, " - rm: ", rm);

            auto found = algorithm::find( m_resources.all, rm);

            if( found)
            {
               auto code = common::code::convert::to::tx( found->rollback( transaction.trid, flag::xa::Flag::no_flags));
               exception::tx::handle( code, "resource rollback");
            }
            else
               throw exception::tx::Error{ string::compose( "resource id not known - rm: ", rm, " transaction: ", transaction)};
         }

         void Context::pop_transaction()
         {
            Trace trace{ "transaction::Context::pop_transaction"};

            // Dependent on control we do different stuff
            switch( m_control)
            {
               case Control::unchained:
               {
                  // Same as pop, then push null-xid
                  m_transactions.back() = Transaction{};
                  break;
               }
               case Control::chained:
               {
                  // Same as pop, then push null-xid
                  m_transactions.back() = Transaction{};

                  // We start a new one
                  begin();
                  break;
               }
               case Control::stacked:
               {
                  // Just promote the previous transaction in the stack to current
                  m_transactions.pop_back();

                  // Tell the RM:s to resume
                  resources_end( m_transactions.back(), flag::xa::Flag::resume);

                  break;
               }
               default:
               {
                  throw exception::tx::Fail{ "unknown control directive - this can not happen"};
               }
            }
         }

      } // transaction
   } // common
} //casual


