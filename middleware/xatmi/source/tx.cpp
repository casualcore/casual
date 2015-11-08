//!
//! tx.cpp
//!
//! Created on: Jul 14, 2013
//!     Author: Lazan
//!


#include "tx.h"

#include "common/transaction/context.h"
#include "common/error.h"


int tx_begin(void)
{
   try
   {
      return casual::common::transaction::Context::instance().begin();
   }
   catch( ...)
   {
      return casual::common::error::tx::handler();
   }
   return TX_OK;
}

int tx_close(void)
{
   try
   {
      casual::common::transaction::Context::instance().close();
   }
   catch( ...)
   {
      return casual::common::error::tx::handler();
   }
   return TX_OK;
}

int tx_commit(void)
{
  try
  {
     return casual::common::transaction::Context::instance().commit();
  }
  catch( ...)
  {
     return casual::common::error::tx::handler();
  }
  return TX_OK;
}

int tx_open(void)
{
  try
  {
     casual::common::transaction::Context::instance().open();
  }
  catch( ...)
  {
     return casual::common::error::tx::handler();
  }
  return TX_OK;
}

int tx_rollback(void)
{
  try
  {
     return casual::common::transaction::Context::instance().rollback();
  }
  catch( ...)
  {
     return casual::common::error::tx::handler();
  }
  return TX_OK;
}

int tx_set_commit_return(COMMIT_RETURN value)
{

   try
   {
      return casual::common::transaction::Context::instance().setCommitReturn( value);
   }
   catch( ...)
   {
      return casual::common::error::tx::handler();
   }
   return TX_OK;

}

int tx_set_transaction_control(TRANSACTION_CONTROL control)
{
  try
  {
     return casual::common::transaction::Context::instance().setTransactionControl( control);
  }
  catch( ...)
  {
     return casual::common::error::tx::handler();
  }
  return TX_OK;
}

int tx_set_transaction_timeout(TRANSACTION_TIMEOUT timeout)
{
  try
  {
     casual::common::transaction::Context::instance().setTransactionTimeout( timeout);
  }
  catch( ...)
  {
     return casual::common::error::tx::handler();
  }
  return TX_OK;
}

int tx_info( TXINFO* info)
{
   try
   {
      return casual::common::transaction::Context::instance().info( info) ? 1 : 0;
   }
   catch( ...)
   {
      return casual::common::error::tx::handler();
   }
   return TX_OK;
}

/* casual extension */
int tx_suspend( XID* xid)
{
   try
   {
      casual::common::transaction::Context::instance().suspend( xid);
   }
   catch( ...)
   {
      return casual::common::error::tx::handler();
   }
   return TX_OK;
}

int tx_resume( const XID* xid)
{
   try
   {
      casual::common::transaction::Context::instance().resume( xid);
   }
   catch( ...)
   {
      return casual::common::error::tx::handler();
   }
   return TX_OK;
}


COMMIT_RETURN tx_get_commit_return()
{
   try
   {
      return casual::common::transaction::Context::instance().get_commit_return();
   }
   catch( ...)
   {
      return casual::common::error::tx::handler();
   }
}




