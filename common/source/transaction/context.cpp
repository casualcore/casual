//!
//! context.cpp
//!
//! Created on: Jul 14, 2013
//!     Author: Lazan
//!

#include "common/transaction/context.h"
#include "common/call/context.h"

#include "common/queue.h"
#include "common/environment.h"
#include "common/process.h"
#include "common/internal/log.h"
#include "common/internal/trace.h"
#include "common/algorithm.h"
#include "common/error.h"
#include "common/exception.h"



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



         void handleXAresponse( std::vector< int>& result)
         {

         }



         Context& Context::instance()
         {
            static Context singleton;
            return singleton;
         }


         Context::Context()
         {

         }

         Context::Manager::Manager()
         {
            common::trace::Scope trace{ "transaction::Context::Manager::Manager", common::log::internal::transaction};

            message::transaction::client::connect::Request request;
            request.process = process::handle();
            request.path = process::path();

            log::internal::transaction << "send client connect request" << std::endl;
            queue::blocking::Writer writer( ipc::broker::id());
            auto correlation = writer( request);


            queue::blocking::Reader reader( ipc::receive::queue());
            message::transaction::client::connect::Reply reply;
            reader( reply, correlation);



            common::environment::domain::name( reply.domain);
            std::swap( resources, reply.resources);

            log::internal::transaction << "received client connect reply from broker" << std::endl;

         }


         const Context::Manager& Context::Manager::instance()
         {
            static const Manager singleton = Manager();
            return singleton;
         }

         ipc::send::Queue::id_type Context::Manager::queue() const
         {
            //
            // Will block until the TM is up.
            //
            return process::instance::transaction::manager::handle().queue;
         }


         const Context::Manager& Context::manager()
         {
            return Manager::instance();
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


         bool Context::associated( platform::descriptor_type descriptor)
         {
            for( auto& transaction : m_transactions)
            {
               if( transaction.state == Transaction::State::active && range::find( transaction.descriptors, descriptor))
               {
                  return true;
               }
            }
            return false;
         }

         void Context::set( const std::vector< Resource>& resources)
         {
            common::trace::Scope trace{ "transaction::Context::set", common::log::internal::transaction};


            using RM = message::transaction::resource::Manager;

            //
            // We don't need resources from 'manager' after this, no point
            // occupying memory -> we move it.
            //
            auto configuration = std::move( manager().resources);

            auto configRange = range::make( configuration);


            for( auto& resource : resources)
            {
               //
               // It could be several RM-configuration for one linked RM.
               //

               auto configPartition = range::stable_partition( configRange, [&]( const RM& rm){ return resource.key == rm.key;});

               if( std::get< 0>( configPartition))
               {
                  for( auto& rm : std::get< 0>( configPartition))
                  {
                     m_resources.all.emplace_back(
                           resource.key,
                           resource.xa_switch,
                           rm.id,
                           std::move( rm.openinfo),
                           std::move( rm.closeinfo));
                  }
               }
               else
               {
                  throw exception::invalid::Argument( "missing configuration for linked RM: " + resource.key + " - check group memberships");
               }

               //
               // Continue with the rest
               //
               configRange = std::get< 1>( configPartition);
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
            namespace pop
            {
               struct Guard
               {
                  Guard( std::vector< Transaction>& transactions) : m_transactions( transactions) {}
                  ~Guard() { m_transactions.erase( std::end( m_transactions));}

                  std::vector< Transaction>& m_transactions;
               };
            } // pop



            template< typename QW, typename QR>
            Transaction startTransaction( QW& writer, QR& reader, std::vector< int> resources, const platform::time_point& start, TRANSACTION_TIMEOUT timeout)
            {

               message::transaction::begin::Request request;
               request.process = process::handle();
               request.trid = transaction::ID::create();
               request.timeout = std::chrono::seconds{ timeout};
               request.start = start;
               request.resources = std::move( resources);

               writer( request);

               message::transaction::begin::Reply reply;
               reader( reply);

               if( reply.state != XA_OK)
               {
                  // TODO: more explicit exception?
                  throw exception::tx::Fail{ "failed to start transaction - " + std::string( error::xa::error( reply.state))};
               }

               Transaction transaction{ std::move( reply.trid)};
               transaction.state = Transaction::State::active;

               return transaction;
            }

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
               queue::blocking::Writer writer{ manager().queue()};

               message::transaction::resource::Involved message;
               message.process = process::handle();
               message.trid = trid;
               message.resources = std::move( resources);

               common::log::internal::transaction << "involved message: " << message << '\n';

               writer( message);
            }
         }


         void Context::join( const transaction::ID& trid)
         {
            common::trace::Scope trace{ "transaction::Context::join", common::log::internal::transaction};

            Transaction transaction( trid);

            if( trid)
            {

               involved( transaction.trid, resources());

               resources_start( transaction, TMNOFLAGS);
            }

            m_transactions.push_back( std::move( transaction));
         }

         void Context::start( const platform::time_point& start)
         {
            common::trace::Scope trace{ "transaction::Context::start", common::log::internal::transaction};

            queue::blocking::Writer writer( manager().queue());
            queue::blocking::Reader reader( ipc::receive::queue());

            auto transaction = local::startTransaction( writer, reader, resources(), start, m_timeout);

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
               transaction.discard( reply.descriptor);

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
                  transaction.discard( reply.descriptor);
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
               if( ! transaction.descriptors.empty())
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
                  for( auto& descriptor : transaction.descriptors)
                  {
                     call::Context::instance().cancel( descriptor);
                  }
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

            if( transaction.trid)
            {
               //
               // Notify TM that this RM is involved
               //
               involved( transaction.trid, { rmid});
            }

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



            queue::blocking::Writer writer( manager().queue());
            queue::blocking::Reader reader( ipc::receive::queue());



            auto trans = local::startTransaction( writer, reader, resources(), platform::clock_type::now(), m_timeout);

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

            if( ! transaction.descriptors.empty())
            {
               throw exception::tx::Protocol{ "commit - pending replies associated with transaction", CASUAL_NIP( transaction)};
            }

            //
            // end resources
            //
            resources_end( transaction, TMSUCCESS);

            message::transaction::commit::Request request;
            request.trid = transaction.trid;
            request.process = process;

            queue::blocking::Send send;

            auto correlation = send( manager().queue(), request);


            //
            // Get reply
            //
            {
               queue::blocking::Reader reader( ipc::receive::queue());

               message::transaction::commit::Reply reply;
               reader( reply, correlation);

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
                        ipc::receive::queue().discard( correlation);
                     }
                     else
                     {
                        //
                        // Wait for the commit
                        //
                        reader( reply, correlation);

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

            queue::blocking::Writer writer( manager().queue());
            writer( request);

            queue::blocking::Reader reader( ipc::receive::queue());

            message::transaction::rollback::Reply reply;
            reader( reply);

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
               //common::log::internal::transaction << "TX_EINVAL - transaction not known: " << *xid << std::endl;
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
               std::rotate( found.first, found.first + 1, std::end( m_transactions));
            }
         }


         void Context::resources_start( const Transaction& transaction, long flags)
         {
            common::trace::Scope trace{ "transaction::Context::resources_start", common::log::internal::transaction};

            if( transaction && m_resources.fixed)
            {
               //
               // We call start only on static resources
               //

               auto start = std::bind( &Resource::start, std::placeholders::_1, std::ref( transaction), flags);
               range::for_each( m_resources.fixed, start);

               // TODO: throw if some of the rm:s report an error?
            }
         }

         void Context::resources_end( const Transaction& transaction, long flags)
         {
            common::trace::Scope trace{ "transaction::Context::resources_end", common::log::internal::transaction};

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

         int Context::pop_transaction()
         {
            common::trace::Scope trace{ __func__, common::log::internal::transaction};

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


