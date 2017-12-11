//!
//! casual
//!


#include "tx.h"

#include "common/transaction/context.h"
#include "common/code/tx.h"
#include "common/exception/tx.h"
#include "common/cast.h"

namespace local
{
   namespace
   {
      int convert( casual::common::code::tx value)
      {
         return casual::common::cast::underlying( value);
      }

      template< typename E, typename... Args>
      int wrap( E&& executer, Args&&... args)
      {
         try
         {
            executer( std::forward< Args>( args)...);
         }
         catch( ...)
         {
            return convert( casual::common::exception::tx::handle());
         }
         return convert( casual::common::code::tx::ok);
      }


   } // <unnamed>
} // local

int tx_begin()
{
   return local::wrap( [](){
      return casual::common::transaction::Context::instance().begin();
   });
}

int tx_close()
{
   return local::wrap( [](){
      casual::common::transaction::Context::instance().close();
   });
}

int tx_commit()
{
   return local::wrap( [](){
      return casual::common::transaction::Context::instance().commit();
   });
}

int tx_open()
{
   return local::wrap( [](){
      casual::common::transaction::Context::instance().open();
   });
}

int tx_rollback()
{
   return local::wrap( [](){
      return casual::common::transaction::Context::instance().rollback();
   });
}

int tx_set_commit_return(COMMIT_RETURN value)
{
   return local::wrap( []( auto value){
      casual::common::transaction::Context::instance().set_commit_return( value);
   }, value);
}

int tx_set_transaction_control(TRANSACTION_CONTROL control)
{
   return local::wrap( []( auto value){
      return casual::common::transaction::Context::instance().set_transaction_control( value);
   }, control);
}

int tx_set_transaction_timeout(TRANSACTION_TIMEOUT timeout)
{
   return local::wrap( []( auto value){
      casual::common::transaction::Context::instance().set_transaction_timeout( value);
   }, timeout);
}

int tx_info( TXINFO* info)
{
   try
   {
      return casual::common::transaction::Context::instance().info( info) ? 1 : 0;
   }
   catch( ...)
   {
      casual::common::exception::tx::handle();
      return 0; // false;
   }
}

/* casual extension */
int tx_suspend( XID* xid)
{
   return local::wrap( [xid](){
      casual::common::transaction::Context::instance().suspend( xid);
   });
}

int tx_resume( const XID* xid)
{
   return local::wrap( [xid](){
      casual::common::transaction::Context::instance().resume( xid);
   });
}


COMMIT_RETURN tx_get_commit_return()
{
   try
   {
      return casual::common::transaction::Context::instance().get_commit_return();
   }
   catch( ...)
   {
      casual::common::exception::tx::handle();
      return 0;
   }
}




