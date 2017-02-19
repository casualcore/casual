//!
//! casual
//!

#include "common/transaction/context.h"
#include "common/service/call/context.h"

#include "common/communication/ipc.h"
#include "common/environment.h"
#include "common/process.h"
#include "common/internal/log.h"
#include "common/internal/trace.h"
#include "common/algorithm.h"
#include "common/error.h"
#include "common/exception.h"

#include "common/message/domain.h"



#include <map>
#include <algorithm>

namespace casual
{
   namespace common
   {

      namespace transaction
      {

         int xaTotx( int code)
         {
            switch( code)
            {
               case XA_RBROLLBACK:
               case XA_RBCOMMFAIL:
               case XA_RBDEADLOCK:
               case XA_RBINTEGRITY:
               case XA_RBOTHER:
               case XA_RBPROTO:
               case XA_RBTIMEOUT:
               case XA_RBTRANSIENT:
               case XA_NOMIGRATE:
               case XA_HEURHAZ: return TX_HAZARD;
               case XA_HEURCOM:
               case XA_HEURRB:
               case XA_HEURMIX: return TX_MIXED;
               case XA_RETRY:
               case XA_RDONLY: return TX_OK;
               case XA_OK: return TX_OK;
               case XAER_ASYNC:
               case XAER_RMERR:
               case XAER_NOTA: return TX_NO_BEGIN;
               case XAER_INVAL:
               case XAER_PROTO: return TX_PROTOCOL_ERROR;
               case XAER_RMFAIL:
               case XAER_DUPID:
               case XAER_OUTSIDE: return TX_OUTSIDE;
               default: return TX_FAIL;

            }
         }


         Context& Context::instance()
         {
            static Context singleton;
            return singleton;
         }


         Context::Context()
         {
         }


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
                     common::trace::Scope trace{ "transaction::local::resource::configuration", common::log::internal::transaction};


                     message::transaction::resource::lookup::Request request;
                     request.process = process::handle();
                     request.resources = std::move( names);

                     return communication::ipc::call( communication::ipc::transaction::manager::device(), request);

                  }

               } // resource

            } // <unnamed>
         } // local

         void Context::configure( const std::vector< Resource>& resources, std::vector< std::string> names)
         {
            common::trace::Scope trace{ "transaction::Context::configure", common::log::internal::transaction};

            if( ! resources.empty())
            {

               auto reply = local::resource::configuration( std::move( names));

               auto configuration = range::make( reply.resources);


               for( auto& resource : resources)
               {


                  using RM = decltype( range::front( configuration));

                  //
                  // It could be several RM-configuration for one linked RM.
                  //

                  auto splitted = range::stable_partition( configuration, [&]( RM rm){ return resource.key == rm.key;});

                  auto partition = std::get< 0>( splitted);

                  if( ! partition)
                  {
                     throw exception::invalid::Argument( "missing configuration for linked RM: " + resource.key + " - check group memberships");
                  }

                  for( auto& rm : partition)
                  {
                     m_resources.all.emplace_back(
                           resource.key,
                           resource.xa_switch,
                           rm.id,
                           std::move( rm.openinfo),
                           std::move( rm.closeinfo));
                  }



                  //
                  // Continue with the rest
                  //
                  configuration = std::get< 1>( splitted);
               }
            }

            //
            // create the views
            //
            std::tie( m_resources.dynamic, m_resources.fixed) =
                  range::partition( m_resources.all, std::mem_fn( &Resource::dynamic));

            common::log::internal::transaction << "static resources: " << m_resources.fixed << std::endl;
            common::log::internal::transaction << "dynamic resources: " << m_resources.dynamic << std::endl;


            //
            // Open the resources...
            // TODO: Not sure if we can do this, or if users has to call tx_open by them self...
            //
            open();

         }


         bool Context::pending() const
         {
            const auto process = process::handle();

            return ! range::find_if( m_transactions, [&]( const Transaction& transaction){
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



               Transaction startTransaction( const platform::time::point::type& start, TRANSACTION_TIMEOUT timeout)
               {

                  Transaction transaction{ transaction::ID::create( process::handle())};
                  transaction.state = Transaction::State::active;
                  transaction.timout.start = start;
                  transaction.timout.timeout = std::chrono::seconds{ timeout};

                  return transaction;
               }

            } // <unnamed>
         } // local

         std::vector< int> Context::resources() const
         {
            std::vector< int> result;

            for( auto& resource : m_resources.fixed)
            {
               result.push_back( resource.id);
            }
            return result;
         }

         void Context::involved( const transaction::ID& trid, std::vector< int> resources)
         {
            common::trace::Scope trace{ "transaction::Context::involved", common::log::internal::transaction};

            if( ! resources.empty())
            {
               message::transaction::resource::Involved message;
               message.process = process::handle();
               message.trid = trid;
               message.resources = std::move( resources);

               common::log::internal::transaction << "involved message: " << message << '\n';

               communication::ipc::blocking::send( communication::ipc::transaction::manager::device(), message);
            }
         }


         void Context::join( const transaction::ID& trid)
         {
            common::trace::Scope trace{ "transaction::Context::join", common::log::internal::transaction};

            Transaction transaction( trid);

            if( trid)
            {
               resources_start( transaction, TMNOFLAGS);
            }

            m_transactions.push_back( std::move( transaction));
         }

         void Context::start( const platform::time::point::type& start)
         {
            common::trace::Scope trace{ "transaction::Context::start", common::log::internal::transaction};

            auto transaction = local::startTransaction( start, m_timeout);

            resources_start( transaction, TMNOFLAGS);

            m_transactions.push_back( std::move( transaction));
         }


         void Context::update( message::service::call::Reply& reply)
         {
            if( reply.transaction.trid)
            {
               auto found = range::find( m_transactions, reply.transaction.trid);

               if( ! found)
               {
                  throw exception::xatmi::System{ "failed to find transaction", __FILE__, __LINE__};
               }

               auto& transaction = *found;

               auto state = Transaction::State( reply.transaction.state);

               if( transaction.state < state)
               {
                  transaction.state = state;
               }

               //
               // this descriptor is done, and we can remove the association to the transaction
               //
               transaction.replied( reply.correlation);

               log::internal::transaction << "updated state: " << transaction << std::endl;
            }
            else
            {
               //
               // TODO: if call was made in transaction but the service has 'none', the
               // trid is not replied, but the descriptor is still associated with the transaction.
               // Don't know what is the best way to solve this, but for now we just go through all
               // transaction and discard the descriptor, just in case.
               // TODO: Look this over when we redesign 'call/transaction-context'
               //
               for( auto& transaction : m_transactions)
               {
                  transaction.replied( reply.correlation);
               }
            }
         }

         void Context::finalize( message::service::call::Reply& message, int return_state)
         {
            common::trace::Scope trace{ "transaction::Context::finalize", common::log::internal::transaction};

            //
            // Regardless, we will consume every transaction.
            //
            decltype( m_transactions) transactions;
            std::swap( transactions, m_transactions);

            const auto process = process::handle();



            auto pending_check = [&]( Transaction& transaction)
            {
               if( transaction.pending())
               {
                  if( transaction.trid)
                  {
                     log::error << "pending replies associated with transaction - action: transaction state to rollback only\n";
                     log::internal::transaction << transaction << std::endl;

                     transaction.state = Transaction::State::rollback;
                     message.error = TPESVCERR;
                  }

                  //
                  // Discard pending
                  //
                  /*
                  for( const auto& correlation : transaction.correlations())
                  {
                     service::call::Context::instance().cancel( descriptor);
                  }
                  */
               }
            };


            auto trans_rollback = [&]( const Transaction& transaction)
            {
               auto result = rollback( transaction);

               if( result != TX_OK)
               {
                  log::error << "failed to rollback transaction: " << transaction.trid << " - " << error::tx::error( result) << std::endl;
                  message.error = TPESVCERR;
               }
            };

            auto trans_commit_rollback = [&]( const Transaction& transaction)
            {
               if( return_state == TPSUCCESS && transaction.state == Transaction::State::active)
               {
                  auto result = commit( transaction);

                  if( result != TX_OK)
                  {
                     log::error << "failed to commit transaction: " << transaction.trid << " - " << error::tx::error( result) << std::endl;
                     message.error = TPESVCERR;
                  }
               }
               else
               {
                  trans_rollback( transaction);
               }
            };

            switch( return_state)
            {
               case TPESVCERR: break;
               case TPSUCCESS: break;
               default: message.error = TPESVCFAIL; break;
            }


            //
            // Check pending calls
            //
            range::for_each( transactions, pending_check);

            //
            // Ignore 'null trid':s
            //
            auto actual_transactions = std::get< 0>( range::stable_partition( transactions, [&]( const Transaction& transaction){
               return ! transaction.trid.null();
            }));


            auto owner_split = range::stable_partition( actual_transactions, [&]( const Transaction& transaction){
               return transaction.trid.owner() != process;
            });




            //
            // take care of owned transactions
            //
            {
               auto owner = std::get< 1>( owner_split);
               range::for_each( owner, trans_commit_rollback);
            }


            //
            // Take care of not-owned transaction(s)
            //
            {
               auto not_owner = std::get< 0>( owner_split);

               //
               // should be 0..1
               //
               assert( not_owner.size() <= 1);

               auto found = range::find_if( not_owner, [&]( const Transaction& transaction){
                  return transaction.trid == caller;
               });

               if( found)
               {
                  if( return_state == TPSUCCESS)
                  {
                     message.transaction.state = static_cast< decltype( message.transaction.state)>( found->state);
                  }
                  else
                  {
                     message.transaction.state = static_cast< decltype( message.transaction.state)>( Transaction::State::rollback);
                  }
                  

                  //
                  // Notify TM about resources involved in this transaction
                  //
                  {
                     auto& transaction = found.front();
                     auto involved = resources();
                     range::append( transaction.resources, involved);

                     if( transaction && ! involved.empty())
                     {
                        Context::involved( transaction.trid, std::move( involved));
                     }
                  }

                  //
                  // end resource
                  //
                  resources_end( *found, TMSUCCESS);
               }
               message.transaction.trid = std::move( caller);
            }
         }

         int Context::resourceRegistration( int rmid, XID* xid, long flags)
         {
            common::trace::Scope trace{ "transaction::Context::resourceRegistration", common::log::internal::transaction};

            //
            // Verify that rmid is known and is dynamic
            //
            if( ! common::range::find( m_resources.dynamic, rmid))
            {
               common::log::error << "invalid resource id " << rmid << " TMER_INVAL\n";
               return TMER_INVAL;
            }


            auto& transaction = current();

            //
            // XA-spec - RM can't reg when it's already regged... Why?
            //
            // We'll interpret this as the transaction has been suspended, and
            // then resumed.
            //
            if( common::range::find( transaction.resources, rmid))
            {
               return TM_RESUME;
            }

            //
            // Let the resource know the xid (if any)
            //
            *xid = transaction.trid.xid;


            transaction.resources.push_back( rmid);

            return TM_OK;
         }

         int Context::resourceUnregistration( int rmid, long flags)
         {
            common::trace::Scope trace{ "transaction::Context::resourceUnregistration", common::log::internal::transaction};

            auto&& transaction = current();

            //
            // RM:s can only unregister if we're outside global
            // transactions
            //
            if( ! transaction.trid)
            {
               auto found = common::range::find( transaction.resources, rmid);

               if( found)
               {
                  transaction.resources.erase( found.begin());
                  return TM_OK;
               }
            }
            return TMER_PROTO;
         }


         int Context::begin()
         {
            common::trace::Scope trace{ "transaction::Context::begin", common::log::internal::transaction};

            auto&& transaction = current();

            if( transaction.trid)
            {
               if( m_control != Control::stacked)
               {
                  throw exception::tx::Protocol{ "begin - already in transaction mode", CASUAL_NIP( transaction)};
               }

               //
               // Tell the RM:s to suspend
               //
               resources_end( transaction, TMSUSPEND);

            }
            else if( ! transaction.resources.empty())
            {
               throw exception::tx::Outside{ "begin - resources not done with work outside global transaction"}; //, exception::make_nip( "resources", range::make( transaction.resources))};
            }

            auto trans = local::startTransaction( platform::time::clock::type::now(), m_timeout);

            resources_start( trans, TMNOFLAGS);

            m_transactions.push_back( std::move( trans));

            common::log::internal::transaction << "transaction: " << m_transactions.back().trid << " started\n";

            return TX_OK;
         }


         void Context::open()
         {
            common::trace::Scope trace{ "transaction::Context::open", common::log::internal::transaction};

            std::vector< int> result;

            auto open = std::bind( &Resource::open, std::placeholders::_1, TMNOFLAGS);

            range::transform( m_resources.all, result, open);

            //
            // XA spec: if one, or more of resources opens ok, then it's not an error...
            //   seams really strange not to notify user that some of the resources has
            //   failed to open...
            //

            if( range::all_of( result, []( int value) { return value != XA_OK;}))
            {
               //throw exception::tx::Error( "failed to open all resources"); //, CASUAL_NIP( m_resources.all));
            }
         }

         void Context::close()
         {
            common::trace::Scope trace{ "transaction::Context::close", common::log::internal::transaction};

            std::vector< int> result;

            auto close = std::bind( &Resource::close, std::placeholders::_1, TMNOFLAGS);

            range::transform( m_resources.all, result, close);

            // TODO:
            // TX_PROTOCOL_ERROR
            // TX_FAIL

            /*
            if( result != TX_OK)
            {
               throw exception::tx::Error( "failed to close resources");
            }
            */
         }


         int Context::commit( const Transaction& transaction)
         {
            common::trace::Scope trace{ "transaction::Context::commit", common::log::internal::transaction};

            const auto process = process::handle();

            if( ! transaction.trid)
            {
               throw exception::tx::no::Begin{ "commit - no ongoing transaction"};
            }

            if( transaction.trid.owner() != process)
            {
               throw exception::tx::Protocol{ "commit - not owner of transaction", CASUAL_NIP( transaction)};
            }

            if( transaction.state != Transaction::State::active)
            {
               throw exception::tx::Protocol{ "commit - transaction is in rollback only mode", CASUAL_NIP( transaction)};
            }

            if( transaction.pending())
            {
               throw exception::tx::Protocol{ "commit - pending replies associated with transaction", CASUAL_NIP( transaction)};
            }


            //
            // end resources
            //
            resources_end( transaction, TMSUCCESS);


            if( transaction.local() && transaction.resources.size() + m_resources.fixed.size() <= 1)
            {
               Trace trace{ "transaction::Context::commit - local", common::log::internal::transaction};

               //
               // transaction is local, and at most one resource is involved.
               // We do the commit directly against the resource (if any).
               //
               // TODO: we could do a two-phase-commit local if the transaction is 'local'
               //

               if( ! transaction.resources.empty())
               {
                  return resource_commit( transaction.resources.front(), transaction, TMONEPHASE);
               }

               if( ! m_resources.fixed.empty())
               {
                  return resource_commit( m_resources.fixed.front().id, transaction, TMONEPHASE);
               }

               //
               // No resources associated to this transaction, hence the commit is successful.
               //
               return TX_OK;
            }
            else
            {
               Trace trace{ "transaction::Context::commit - distributed", common::log::internal::transaction};

               message::transaction::commit::Request request;
               request.trid = transaction.trid;
               request.process = process;
               request.resources = resources();
               range::append( transaction.resources, request.resources);

               //
               // Get reply
               //
               {
                  auto reply = communication::ipc::call( communication::ipc::transaction::manager::device(), request);

                  //
                  // We could get commit-reply directly in an one-phase-commit
                  //

                  switch( reply.stage)
                  {
                     case message::transaction::commit::Reply::Stage::prepare:
                     {
                        log::internal::transaction << "prepare reply: " << error::xa::error( reply.state) << '\n';

                        if( m_commit_return == Commit_Return::logged)
                        {
                           log::internal::transaction << "decision logged directive\n";

                           //
                           // Discard the coming commit-message
                           //
                           communication::ipc::inbound::device().discard( reply.correlation);
                        }
                        else
                        {
                           //
                           // Wait for the commit
                           //
                           communication::ipc::blocking::receive( communication::ipc::inbound::device(), reply, reply.correlation);

                           log::internal::transaction << "commit reply: " << error::xa::error( reply.state) << '\n';
                        }

                        break;
                     }
                     case message::transaction::commit::Reply::Stage::commit:
                     {
                        log::internal::transaction << "commit reply: " << error::xa::error( reply.state) << '\n';

                        break;
                     }
                     case message::transaction::commit::Reply::Stage::error:
                     {
                        log::error << "commit error: " << error::xa::error( reply.state) << std::endl;

                        break;
                     }
                  }

                  return xaTotx( reply.state);
               }
            }
         }

         int Context::commit()
         {
            common::trace::Scope trace{ "transaction::Context::commit", common::log::internal::transaction};

            auto result = commit( current());

            //
            // We only remove/consume transaction if commit succeed
            // TODO: any other situation we should remove?
            //
            if( result == TX_OK)
            {
               return pop_transaction();
            }

            return result;

         }

         int Context::rollback( const Transaction& transaction)
         {
            common::trace::Scope trace{ "transaction::Context::rollback", common::log::internal::transaction};

            if( ! transaction)
            {
               throw exception::tx::Protocol{ "no ongoing transaction"};
            }

            const auto process = process::handle();

            if( transaction.trid.owner() != process)
            {
               throw exception::tx::Protocol{ "current process not owner of transaction", CASUAL_NIP( transaction.trid)};
            }

            //
            // end resources
            //
            resources_end( transaction, TMSUCCESS);

            message::transaction::rollback::Request request;
            request.trid = transaction.trid;
            request.process = process;
            request.resources = resources();
            range::append( transaction.resources, request.resources);

            auto reply = communication::ipc::call( communication::ipc::transaction::manager::device(), request);

            log::internal::transaction << "rollback reply xa: " << error::xa::error( reply.state) << " tx: " << error::tx::error( xaTotx( reply.state)) << std::endl;

            return xaTotx( reply.state);

            return TX_OK;

         }

         int Context::rollback()
         {

            auto result = rollback( current());

            if( result == TX_OK)
            {
               return pop_transaction();
            }

            return result;
         }



         int Context::setCommitReturn( COMMIT_RETURN value)
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
                  return TX_EINVAL;
               }
            }
            return TX_OK;
         }

         COMMIT_RETURN Context::get_commit_return()
         {
            return static_cast< COMMIT_RETURN>( m_commit_return);
         }

         int Context::setTransactionControl(TRANSACTION_CONTROL control)
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
                  throw exception::tx::Argument{ "argument control has invalid value", CASUAL_NIP( control)};
               }
            }
            return TX_OK;
         }

         void Context::setTransactionTimeout( TRANSACTION_TIMEOUT timeout)
         {
            if( timeout < 0)
            {
               throw exception::tx::Argument{ "timeout value has to be 0 or greater", CASUAL_NIP( timeout)};
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
            common::trace::Scope trace{ "transaction::Context::suspend", common::log::internal::transaction};

            if( xid == nullptr)
            {
               throw exception::tx::Argument{ "argument xid is null"};
            }

            auto& ongoing = current();

            if( transaction::null( ongoing.trid))
            {
               throw exception::tx::Protocol{ "attempt to suspend a null xid"};
            }

            //
            // We don't check if current transaction is aborted. This differs from Tuxedo's semantics
            //

            //
            // Tell the RM:s to suspend
            //
            resources_end( ongoing, TMSUSPEND);

            *xid = ongoing.trid.xid;

            //
            // Push a null-xid to indicate suspended transaction
            //
            m_transactions.push_back( Transaction{});
         }



         void Context::resume( const XID* xid)
         {
            common::trace::Scope trace{ "transaction::Context::resume", common::log::internal::transaction};

            if( xid == nullptr)
            {
               throw exception::tx::Argument{ "argument xid is null"};
            }

            if( transaction::null( *xid))
            {
               throw exception::tx::Argument{ "attempt to resume a 'null xid'"};
            }

            auto& ongoing = current();

            if( ongoing.trid)
            {
               throw exception::tx::Protocol{ "active transaction"};
            }

            if( ! ongoing.resources.empty())
            {
               auto& global = ongoing;
               throw exception::tx::Outside{ "ongoing work outside global transaction", CASUAL_NIP( global)};
            }



            auto found = range::find( m_transactions, *xid);

            if( ! found)
            {
               throw exception::tx::Argument{ "transaction not known"};
            }


            //
            // All precondition is met, let's set wanted transaction as current
            //
            {
               //
               // Tell the RM:s to resume
               //
               resources_start( *found, TMRESUME);

               //
               // We pop the top null-xid that represent a suspended transaction
               //
               m_transactions.pop_back();

               //
               // We rotate the wanted to end;
               //
               std::rotate( std::begin( found), std::begin( found) + 1, std::end( m_transactions));
            }
         }


         void Context::resources_start( const Transaction& transaction, long flags)
         {
            Trace trace{ "transaction::Context::resources_start", common::log::internal::transaction};

            if( transaction && m_resources.fixed)
            {
               //
               // We call start only on static resources
               //

               auto start = std::bind( &Resource::start, std::placeholders::_1, std::ref( transaction), flags);
               range::for_each( m_resources.fixed, start);

            }
         }

         void Context::resources_end( const Transaction& transaction, long flags)
         {
            Trace trace{ "transaction::Context::resources_end", common::log::internal::transaction};

            if( transaction && ! m_resources.all.empty())
            {
               //
               // We call end on all resources
               //
               auto end = std::bind( &Resource::end, std::placeholders::_1, std::ref( transaction), flags);

               std::vector< int> results;
               range::transform( m_resources.all, results, end);

               // TODO: throw if some of the rm:s report an error?
            }
         }

         int Context::resource_commit( platform::resource::id::type rm, const Transaction& transaction, long flags)
         {
            Trace trace{ "transaction::Context::resources_commit", common::log::internal::transaction};

            auto commit = std::bind( &Resource::commit, std::placeholders::_1, std::ref( transaction), flags);

            for( auto& resource : m_resources.all)
            {
               if( resource.id == rm)
               {
                  return xaTotx( commit( resource));
               }
            }
            throw exception::tx::Error{ "resource id not known", CASUAL_NIP( rm), CASUAL_NIP( transaction)};

         }

         int Context::pop_transaction()
         {
            Trace trace{ "transaction::Context::pop_transaction", common::log::internal::transaction};

            //
            // Dependent on control we do different stuff
            //
            switch( m_control)
            {
               case Control::unchained:
               {
                  //
                  // Same as pop, then push null-xid
                  //
                  m_transactions.back() = Transaction{};
                  break;
               }
               case Control::chained:
               {
                  //
                  // Same as pop, then push null-xid
                  //
                  m_transactions.back() = Transaction{};

                  //
                  // We start a new one
                  //
                  return begin();
               }
               case Control::stacked:
               {
                  //
                  // Just promote the previous transaction in the stack to current
                  //
                  m_transactions.pop_back();

                  //
                  // Tell the RM:s to resume
                  //
                  resources_end( m_transactions.back(), TMRESUME);

                  break;
               }
               default:
               {
                  throw exception::tx::Fail{ "unknown control directive - this can not happen"};
               }
            }
            return TX_OK;
         }

      } // transaction
   } // common
} //casual


