//!
//! context.cpp
//!
//! Created on: Jul 14, 2013
//!     Author: Lazan
//!

#include "common/transaction_context.h"
#include "common/queue.h"
#include "common/environment.h"
#include "common/process.h"
#include "common/internal/log.h"
#include "common/internal/trace.h"
#include "common/algorithm.h"
#include "common/error.h"
#include "common/exception.h"

#include "xatmi.h"

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


         void unique_xid( XID& xid)
         {
            auto gtrid = Uuid::make();
            auto bqual = Uuid::make();

            std::copy( std::begin( gtrid.get()), std::end( gtrid.get()), std::begin( xid.data));
            xid.gtrid_length = std::distance(std::begin( gtrid.get()), std::end( gtrid.get()));

            std::copy( std::begin( bqual.get()), std::end( bqual.get()), std::begin( xid.data) + xid.gtrid_length);
            xid.bqual_length = xid.gtrid_length;
         }

         namespace local
         {
            namespace
            {
               XID* non_const_xid( const Transaction& transaction)
               {
                  return const_cast< XID*>( &transaction.xid.xid());
               }

            } // <unnamed>
         } // local

         int Resource::commmit( const Transaction& transaction, long flags)
         {
            log::internal::transaction << "commit resource: " << *this << " transaction: " << transaction << " flags: " << flags << '\n';

            auto result = xaSwitch->xa_commit_entry( local::non_const_xid( transaction), id, flags);

            if( result != XA_OK)
            {
               log::error << error::xa::error( result) << " failed to commit resource - " << *this << '\n';
            }
            return result;
         }

         int Resource::rollback( const Transaction& transaction, long flags)
         {
            log::internal::transaction << "rollback resource: " << *this << " transaction: " << transaction << " flags: " << flags << '\n';

            auto result = xaSwitch->xa_rollback_entry( local::non_const_xid( transaction), id, flags);

            if( result != XA_OK)
            {
               log::error << error::xa::error( result) << " failed to rollback resource - " << *this << '\n';
            }
            return result;
         }

         int Resource::start( const Transaction& transaction, long flags)
         {
            log::internal::transaction << "start resource: " << *this << " transaction: " << transaction << " flags: " << flags << '\n';

            auto result = xaSwitch->xa_start_entry( local::non_const_xid( transaction), id, flags);

            if( result != XA_OK)
            {
               log::error << error::xa::error( result) << " failed to start resource - " << *this << '\n';
            }
            return result;
         }

         int Resource::end( const Transaction& transaction, long flags)
         {
            log::internal::transaction << "end resource: " << *this << " transaction: " << transaction << " flags: " << flags << '\n';

            auto result = xaSwitch->xa_end_entry( local::non_const_xid( transaction), id, flags);

            if( result != XA_OK)
            {
               log::error << error::xa::error( result) << " failed to end resource - " << *this << '\n';
            }
            return result;

         }

         int Resource::open( long flags)
         {
            log::internal::transaction << "open resource: " << *this <<  " flags: " << flags << '\n';

            auto result = xaSwitch->xa_open_entry( openinfo.c_str(), id, flags);

            if( result != XA_OK)
            {
               log::error << error::xa::error( result) << " failed to open resource - " << *this << '\n';
            }
            return result;
         }

         int Resource::close( long flags)
         {
            log::internal::transaction << "close resource: " << *this <<  " flags: " << flags << '\n';

            auto result = xaSwitch->xa_close_entry( closeinfo.c_str(), id, flags);

            if( result != XA_OK)
            {
               log::error << error::xa::error( result) << " failed to close resource - " << *this << '\n';
            }
            return result;
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
            trace::internal::Scope trace{ "transaction::Context::Manager::Manager"};

            message::transaction::client::connect::Request request;
            request.server = message::server::Id::current();
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

         const Transaction& Context::currentTransaction() const
         {
            if( ! m_transactions.empty() && ! m_transactions.top().suspended)
            {
               return m_transactions.top();
            }
            else
            {
               static Transaction nullXid;
               return nullXid;
            }
         }


         void Context::set( const std::vector< Resource>& resources)
         {
            trace::internal::Scope trace{ "transaction::Context::set"};


            using RM = message::transaction::resource::Manager;
            auto equalKey = [] ( const Resource& r, const RM& rm){ return r.key == rm.key;};

            //
            // Sanity check
            //
            if( ! range::uniform( resources, manager().resources, equalKey))
            {
               throw exception::NotReallySureWhatToNameThisException( "missmatch between registrated/linked RM:s and configured resources");
            }

            for( auto&& rm : manager().resources)
            {
               auto found = range::find_if( resources, [&]( const Resource& r){ return r.key == rm.key;});

               if( found)
               {
                  Resource resource{ *found};
                  resource.openinfo = rm.openinfo;
                  resource.closeinfo = rm.closeinfo;
                  resource.id = rm.id;

                  m_resources.push_back( std::move( resource));
               }
               else
               {
                  throw exception::NotReallySureWhatToNameThisException( "missmatch between registrated/linked RM:s and configured resources");
               }
            }

            //
            // We don't need the manager config any more
            //
            //decltype( manager().resources) empty;
            //empty.swap( manager().resources);

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
            } // remove



            template< typename QW, typename QR>
            int startTransaction( QW& writer, QR& reader, Transaction& trans)
            {
               message::transaction::begin::Request request;
               request.id.queue_id = ipc::receive::id();
               request.xid.generate();
               writer( request);

               message::transaction::begin::Reply reply;
               reader( reply);

               if( reply.state == XA_OK)
               {
                  trans.state = Transaction::State::active;
                  trans.owner = process::id();
                  trans.xid = std::move( reply.xid);
               }

               return reply.state;
            }




         } // local

         void Context::joinOrStart( const message::Transaction& transaction)
         {
            common::trace::internal::Scope trace{ "transaction::Context::joinOrStart"};

            Transaction trans;


            if( transaction.xid)
            {
               trans.owner = transaction.creator;
               trans.xid = transaction.xid;
               trans.state = Transaction::State::active;

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

            //
            // TODO: dynamic registration
            if( ! m_resources.empty())
            {
               queue::blocking::Writer writer{ manager().queue};

               message::transaction::resource::Involved message;
               message.xid = trans.xid;

               start( trans, TMNOFLAGS);

               auto ids = std::bind( &Resource::id, std::placeholders::_1);
               range::transform( m_resources, message.resources, ids);

               writer( message);
            }

            m_transactions.push( std::move( trans));
         }

         void Context::finalize( message::service::Reply& message)
         {
            common::trace::internal::Scope trace{ "transaction::Context::finalize"};

            while( ! m_transactions.empty())
            {
               auto& transaction = m_transactions.top();

               if( message.returnValue == TPSUCCESS)
               {
                  if( transaction.owner == process::id())
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
                  if( transaction.owner == process::id())
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
            // TODO: implement
            return TM_OK;
         }

         int Context::resourceUnregistration( int rmid, long flags)
         {
            // TODO: implement
            return TM_OK;
         }





         int Context::begin()
         {
            common::trace::internal::Scope trace{ "transaction::Context::begin"};

            if( ! m_transactions.empty())
            {
               if( m_transactions.top().state != Transaction::State::inactive)
               {
                  return TX_PROTOCOL_ERROR;
               }
            }

            queue::blocking::Writer writer( manager().queue);
            queue::blocking::Reader reader( ipc::receive::queue());

            Transaction trans;

            auto code = local::startTransaction( writer, reader, trans);
            if( code  == XA_OK)
            {
               m_transactions.push( std::move( trans));

               start( trans, TMNOFLAGS);

            }

            return code;
         }

         void Context::open()
         {
            common::trace::internal::Scope trace{ "transaction::Context::open"};

            std::vector< int> result;

            auto open = std::bind( &Resource::open, std::placeholders::_1, TMNOFLAGS);

            range::transform( m_resources, result, open);

            // TODO: TX_FAIL
            if( std::all_of( std::begin( result), std::end( result), []( int value) { return value != XA_OK;}))
            {
               //throw exception::tx::Error( "failed to open resources");
            }
         }

         void Context::close()
         {
            common::trace::internal::Scope trace{ "transaction::Context::close"};

            std::vector< int> result;

            auto close = std::bind( &Resource::close, std::placeholders::_1, TMNOFLAGS);

            range::transform( m_resources, result, close);

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
            common::trace::internal::Scope trace{ "transaction::Context::commit"};

            if( transaction.xid)
            {
               message::transaction::commit::Request request;
               request.xid = transaction.xid;
               request.id = message::server::Id::current();

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
               if( reply.type() == message::transaction::commit::Reply::message_type)
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
            auto& transaction = currentTransaction();
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
            common::trace::internal::Scope trace{ "transaction::Context::rollback"};

            if( transaction.xid)
            {
               message::transaction::rollback::Request request;
               request.xid = transaction.xid;
               request.id = message::server::Id::current();

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



         void Context::setCommitReturn( COMMIT_RETURN value)
         {

         }

         void Context::setTransactionControl(TRANSACTION_CONTROL control)
         {

         }

         void Context::setTransactionTimeout(TRANSACTION_TIMEOUT timeout)
         {

         }

         void Context::info( TXINFO& info)
         {
            //return TX_OK;
         }

         void Context::start( const Transaction& transaction, long flags)
         {
            auto start = std::bind( &Resource::start, std::placeholders::_1, transaction, flags);
            range::for_each( m_resources, start);

         }

         void Context::end( const Transaction& transaction, long flags)
         {
            auto end = std::bind( &Resource::end, std::placeholders::_1, transaction, flags);
            range::for_each( m_resources, end);
         }

      } // transaction
   } // common
} //casual


