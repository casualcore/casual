//!
//! context.cpp
//!
//! Created on: Jul 14, 2013
//!     Author: Lazan
//!

#include "common/transaction/context.h"

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

            return m_transactions.top();
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
                  Guard( std::stack< Transaction>& transactions) : m_transactions( transactions) {}
                  ~Guard() { m_transactions.pop();}

                  std::stack< Transaction>& m_transactions;
               };
            } // pop



            template< typename QW, typename QR>
            int startTransaction( QW& writer, QR& reader, Transaction& trans)
            {
               message::transaction::begin::Request request;
               request.process = process::handle();
               request.trid = transaction::ID::create();
               writer( request);

               message::transaction::begin::Reply reply;
               reader( reply);

               if( reply.state == XA_OK)
               {
                  trans.state( Transaction::State::active);
                  trans.trid = std::move( reply.trid);
               }

               return reply.state;
            }

         } // local

         void Context::involved( transaction::ID& id, std::vector< int> resources)
         {
            common::trace::Scope trace{ "transaction::Context::involved", common::log::internal::transaction};

            common::log::internal::transaction << "resources involved: " << common::range::make( resources) << std::endl;

            queue::blocking::Writer writer{ manager().queue};

            message::transaction::resource::Involved message;
            message.trid = id;
            message.resources = std::move( resources);

            writer( message);
         }

         void Context::joinOrStart( const transaction::ID& transaction)
         {
            common::trace::Scope trace{ "transaction::Context::joinOrStart", common::log::internal::transaction};

            Transaction trans;


            if( transaction)
            {
               trans.trid = transaction;
               trans.state( Transaction::State::active);

               //auto code = local::startTransaction( writer, reader, trans);
            }
            else
            {
               queue::blocking::Writer writer( manager().queue);
               queue::blocking::Reader reader( ipc::receive::queue());

               auto code = local::startTransaction( writer, reader, trans);
               if( code  == XA_OK)
               {
                  // TODO:
               }
            }

            if( m_resources.fixed)
            {
               start( trans, TMNOFLAGS);

               std::vector< int> resources;

               auto ids = std::mem_fn( &Resource::id);
               range::transform( m_resources.fixed, resources, ids);

               involved( trans.trid, std::move( resources));

            }

            m_transactions.push( std::move( trans));
         }

         void Context::finalize( message::service::Reply& message)
         {
            common::trace::Scope trace{ "transaction::Context::finalize", common::log::internal::transaction};

            while( ! m_transactions.empty())
            {
               auto& transaction = m_transactions.top();

               if( message.returnValue == TPSUCCESS)
               {
                  if( transaction.trid.owner() == process::handle())
                  {
                     if( commit( transaction) != XA_OK)
                     {
                        message.returnValue = TPESVCERR;
                     }
                  }
                  end( transaction, TMSUCCESS);
               }
               else
               {
                  if( transaction.trid.owner() == process::handle())
                  {
                     if( rollback( transaction) != XA_OK)
                     {
                        message.returnValue = TPESVCERR;
                     }
                  }
                  end( transaction, TMFAIL);
               }

               m_transactions.pop();
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
            if( common::range::find( transaction.associated, rmid))
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

            transaction.associated.push_back( rmid);

            *xid = transaction.trid.xid;

            switch( transaction.previous())
            {
               case Transaction::State::suspended:
               {
                  return TM_RESUME;
               }
               default:
               {
                  return TM_OK;
               }
            }
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
               auto found = common::range::find( transaction.associated, rmid);

               if( found)
               {
                  transaction.associated.erase( found.begin());
                  return TM_OK;
               }
            }
            return TMER_PROTO;
         }


         int Context::begin()
         {
            common::trace::Scope trace{ "transaction::Context::begin", common::log::internal::transaction};

            auto&& transaction = current();

            if( ! transaction.associated.empty())
            {
               return TX_OUTSIDE;
            }

            if( transaction.state() != Transaction::State::inactive)
            {
               return TX_PROTOCOL_ERROR;
            }

            queue::blocking::Writer writer( manager().queue);
            queue::blocking::Reader reader( ipc::receive::queue());

            Transaction trans;

            auto code = local::startTransaction( writer, reader, trans);
            if( code  == XA_OK)
            {

               m_transactions.push( trans);

               start( trans, TMNOFLAGS);

               common::log::internal::transaction << "transaction: " << trans.trid << " started\n";
            }

            return code;
         }

         void Context::open()
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

            if( transaction.trid)
            {
               message::transaction::commit::Request request;
               request.trid = transaction.trid;
               request.process = process::handle();

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
            }
            else
            {
               log::internal::transaction << "no ongoing transaction: " << error::tx::error( TX_NO_BEGIN) << std::endl;
               return TX_NO_BEGIN;
            }

            return TX_OK;
         }

         int Context::commit()
         {
            auto& transaction = current();
            if( transaction)
            {
               //
               // Regardless what happens we'll remove the current transaction
               //
               local::pop::Guard popGuard( m_transactions);

               return commit( m_transactions.top());
            }
            else
            {
               log::internal::transaction << "no ongoing transaction: " << error::tx::error( TX_PROTOCOL_ERROR) << std::endl;
               return TX_PROTOCOL_ERROR;
            }

            return TX_OK;
         }

         int Context::rollback( const Transaction& transaction)
         {
            common::trace::Scope trace{ "transaction::Context::rollback", common::log::internal::transaction};

            if( transaction.trid)
            {
               message::transaction::rollback::Request request;
               request.trid = transaction.trid;
               request.process = process::handle();

               queue::blocking::Writer writer( manager().queue);
               writer( request);

               queue::blocking::Reader reader( ipc::receive::queue());

               message::transaction::rollback::Reply reply;
               reader( reply);

               log::internal::transaction << "rollback reply xa: " << error::xa::error( reply.state) << " tx: " << error::tx::error( xaTotx( reply.state)) << std::endl;

               return xaTotx( reply.state);
            }
            else
            {
               log::internal::transaction << "no ongoing transaction: " << error::tx::error( TX_NO_BEGIN) << std::endl;
               return TX_NO_BEGIN;
            }
            return TX_OK;

         }

         int Context::rollback()
         {
            if( ! m_transactions.empty())
            {
               //
               // Regardless what happens we'll remove the current transaction
               //
               local::pop::Guard popGuard( m_transactions);

               return rollback( m_transactions.top());
            }
            else
            {
               log::internal::transaction << "no ongoing transaction: " << error::tx::error( TX_NO_BEGIN) << std::endl;
               return TX_NO_BEGIN;
            }

            return TX_OK;
         }



         int Context::setCommitReturn( COMMIT_RETURN value)
         {
            if( value == TX_COMMIT_COMPLETED)
            {
               return TX_OK;
            }

            return TX_NOT_SUPPORTED;
         }

         void Context::setTransactionControl(TRANSACTION_CONTROL control)
         {

         }

         void Context::setTransactionTimeout(TRANSACTION_TIMEOUT timeout)
         {

         }

         int Context::info( TXINFO& info)
         {
            auto&& transaction = current();

            info.xid = transaction.trid.xid;


            return transaction ? 1 : 0;
         }

         void Context::start( Transaction& transaction, long flags)
         {
            common::trace::Scope trace{ "transaction::Context::start", common::log::internal::transaction};

            //
            // We call start only on static resources
            //

            auto start = std::bind( &Resource::start, std::placeholders::_1, transaction, flags);
            range::for_each( m_resources.fixed, start);
         }

         void Context::end( const Transaction& transaction, long flags)
         {
            common::trace::Scope trace{ "transaction::Context::end", common::log::internal::transaction};

            //
            // We call end on all resources
            //
            auto end = std::bind( &Resource::end, std::placeholders::_1, transaction, flags);
            range::for_each( m_resources.all, end);
         }

      } // transaction
   } // common
} //casual


