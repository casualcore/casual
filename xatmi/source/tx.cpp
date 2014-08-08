//!
//! tx.cpp
//!
//! Created on: Jul 14, 2013
//!     Author: Lazan
//!


#include "tx.h"

#include "common/transaction_context.h"
#include "common/error.h"


int tx_begin(void)
{
   try
   {
      casual::common::transaction::Context::instance().begin();
   }
   catch( ...)
   {
      return casual::common::error::handler();
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
      return casual::common::error::handler();
   }
   return TX_OK;
}

int tx_commit(void)
{
  try
  {
     casual::common::transaction::Context::instance().commit();
  }
  catch( ...)
  {
     return casual::common::error::handler();
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
     return casual::common::error::handler();
  }
  return TX_OK;
}

int tx_rollback(void)
{
  try
  {
     casual::common::transaction::Context::instance().rollback();
  }
  catch( ...)
  {
     return casual::common::error::handler();
  }
  return TX_OK;
}

int tx_set_commit_return(COMMIT_RETURN value)
{

  if( value == TX_COMMIT_COMPLETED)
  {
     return TX_OK;
  }
  else
  {
     return TX_NOT_SUPPORTED;
  }

}

int tx_set_transaction_control(TRANSACTION_CONTROL control)
{
  try
  {
     casual::common::transaction::Context::instance().setTransactionControl( control);
  }
  catch( ...)
  {
     return casual::common::error::handler();
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
     return casual::common::error::handler();
  }
  return TX_OK;
}

int tx_info( TXINFO* info)
{
   try
   {
      return casual::common::transaction::Context::instance().info( *info);
   }
   catch( ...)
   {
      casual::common::error::handler();
   }
   return 0;
}



