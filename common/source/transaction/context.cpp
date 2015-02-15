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
            writer( request);


            queue::blocking::Reader reader( ipc::receive::queue());
            message::transaction::client::connect::Reply reply;
            reader( reply);

            queue = reply.transactionManagerQueue;
            common::environment::domain::name( reply.domain);
            std::swap( resources, reply.resourceManagers);

            log::internal::transaction << "received client connect reply from broker" << std::endl;

         }


         const Context::Manager& Context::Manager::instance()
         {
            static const Manager singleton = Manager();
            return singleton;
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
                           resource.xaSwitch,
                           rm.id,
                           std::move( rm.openinfo),
                           std::move( rm.closeinfo));
                  }
               }
               else
               {
                  throw exception::invalid::Argument( "missing configuration for linked RM: " + resource.key + " - check group memberships", __FILE__, __LINE__);
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
            Transaction startTransaction( QW& writer, QR& reader)
            {

               message::transaction::begin::Request request;
               request.process = process::handle();
               request.trid = transaction::ID::create();
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

         void Context::involved( const transaction::ID& id, std::vector< int> resources)
         {
            common::trace::Scope trace{ "transaction::Context::involved", common::log::internal::transaction};

            common::log::internal::transaction << "resources involved: " << common::range::make( resources) << std::endl;

            queue::blocking::Writer writer{ manager().queue};

            message::transaction::resource::Involved message;
            message.trid = id;
            message.resources = std::move( resources);

            writer( message);
         }


         void Context::join( const transaction::ID& trid)
         {
            common::trace::Scope trace{ "transaction::Context::join", common::log::internal::transaction};

            Transaction transaction( trid);

            resources_start( transaction, TMNOFLAGS);

            m_transactions.push_back( std::move( transaction));
         }

         void Context::start()
         {
            common::trace::Scope trace{ "transaction::Context::start", common::log::internal::transaction};

            queue::blocking::Writer writer( manager().queue);
            queue::blocking::Reader reader( ipc::receive::queue());

            auto transaction = local::startTransaction( writer, reader);

            resources_start( transaction, TMNOFLAGS);

            m_transactions.push_back( std::move( transaction));
         }


         void Context::update( message::service::Reply& reply)
         {
            if( reply.transaction.trid)
            {
               auto found = range::find( m_transactions, reply.transaction.trid);

               if( ! found)
               {
                  throw exception::xatmi::SystemError{ "failed to find transaction", __FILE__, __LINE__};
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
         }

         void Context::finalize( message::service::Reply& message, int return_state)
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
                  log::error << "pending replies associated with transaction - action: transaction state to rollback only\n";
                  log::internal::transaction << transaction << std::endl;

                  transaction.state = Transaction::State::rollback;
                  message.error = TPESVCERR;

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

            if( return_state != TPSUCCESS)
            {
               message.error = TPESVCFAIL;
            }


            //
            // Check pending calls
            //
            range::for_each( transactions, pending_check);



            auto owner_split = range::stable_partition( transactions, [&]( const Transaction& transaction){
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
            if( common::range::find( transaction.resources, rmid))
            {
               return TMER_PROTO;
            }

            if( transaction.trid)
            {
               //
               // Notify TM that this RM is involved
               //
               involved( transaction.trid, { rmid});
            }

            transaction.resources.push_back( rmid);

            *xid = transaction.trid.xid;

            if( transaction.suspended)
            {
               return TM_RESUME;
            }

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

            if( ! transaction.resources.empty())
            {
               return TX_OUTSIDE;
            }

            if( ! transaction.suspended)
            {
               return TX_PROTOCOL_ERROR;
            }

            queue::blocking::Writer writer( manager().queue);
            queue::blocking::Reader reader( ipc::receive::queue());


            auto trans = local::startTransaction( writer, reader);

            resources_start( trans, TMNOFLAGS);

            m_transactions.push_back( std::move( trans));

            common::log::internal::transaction << "transaction: " << m_transactions.back().trid << " started\n";

            return TX_OK;
         }


         int Context::open()
         {
            common::trace::Scope trace{ "transaction::Context::open", common::log::internal::transaction};

            std::vector< int> result;

            auto open = std::bind( &Resource::open, std::placeholders::_1, TMNOFLAGS);

            range::transform( m_resources.all, result, open);

            // TODO: TX_FAIL
            if( std::all_of( std::begin( result), std::end( result), []( int value) { return value != XA_OK;}))
            {
               //throw exception::tx::Error( "failed to open resources");
            }
            return TX_OK;
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

            if( transaction.trid.owner() != process)
            {
               log::internal::transaction << "commit - current process not owner of transaction " << transaction << " - " << error::tx::error( TX_PROTOCOL_ERROR) << std::endl;
               return TX_PROTOCOL_ERROR;
            }

            if( ! transaction.descriptors.empty())
            {
               log::error << "commit - pending replies " << range::make( transaction.descriptors) << " associated with transaction " << transaction << " - " << error::tx::error( TX_PROTOCOL_ERROR) << std::endl;
               return TX_PROTOCOL_ERROR;
            }


            message::transaction::commit::Request request;
            request.trid = transaction.trid;
            request.process = process;

            queue::blocking::Writer writer( manager().queue);
            writer( request);


            queue::blocking::Reader reader( ipc::receive::queue());

            //
            // We could get commit-reply directly in an one-phase-commit
            //

            auto reply = reader.next( {
               message::transaction::prepare::Reply::message_type,
               message::transaction::commit::Reply::message_type});

            //
            // Did we get commit reply directly?
            //
            if( reply.type == message::transaction::commit::Reply::message_type)
            {
               message::transaction::commit::Reply commitReply;
               reply >> commitReply;

               log::internal::transaction << "commit reply xa: " << error::xa::error( commitReply.state) << " tx: " << error::tx::error( xaTotx( commitReply.state)) << std::endl;

               return xaTotx( commitReply.state);
            }
            else
            {

               message::transaction::prepare::Reply prepareReply;
               reply >> prepareReply;

               log::internal::transaction << "prepare reply: " << error::xa::error( prepareReply.state) << std::endl;


               if( prepareReply.state == XA_OK)
               {
                  //
                  // Now we wait for the commit
                  //
                  message::transaction::commit::Reply commitReply;
                  reader( commitReply);

                  log::internal::transaction << "commit reply xa: " << error::xa::error( commitReply.state) << " tx: " << error::tx::error( xaTotx( commitReply.state)) << std::endl;

                  return xaTotx( commitReply.state);
               }
               return xaTotx( prepareReply.state);

            }



            return TX_OK;
         }

         int Context::commit()
         {
            if( m_transactions.empty())
            {
               log::internal::transaction << "commit - no ongoing transaction: " << error::tx::error( TX_NO_BEGIN) << std::endl;
               return TX_NO_BEGIN;
            }


            auto result = commit( m_transactions.back());

            //
            // We only remove/consume transaction if commit succeed
            // TODO: any other situation we should remove?
            //
            if( result == TX_OK)
            {
               m_transactions.pop_back();
            }

            return result;

         }

         int Context::rollback( const Transaction& transaction)
         {
            common::trace::Scope trace{ "transaction::Context::rollback", common::log::internal::transaction};

            if( ! transaction)
            {
               log::error << "rollback - no ongoing transaction: " << error::tx::error( TX_PROTOCOL_ERROR) << std::endl;
               return TX_PROTOCOL_ERROR;
            }

            const auto process = process::handle();

            if( transaction.trid.owner() != process)
            {
               log::internal::transaction << "rollback - current process not owner of transaction " << transaction << " - " << error::tx::error( TX_PROTOCOL_ERROR) << std::endl;
               return TX_PROTOCOL_ERROR;
            }

            message::transaction::rollback::Request request;
            request.trid = transaction.trid;
            request.process = process;

            queue::blocking::Writer writer( manager().queue);
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
            auto& transaction = current();

            auto result = rollback( transaction);

            if( transaction)
            {
               m_transactions.pop_back();
            }

            return result;
         }



         int Context::setCommitReturn( COMMIT_RETURN value)
         {
            if( value == TX_COMMIT_COMPLETED)
            {
               return TX_OK;
            }

            return TX_NOT_SUPPORTED;
         }

         int Context::setTransactionControl(TRANSACTION_CONTROL control)
         {
            // TODO:
            return TX_OK;
         }

         int Context::setTransactionTimeout(TRANSACTION_TIMEOUT timeout)
         {
            // TODO:
            return TX_OK;
         }

         int Context::info( TXINFO& info)
         {
            auto&& transaction = current();

            info.xid = transaction.trid.xid;
            info.transaction_state = static_cast< decltype( info.transaction_state)>( transaction.state);


            return transaction ? 1 : 0;
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


               //
               // Notify the TM about the involved RM:s
               //
               {
                  std::vector< int> resources;

                  auto ids = std::mem_fn( &Resource::id);
                  range::transform( m_resources.fixed, resources, ids);

                  involved( transaction.trid, std::move( resources));
               }
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
               range::for_each( m_resources.all, end);
            }
         }

      } // transaction
   } // common
} //casual


