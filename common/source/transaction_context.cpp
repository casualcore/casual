//!
//! context.cpp
//!
//! Created on: Jul 14, 2013
//!     Author: Lazan
//!

#include "common/transaction_context.h"
#include "common/logger.h"
#include "common/queue.h"

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

         Context& Context::instance()
         {
            static Context singleton;
            return singleton;
         }

         void Context::apply( const message::server::Configuration& configuration)
         {
            m_state.transactionManagerQueue = configuration.transactionManagerQueue;

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

            typedef message::server::resource::Manager RM;

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

         void Context::associateOrStart( const message::Transaction& transaction)
         {
            Transaction trans;
            trans.state = Transaction::State::active;

            queue::ipc_wrapper< queue::blocking::Writer> writeTM( m_state.transactionManagerQueue);

            if( is_null( transaction.xid))
            {

            }
            else
            {
               trans.owner = transaction.creator;
               trans.xid = transaction.xid;
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

               writeTM( message);
            }

            m_state.transactions.push( std::move( trans));
         }

         void Context::finalize( const message::service::Reply& message)
         {
         }

         Context::Context()
         {

         }

         int Context::begin()
         {
            if( ! m_state.transactions.empty())
            {
               if( m_state.transactions.top().state != Transaction::State::inactive)
               {
                  return TX_PROTOCOL_ERROR;
               }
            }

            queue::ipc_wrapper< queue::blocking::Writer> writeTM( m_state.transactionManagerQueue);

            message::transaction::begin::Request request;
            request.id.queue_id = ipc::getReceiveQueue().id();
            unique_xid( request.xid);

            writeTM( request);

            queue::blocking::Reader reader( ipc::getReceiveQueue());
            message::transaction::begin::Reply reply;
            reader( reply);

            if( reply.state == XA_OK)
            {
               Transaction trans;
               trans.state = Transaction::State::active;
               trans.owner = process::id();
               trans.xid = reply.xid;

               m_state.transactions.push( std::move( trans));
            }

            // TODO: throw instead...
            return reply.state;
         }

         int Context::open()
         {
            std::vector< int> result;

            for( auto& xa : m_state.resources)
            {
               result.push_back( xa.xaSwitch->xa_open_entry( xa.openinfo.c_str(), xa.id, TMNOFLAGS));

               if( result.back() != XA_OK)
               {
                  logger::error << xaError( result.back()) << " failed to open resource - key: " << xa.key << " id: " << xa.id << " open info: " << xa.openinfo;
               }
            }
            // TODO: TX_FAIL
            return std::all_of( std::begin( result), std::end( result), []( int value) { return value != XA_OK;}) ? TX_ERROR : TX_OK;
         }

         int Context::close()
         {
            int result = TX_OK;

            for( auto& xa : m_state.resources)
            {
               int status = xa.xaSwitch->xa_close_entry( xa.closeinfo.c_str(), xa.id, TMNOFLAGS);

               // TODO:
               // TX_PROTOCOL_ERROR
               // TX_FAIL
               if( status != XA_OK)
               {
                  logger::error << xaError( status) << " failed to close resource - key: " << xa.key << " id: " << xa.id << " close info: " << xa.closeinfo;
                  result = TX_ERROR;
               }
            }
            return result;
         }

         int Context::commit()
         {
            return TX_OK;
         }

         int Context::rollback()
         {
            return TX_OK;
         }



         int Context::setCommitReturn( COMMIT_RETURN value)
         {
            return TX_OK;
         }

         int Context::setTransactionControl(TRANSACTION_CONTROL control)
         {
            return TX_OK;
         }

         int Context::setTransactionTimeout(TRANSACTION_TIMEOUT timeout)
         {
            return TX_OK;
         }

         int Context::info( TXINFO& info)
         {
            return TX_OK;
         }

         State& Context::state()
         {
            return m_state;
         }
      } // transaction
   } // common
} //casual


