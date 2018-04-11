//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

/*
 * X/Open CAE Specification
 * Distributed Transaction Processing:
 * The TX (Transaction Demarcation) Specification
 * ISBN: 1-85912-094-6
 * X/Open Document Number: C504
*/


#include "cobol/tx.h"
#include "tx.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

extern "C"
{

   void TXBEGIN( struct TXSTATUS_REC_s* status)
   {
      status->TX_STATUS = tx_begin();
   }


   void TXCLOSE( struct TXSTATUS_REC_s* status)
   {
     status->TX_STATUS = tx_close();
   }


   void TXCOMMIT( struct TXSTATUS_REC_s* status)
   {
     status->TX_STATUS = tx_commit();
   }


   void TXINFORM( struct TXINFDEF_REC_s *info, struct TXSTATUS_REC_s *status)
   {
     TXINFO txinfo;

     int32_t rv = tx_info(&txinfo);
     info->XID_REC.FORMAT_ID = txinfo.xid.formatID;
     info->XID_REC.GTRID_LENGTH = txinfo.xid.gtrid_length;
     info->XID_REC.BRANCH_LENGTH = txinfo.xid.bqual_length;
     memcpy(info->XID_REC.XID_DATA, txinfo.xid.data, XIDDATASIZE);
     /*
      * If the caller is in transaction mode, 1 is returned. If the caller
      * is not in transaction mode, 0 is returned.
     */
     if (rv == 0 || rv == 1) {
       info->TRANSACTION_MODE = rv;
       rv = TX_OK;
     }
     info->COMMIT_RETURN = txinfo.when_return;
     info->TRANSACTION_CONTROL = txinfo.transaction_control;
     info->TRANSACTION_TIMEOUT = txinfo.transaction_timeout;
     info->TRANSACTION_STATE = txinfo.transaction_state;
     status->TX_STATUS = rv;
   }


   void TXOPEN( struct TXSTATUS_REC_s* TXSTATUS_REC)
   {
       TXSTATUS_REC->TX_STATUS = tx_open();
   }


   void TXROLLBACK( struct TXSTATUS_REC_s* TXSTATUS_REC)
   {
     TXSTATUS_REC->TX_STATUS = tx_rollback();
   }


   void TXSETCOMMITRET( struct TXINFDEF_REC_s* info, struct TXSTATUS_REC_s* status)
   {
     status->TX_STATUS = tx_set_commit_return( info->COMMIT_RETURN);
   }


   void TXSETTIMEOUT( struct TXINFDEF_REC_s* info, struct TXSTATUS_REC_s* status)
   {
     status->TX_STATUS = tx_set_transaction_timeout( info->TRANSACTION_TIMEOUT);
   }


   void TXSETTRANCTL( struct TXINFDEF_REC_s* info, struct TXSTATUS_REC_s* status)
   {
     status->TX_STATUS = tx_set_transaction_control( info->TRANSACTION_CONTROL);
   }
}

