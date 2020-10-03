//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/transaction/context.h"

#include "common/communication/ipc.h"
#include "common/communication/instance.h"
#include "common/environment.h"
#include "common/environment/normalize.h"
#include "common/process.h"
#include "common/log.h"
#include "common/algorithm.h"
#include "common/code/raise.h"
#include "common/code/tx.h"
#include "common/code/xa.h"
#include "common/code/casual.h"
#include "common/code/convert.h"
#include "common/exception/handle.h"

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

         Context::Context() = default;

         Context::~Context()
         {
            if( ! m_transactions.empty())
               log::line( log::stream::get( "error"), code::casual::invalid_semantics, " transactions not consumed: ", m_transactions.size());
         }

         Context& Context::clear()
         {
            instance() = Context{};
            return instance();
         }

         bool Context::empty() const
         {
            return m_transactions.empty();
         }

         Transaction& Context::current()
         {
            if( m_transactions.empty() || m_transactions.back().suspended())
               return m_empty;

            return m_transactions.back();
         }

         bool Context::associated( const Uuid& correlation)
         {
            return algorithm::any_of( m_transactions, [&correlation]( auto& transaction)
            {
               return transaction.state == Transaction::State::active && transaction.associated( correlation);
            });
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

                     common::environment::normalize( reply);

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
                        common::event::error::raise( code::casual::invalid_configuration, "missing configuration for linked named RM: ", resource.name, " - check domain configuration");

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
                     common::event::error::raise( code::casual::invalid_configuration, "missing configuration for linked RM: ", resource.key, " - check domain configuration");

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
            // TODO semantics: Not sure if we can do this, or if users has to call tx_open by them self...
            open();

         }


         bool Context::pending() const
         {
            const auto process = process::handle();

            return ! algorithm::find_if( m_transactions, [&]( const Transaction& transaction){
               return ! transaction.trid.null() && transaction.trid.owner() == process;
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
                     transaction.timeout.start = start;
                     transaction.timeout.timeout = std::chrono::seconds{ timeout};

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

               auto raise_if_not_ok = []( auto code, auto&& context)
               {
                  if( code != decltype( code)::ok)
                     code::raise::error( code, context);
               };

            } // <unnamed>
         } // local

         std::vector< strong::resource::id> Context::resources() const
         {
            return algorithm::transform( m_resources.all, []( auto& resource){ return resource.id();});
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
            Trace trace{ "transaction::Context::update"};

            if( reply.transaction.trid)
            {
               auto found = algorithm::find( m_transactions, reply.transaction.trid);

               if( ! found)
                  code::raise::error( code::xatmi::system, "failed to find transaction: ", reply.transaction);

               auto& transaction = *found;

               auto state = Transaction::State( reply.transaction.state);

               if( transaction.state < state)
                  transaction.state = state;

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
                  transaction.replied( reply.correlation);
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
                  result.state = message::service::Transaction::State::error;
                  exception::handle( log::category::error, "failed to rollback transaction: ", transaction.trid);
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
                     result.state = message::service::Transaction::State::error;
                     exception::handle( log::category::error, "failed to commit transaction: ", transaction.trid);
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
            Trace trace{ "transaction::Context::resource_registration"};

            // Verify that rmid is known and is dynamic
            if( ! common::algorithm::find( m_resources.dynamic, rmid))
               code::raise::error( code::ax::argument, "resource id: ", rmid);
            
            auto& transaction = current();

            // XA-spec - RM can't reg when it's already regged... Why?
            // We'll interpret this as the transaction has been suspended, and
            // then resumed.
            if( ! transaction.associate_dynamic( rmid))
               code::raise::error( code::ax::resume, "resource id: ", rmid);

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
               code::raise::error( code::ax::protocol, "resource id: ", rmid);
         }


         void Context::begin()
         {
            Trace trace{ "transaction::Context::begin"};

            auto&& transaction = current();

            if( transaction.trid)
            {
               if( m_control != Control::stacked)
                  code::raise::log( code::tx::protocol, "begin - already in transaction mode - ", transaction);

               // Tell the RM:s to suspend
               resources_end( transaction, flag::xa::Flag::suspend);

            }
            else if( ! transaction.dynamic().empty())
               code::raise::error( code::tx::outside,  "begin - dynamic resources not done with work outside global transaction");

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

            auto results = algorithm::transform( m_resources.all, []( auto& resource)
            {
               return resource.open();
            });

            if( ! algorithm::all_of( results, []( auto r){ return r == code::xa::ok;}))
               log::line( log::category::error, code::xa::resource_error, "failed to open one or more resource");
         }

         void Context::close()
         {
            Trace trace{ "transaction::Context::close"};

            auto results = algorithm::transform( m_resources.all, []( auto& resource)
            {
               return resource.close();
            });

            if( ! algorithm::all_of( results, []( auto r){ return r == code::xa::ok;}))
               log::line( log::category::error, code::xa::resource_error, "failed to close one or more resource");
         }


         void Context::commit( const Transaction& transaction)
         {
            Trace trace{ "transaction::Context::commit - transaction"};

            const auto process = process::handle();

            if( ! transaction.trid)
               code::raise::log( code::tx::no_begin, "commit - no ongoing transaction");

            if( transaction.trid.owner() != process)
               code::raise::error( code::tx::protocol, "commit - not owner of transaction: ", transaction.trid);

            if( transaction.state != Transaction::State::active)
               code::raise::error( code::tx::protocol, "commit - transaction is in rollback only mode - ", transaction.trid);

            if( transaction.pending())
               code::raise::error( code::tx::protocol, "commit - pending replies associated with transaction: ", transaction.trid);

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
                     using State = decltype( reply.stage);

                     case State::prepare:
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
                           communication::device::blocking::receive( communication::ipc::inbound::device(), reply, reply.correlation);

                           log::line( log::category::transaction, "commit reply: ", reply.state);
                        }

                        break;
                     }
                     case State::commit:
                     {
                        log::line( log::category::transaction, "commit reply: ", reply.state);
                        break;
                     }
                     case State::error:
                     {
                        log::line( log::category::error, "commit error: ", reply.state);
                        break;
                     }
                  }

                  local::raise_if_not_ok( reply.state, "during commit");
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
               code::raise::log( code::tx::protocol, "no ongoing transaction");

            const auto process = process::handle();

            if( transaction.trid.owner() != process)
               code::raise::error( code::tx::protocol, "current process not owner of transaction: ", transaction.trid);

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

               local::raise_if_not_ok( reply.state, "during rollback");
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
                  code::raise::error( code::tx::argument, "set_commit_return");
               }
            }
         }

         COMMIT_RETURN Context::get_commit_return() noexcept
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
                  code::raise::error( code::tx::argument, "set_transaction_control");
               }
            }
         }

         void Context::set_transaction_timeout( TRANSACTION_TIMEOUT timeout)
         {
            if( timeout < 0)
               code::raise::error( code::tx::argument, "timeout value has to be 0 or greater");

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
               code::raise::error( code::tx::argument, "suspend: argument xid is null");

            auto& ongoing = current();

            if( id::null( ongoing.trid))
               code::raise::error( code::tx::protocol, "suspend: attempt to suspend a null xid");

            // We don't check if current transaction is aborted. This differs from Tuxedo's semantics

            // mark the transaction as suspended
            ongoing.suspend();

            // Tell the RM:s to suspend
            resources_end( ongoing, flag::xa::Flag::suspend);

            *xid = ongoing.trid.xid;
         }



         void Context::resume( const XID* xid)
         {
            Trace trace{ "transaction::Context::resume"};

            if( xid == nullptr)
               code::raise::log( code::tx::argument, "resume: argument xid is null");

            if( id::null( *xid))
               code::raise::log( code::tx::argument, "resume: attempt to resume a 'null xid'");

            auto& ongoing = current();

            if( ongoing.trid && ! ongoing.suspended())
               code::raise::error( code::tx::protocol, "resume: ongoing transaction is active");

            if( ! ongoing.dynamic().empty())
               code::raise::error( code::tx::outside, "resume: ongoing work outside global transaction: ", ongoing);

            auto found = algorithm::find( m_transactions, *xid);

            if( ! found)
               code::raise::error( code::tx::argument, "resume: transaction not known");

            if( ! found->suspended())
               code::raise::error( code::tx::protocol, "resume: wanted transaction is not suspended");


            // All precondition are met, let's set wanted transaction as current
            {
               found->resume();

               // Tell the RM:s to resume
               resources_start( *found, flag::xa::Flag::resume);

               // We rotate the wanted to end;
               algorithm::rotate( m_transactions, ++found);
            }
         }

         void Context::resources_start( Transaction& transaction, flag::xa::Flags flags)
         {
            Trace trace{ "transaction::Context::resources_start"};

            if( ! transaction)
               return; // nothing to do

            auto transform_rmid = []( auto& resource){ return resource.id();};

            auto start_functor = [&]( auto& involved)
            {
               return [&trid = transaction.trid, flags = flags, &involved]( auto& resource)
               {
                  auto deduce_flag = []( auto id, auto flags, auto& involved)
                  {
                     if( ! flags.exist( flag::xa::Flag::resume))
                        return algorithm::find( involved, id).empty() ? flags : flags | flag::xa::Flag::join;

                     return flags;
                  };

                  resource.start( trid, deduce_flag( resource.id(), flags, involved)); 
               };
            };

            if( transaction.local())
            {
               log::line( log::category::transaction, "local transaction: ", transaction, " - flags: ", flags);
               
               // local transaction, we don't need to coralate with TM
               algorithm::for_each( m_resources.fixed, start_functor( transaction.involved()));
            }
            else 
            {
               log::line( log::category::transaction, "distributed transaction: ", transaction, " - flags: ", flags);

               // coordinate with TM, so we know if we need to add `join` flag or not.
               auto involved = local::resource::involved( transaction.trid, 
                  algorithm::transform( m_resources.fixed, transform_rmid));

               algorithm::for_each( m_resources.fixed, start_functor( involved));
            }

            // involve all static resources
            transaction.involve( algorithm::transform( m_resources.fixed, transform_rmid));

            // TODO semantics: throw if some of the rm:s report an error?
            //   don't think so. prepare, commit or rollback will take care of eventual errors.
         }

         void Context::resources_end( const Transaction& transaction, flag::xa::Flags flags)
         {
            Trace trace{ "transaction::Context::resources_end"};

            if( ! transaction)
               return;
            
            log::line( log::category::transaction, "transaction: ", transaction, " - flags: ", flags);

            auto xa_end = [&transaction, flags]( auto& resource)
            {
               resource.end( transaction.trid, flags);
            };

            // We call end on all static resources
            algorithm::for_each( m_resources.fixed, xa_end);

            auto has_registred = [&]( auto& resource)
            {
               return ! algorithm::find( transaction.dynamic(), resource.id()).empty();
            };

            // We call end only on the dynamic resources that has registred them self 
            // to the transaction
            // note: we could partition dynamic first on `has_registred`, but if we can 
            // keep the order of the resources it won't hurt (be as nice as possible to the resources).
            algorithm::for_each_if( m_resources.dynamic, xa_end, has_registred);

            // TODO semantics: throw if some of the rm:s report an error?
            //   don't think so. prepare, commit or rollback will take care of eventual errors.
            
         }

         void Context::resource_commit( strong::resource::id rm, const Transaction& transaction, flag::xa::Flags flags)
         {
            Trace trace{ "transaction::Context::resources_commit"};
            log::line( log::category::transaction, "transaction: ", transaction, " - rm: ", rm, " - flags: ", flags);

            if( auto found = algorithm::find( m_resources.all, rm))
            {
               auto code = common::code::convert::to::tx( found->commit( transaction.trid, flags));
               local::raise_if_not_ok( code, "resource commit");
            }
            else
               code::raise::error( code::tx::error, "resource id not known - rm: ", rm, " transaction: ", transaction);
         }

         void Context::resource_rollback( strong::resource::id rm, const Transaction& transaction)
         {
            Trace trace{ "transaction::Context::resource_rollback"};
            log::line( log::category::transaction, "transaction: ", transaction, " - rm: ", rm);

            if( auto found = algorithm::find( m_resources.all, rm))
            {
               auto code = common::code::convert::to::tx( found->rollback( transaction.trid, flag::xa::Flag::no_flags));
               local::raise_if_not_ok( code, "resource rollback");
            }
            else
               code::raise::error( code::tx::error, "resource id not known - rm: ", rm, " transaction: ", transaction);
         }

         void Context::pop_transaction()
         {
            Trace trace{ "transaction::Context::pop_transaction"};

            // pop the current transaction
            m_transactions.pop_back();

            // Dependent on control we do different stuff
            switch( m_control)
            {
               case Control::unchained:
               {
                  // no op
                  break;
               }
               case Control::chained:
               {
                  // We start a new one
                  begin();
                  break;
               }
               case Control::stacked:
               {
                  if( auto& current = Context::current())
                  {
                     // Tell the RM:s to resume
                     resources_end( current, flag::xa::Flag::resume);
                  }

                  break;
               }
               default:
               {
                  code::raise::error( code::tx::fail, "unknown control directive - this can not happen");
               }
            }
         }

      } // transaction
   } // common
} //casual


