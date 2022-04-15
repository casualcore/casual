//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!



#include "tx.h"

#include "common/transaction/context.h"
#include "common/code/tx.h"
#include "common/code/category.h"
#include "common/exception/capture.h"
#include "common/cast.h"
#include "common/algorithm/compare.h"

namespace local
{
   namespace
   {
      auto convert( casual::common::code::tx value)
      {
         return casual::common::cast::underlying( value);
      }

      auto code()
      {
         auto error = casual::common::exception::capture();

         if( casual::common::code::is::category< casual::common::code::tx>( error.code()))
            return static_cast< casual::common::code::tx>( error.code().value());

         return casual::common::code::tx::error;
      }

      auto handle()
      {         
         return convert( local::code());
      }

      template< typename E, typename... Args>
      int wrap( E&& executer, Args&&... args)
      {
         try
         {
            executer( std::forward< Args>( args)...);
            return convert( casual::common::code::tx::ok);
         }
         catch( ...)
         {
            return local::handle();
         }
      }

   } // <unnamed>
} // local

int tx_begin()
{
   return local::wrap( [](){
      casual::common::transaction::Context::instance().begin();
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
      casual::common::transaction::Context::instance().commit();
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
      casual::common::transaction::Context::instance().rollback();
   });
}

int tx_set_commit_return( COMMIT_RETURN value)
{
   return local::wrap( []( auto value)
   {
      using Return = casual::common::transaction::commit::Return;
      auto commit_return = Return{ value};
      if( ! casual::common::algorithm::compare::any( commit_return, Return::completed, Return::logged))
         casual::common::code::raise::error( casual::common::code::tx::argument);

      casual::common::transaction::Context::instance().set_commit_return( commit_return);
   }, value);
}

int tx_set_transaction_control( TRANSACTION_CONTROL value)
{
   return local::wrap( []( auto value)
   {
      using Control = casual::common::transaction::Control; 
      auto control = Control{ value};

      if( ! casual::common::algorithm::compare::any( control, Control::chained, Control::unchained, Control::stacked))
         casual::common::code::raise::error( casual::common::code::tx::argument);
      
      casual::common::transaction::Context::instance().set_transaction_control( control);
   }, value);
}

int tx_set_transaction_timeout( TRANSACTION_TIMEOUT timeout)
{
   return local::wrap( []( auto value){
      casual::common::transaction::Context::instance().set_transaction_timeout( std::chrono::seconds{ value});
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
      switch( local::code())
      {
         using code = casual::common::code::tx;
         case code::protocol: return local::convert( code::protocol);
         default: return local::convert( code::fail);
      }
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
   return casual::common::cast::underlying( casual::common::transaction::Context::instance().get_commit_return());
}




