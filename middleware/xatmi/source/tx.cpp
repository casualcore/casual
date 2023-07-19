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
#include "common/algorithm/compare.h"
#include "common/log/trace.h"

namespace local
{
   namespace
   {
      struct Trace : casual::common::log::Trace
      {
         template< typename T>
         Trace( T&& value) : casual::common::log::Trace( std::forward< T>( value), casual::common::log::category::transaction) {}
      };

      auto convert( casual::common::code::tx value)
      {
         return std::to_underlying( value);
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
            return convert( executer( std::forward< Args>( args)...));
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
   local::Trace trace{ "tx_begin"};
   return local::wrap( [](){
      return casual::common::transaction::Context::instance().begin();
   });
}

int tx_close()
{
   local::Trace trace{ "tx_close"};
   return local::wrap( [](){
      return casual::common::transaction::Context::instance().close();
   });
}

int tx_commit()
{
   local::Trace trace{ "tx_commit"};
   return local::wrap( [](){
      return casual::common::transaction::Context::instance().commit();
   });
}

int tx_open()
{
   local::Trace trace{ "tx_open"};
   return local::wrap( [](){
      return casual::common::transaction::Context::instance().open();
   });
}

int tx_rollback()
{
   local::Trace trace{ "tx_rollback"};
   return local::wrap( [](){
      return casual::common::transaction::Context::instance().rollback();
   });
}

int tx_set_commit_return( COMMIT_RETURN value)
{
   local::Trace trace{ "tx_set_commit_return"};
   return local::wrap( []( auto value)
   {
      using Return = casual::common::transaction::commit::Return;
      if( ! casual::common::algorithm::compare::any( Return{ value}, Return::completed, Return::logged))
         return casual::common::code::tx::argument;

      return casual::common::transaction::Context::instance().set_commit_return( Return{ value});
   }, value);
}

int tx_set_transaction_control( TRANSACTION_CONTROL value)
{
   local::Trace trace{ "tx_set_transaction_control"};
   return local::wrap( []( auto value)
   {
      using Control = casual::common::transaction::Control; 
      auto control = Control{ value};

      if( ! casual::common::algorithm::compare::any( control, Control::chained, Control::unchained, Control::stacked))
         return casual::common::code::tx::argument;
      
      return casual::common::transaction::Context::instance().set_transaction_control( control);
   }, value);
}

int tx_set_transaction_timeout( TRANSACTION_TIMEOUT timeout)
{
   local::Trace trace{ "tx_set_transaction_timeout"};
   return local::wrap( []( auto value){
      return casual::common::transaction::Context::instance().set_transaction_timeout( std::chrono::seconds{ value});
   }, timeout);
}

int tx_info( TXINFO* info)
{
   try
   {
      local::Trace trace{ "tx_info"};
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
   local::Trace trace{ "tx_suspend"};
   return local::wrap( [xid](){
      return casual::common::transaction::Context::instance().suspend( xid);
   });
}

int tx_resume( const XID* xid)
{
   local::Trace trace{ "tx_resume"};
   return local::wrap( [xid](){
      return casual::common::transaction::Context::instance().resume( xid);
   });
}


COMMIT_RETURN tx_get_commit_return()
{
   local::Trace trace{ "tx_get_commit_return"};
   return std::to_underlying( casual::common::transaction::Context::instance().get_commit_return());
}




