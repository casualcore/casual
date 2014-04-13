//!
//! context.cpp
//!
//! Created on: Jul 14, 2013
//!     Author: Lazan
//!

#include "common/transaction_context.h"
#include "common/queue.h"
#include "common/internal/log.h"
#include "common/internal/trace.h"

#include <map>
#include <algorithm>

namespace casual
{
   namespace common
   {

      namespace transaction
      {

         const char* xaError( int code)
         {
            static const std::map< int, const char*> mapping{
               { XA_RBROLLBACK, "XA_RBROLLBACK"},
               { XA_RBCOMMFAIL, "XA_RBCOMMFAIL"},
               { XA_RBDEADLOCK, "XA_RBDEADLOCK"},
               { XA_RBINTEGRITY, "XA_RBINTEGRITY"},
               { XA_RBOTHER, "XA_RBOTHER"},
               { XA_RBPROTO, "XA_RBPROTO"},
               { XA_RBTIMEOUT, "XA_RBTIMEOUT"},
               { XA_RBTRANSIENT, "XA_RBTRANSIENT"},
               { XA_NOMIGRATE, "XA_NOMIGRATE"},
               { XA_HEURHAZ, "XA_HEURHAZ"},
               { XA_HEURCOM, "XA_HEURCOM"},
               { XA_HEURRB, "XA_HEURRB"},
               { XA_HEURMIX, "XA_HEURMIX"},
               { XA_RETRY, "XA_RETRY"},
               { XA_RDONLY, "XA_RDONLY"},
               { XA_OK, "XA_OK"},
               { XAER_ASYNC, "XAER_ASYNC"},
               { XAER_RMERR, "XAER_RMERR"},
               { XAER_NOTA, "XAER_NOTA"},
               { XAER_INVAL, "XAER_INVAL"},
               { XAER_PROTO, "XAER_PROTO"},
               { XAER_RMFAIL, "XAER_RMFAIL"},
               { XAER_DUPID, "XAER_DUPID"},
               { XAER_OUTSIDE, "XAER_OUTSIDE"}
            };

            return mapping.at( code);
         }



         const char* txError( int code)
         {
            static const std::map< int, const char*> mapping{
               { TX_NOT_SUPPORTED, "TX_NOT_SUPPORTED"},
               { TX_OK, "TX_OK"},
               { TX_OUTSIDE, "TX_OUTSIDE"},
               { TX_ROLLBACK, "TX_ROLLBACK"},
               { TX_MIXED, "TX_MIXED"},
               { TX_HAZARD, "TX_HAZARD"},
               { TX_PROTOCOL_ERROR, "TX_PROTOCOL_ERROR"},
               { TX_ERROR, "TX_ERROR"},
               { TX_FAIL, "TX_FAIL"},
               { TX_EINVAL, "TX_EINVAL"},
               { TX_COMMITTED, "TX_COMMITTED"},
               { TX_NO_BEGIN, "TX_NO_BEGIN"},
               { TX_ROLLBACK_NO_BEGIN, "TX_ROLLBACK_NO_BEGIN"},
               { TX_MIXED_NO_BEGIN, "TX_MIXED_NO_BEGIN"},
               { TX_HAZARD_NO_BEGIN, "TX_HAZARD_NO_BEGIN"},
               { TX_COMMITTED_NO_BEGIN, "TX_COMMITTED_NO_BEGIN"}
            };

            return mapping.at( code);
         }


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

         int Resource::nextResurceId()
         {
            static int id = 1;
            return id++;
         }

         namespace local
         {
            namespace
            {
               ipc::send::Queue::id_type initializeTMQueueId()
               {

                  return 0;
               }

            } // <unnamed>
         } // local

         ipc::send::Queue::id_type State::manager()
         {
            const static ipc::send::Queue::id_type id = local::initializeTMQueueId();
            return id;
         }


         Context& Context::instance()
         {
            static Context singleton;
            return singleton;
         }

         void Context::apply( const message::server::Configuration& configuration)
         {
            //m_state.transactionManagerQueue = configuration.transactionManagerQueue;

            //
            // Local copy
            //
            auto conf = configuration.resourceManagers;

            //
            // Sanity check
            //
            if( conf.size() != m_state.resources.size())
            {
               throw exception::NotReallySureWhatToNameThisException( "missmatch between registrated/linked RM:s and configured resources");
            }


            using RM = message::resource::Manager;

            for( Resource& rm : m_state.resources)
            {
               auto findIter = std::find_if(
                     std::begin( conf), std::end( conf), [&]( const RM& r){ return r.key == rm.key;});

               if( findIter != std::end( conf))
               {
                  rm.openinfo = findIter->openinfo;
                  rm.closeinfo = findIter->closeinfo;

                  //
                  // this rm-configuration is done
                  //
                  conf.erase( findIter);
               }
               else
               {
                  throw exception::NotReallySureWhatToNameThisException( "missmatch between registrated/linked RM:s and configured resources");
               }
            }
         }


         const Transaction& Context::currentTransaction() const
         {
            if( ! m_state.transactions.empty() && ! m_state.transactions.top().suspended)
            {
               return m_state.transactions.top();
            }
            else
            {
               static Transaction nullXid;
               return nullXid;
            }
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
               request.id.queue_id = ipc::getReceiveQueue().id();
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

         void Context::associateOrStart( const message::Transaction& transaction)
         {
            common::Trace trace{ "transaction::Context::associateOrStart"};

            Transaction trans;


            queue::blocking::Writer writer{ m_state.manager()};

            if( transaction.xid)
            {
               trans.owner = transaction.creator;
               trans.xid = transaction.xid;
               trans.state = Transaction::State::active;

               //auto code = local::startTransaction( writer, reader, trans);
            }
            else
            {
               queue::blocking::Reader reader( ipc::getReceiveQueue());
            }

            //
            // TODO: dynamic registration
            if( ! m_state.resources.empty())
            {
               message::transaction::resource::Involved message;
               message.xid = trans.xid;

               for( auto& resource : m_state.resources)
               {
                  message.resources.push_back( resource.id);
               }

               writer( message);
            }

            m_state.transactions.push( std::move( trans));
         }

         void Context::finalize( const message::service::Reply& message)
         {
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

         Context::Context()
         {

         }



         int Context::begin()
         {
            common::trace::internal::Scope trace{ "transaction::Context::begin"};

            if( ! m_state.transactions.empty())
            {
               if( m_state.transactions.top().state != Transaction::State::inactive)
               {
                  return TX_PROTOCOL_ERROR;
               }
            }

            queue::blocking::Writer writer( m_state.manager());
            queue::blocking::Reader reader( ipc::getReceiveQueue());

            Transaction trans;

            auto code = local::startTransaction( writer, reader, trans);
            if( code  == XA_OK)
            {
               m_state.transactions.push( std::move( trans));
            }

            return code;
         }

         void Context::open()
         {
            common::trace::internal::Scope trace{ "transaction::Context::open"};

            std::vector< int> result;

            for( auto& xa : m_state.resources)
            {
               result.push_back( xa.xaSwitch->xa_open_entry( xa.openinfo.c_str(), xa.id, TMNOFLAGS));

               if( result.back() != XA_OK)
               {
                  log::error << xaError( result.back()) << " failed to open resource - key: " << xa.key << " id: " << xa.id << " open info: " << xa.openinfo << std::endl;
               }
            }
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

            for( auto& xa : m_state.resources)
            {
               result.push_back( xa.xaSwitch->xa_close_entry( xa.closeinfo.c_str(), xa.id, TMNOFLAGS));

               // TODO:
               // TX_PROTOCOL_ERROR
               // TX_FAIL
               if( result.back() != XA_OK)
               {
                  log::error << xaError( result.back()) << " failed to close resource - key: " << xa.key << " id: " << xa.id << " close info: " << xa.closeinfo;
               }
            }
            /*
            if( result != TX_OK)
            {
               throw exception::tx::Error( "failed to close resources");
            }
            */
         }

         int Context::commit()
         {
            common::trace::internal::Scope trace{ "transaction::Context::commit"};

            auto transaction = currentTransaction();

            if( transaction.xid)
            {
               //
               // Regardless what happens we'll remove the current transaction
               //
               local::pop::Guard popGuard( m_state.transactions);

               message::transaction::commit::Request request;
               request.xid = transaction.xid;
               request.id = message::server::Id::current();

               queue::blocking::Writer writer( m_state.manager());
               writer( request);




               queue::blocking::Reader reader( ipc::getReceiveQueue());

               //
               // First we receive the prepare-response
               //

               message::transaction::prepare::Reply prepareReply;
               reader( prepareReply);

               log::internal::transaction << "prepare reply: " << xaError( prepareReply.state) << std::endl;


               if( prepareReply.state == XA_OK)
               {
                  //
                  // Now we wait for the commit
                  //
                  message::transaction::commit::Reply commitReply;
                  reader( commitReply);

                  log::internal::transaction << "commit reply xa: " << xaError( commitReply.state) << " tx: " << txError( xaTotx( commitReply.state)) << std::endl;

                  return xaTotx( commitReply.state);
               }
               return xaTotx( prepareReply.state);
            }
            else
            {
               log::internal::transaction << "no ongoing transaction: " << txError( TX_NO_BEGIN) << std::endl;
               return TX_NO_BEGIN;
            }

            return TX_OK;
         }

         int Context::rollback()
         {
            common::trace::internal::Scope trace{ "transaction::Context::rollback"};

            auto transaction = currentTransaction();

            if( transaction.xid)
            {

               //
               // Regardless what happens we'll remove the current transaction
               //
               local::pop::Guard popGuard( m_state.transactions);

               message::transaction::rollback::Request request;
               request.xid = transaction.xid;
               request.id = message::server::Id::current();

               queue::blocking::Writer writer( m_state.manager());
               writer( request);

               queue::blocking::Reader reader( ipc::getReceiveQueue());

               message::transaction::rollback::Reply reply;
               reader( reply);

               log::internal::transaction << "rollback reply xa: " << xaError( reply.state) << " tx: " << txError( xaTotx( reply.state)) << std::endl;

               return xaTotx( reply.state);
            }
            else
            {
               log::internal::transaction << "no ongoing transaction: " << txError( TX_NO_BEGIN) << std::endl;
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

         State& Context::state()
         {
            return m_state;
         }
      } // transaction
   } // common
} //casual


