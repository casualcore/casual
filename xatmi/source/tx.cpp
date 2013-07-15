//!
//! tx.cpp
//!
//! Created on: Jul 14, 2013
//!     Author: Lazan
//!


#include "tx.h"

#include "transaction/context.h"


int tx_begin(void)
{
   try
   {
      return casual::transaction::Context::instance().begin();
   }
   catch( ...)
   {
      return casual::common::error::handler();
   }
}

int tx_close(void)
{
   try
   {
      return casual::transaction::Context::instance().close();
   }
   catch( ...)
   {
      return casual::common::error::handler();
   }
}

int tx_commit(void)
{
  try
  {
     return casual::transaction::Context::instance().commit();
  }
  catch( ...)
  {
     return casual::common::error::handler();
  }
}

int tx_open(void)
{
  try
  {
     return casual::transaction::Context::instance().open();
  }
  catch( ...)
  {
     return casual::common::error::handler();
  }
}

int tx_rollback(void)
{
  try
  {
     return casual::transaction::Context::instance().rollback();
  }
  catch( ...)
  {
     return casual::common::error::handler();
  }
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
     return casual::transaction::Context::instance().setTransactionControl( control);
  }
  catch( ...)
  {
     return casual::common::error::handler();
  }
}

int tx_set_transaction_timeout(TRANSACTION_TIMEOUT timeout)
{
  try
  {
     return casual::transaction::Context::instance().setTransactionTimeout( timeout);
  }
  catch( ...)
  {
     return casual::common::error::handler();
  }
}

int tx_info( TXINFO* info)
{
   try
   {
      return casual::transaction::Context::instance().info( *info);
   }
   catch( ...)
   {
      return casual::common::error::handler();
   }
}



