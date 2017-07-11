/*
 * X/Open CAE Specification
 * Distributed Transaction Processing:
 * The TX (Transaction Demarcation) Specification
 * ISBN: 1-85912-094-6
 * X/Open Document Number: C504
*/

/*
 * tx_cobol.c
*/

#include "../tx/tx_cobol.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <tx.h>


extern "C" void TXBEGIN(struct TXSTATUS_REC_s *TXSTATUS_REC) {
   TXSTATUS_REC->TX_STATUS = tx_begin();
}


extern "C" void TXCLOSE(struct TXSTATUS_REC_s *TXSTATUS_REC) {
  TXSTATUS_REC->TX_STATUS = tx_close();
}


extern "C" void TXCOMMIT(struct TXSTATUS_REC_s *TXSTATUS_REC) {
  TXSTATUS_REC->TX_STATUS = tx_commit();
}


extern "C" void TXINFORM(struct TXINFDEF_REC_s *TXINFDEF_REC,
                         struct TXSTATUS_REC_s *TXSTATUS_REC) {
  TXINFO txinfo;
  int rv;

  rv = tx_info(&txinfo);
  TXINFDEF_REC->XID_REC.FORMAT_ID = (int32_t)txinfo.xid.formatID;
  TXINFDEF_REC->XID_REC.GTRID_LENGTH = (int32_t)txinfo.xid.gtrid_length;
  TXINFDEF_REC->XID_REC.BRANCH_LENGTH = (int32_t)txinfo.xid.bqual_length;
  memcpy(TXINFDEF_REC->XID_REC.XID_DATA, txinfo.xid.data, XIDDATASIZE);
  /*
   * If the caller is in transaction mode, 1 is returned. If the caller
   * is not in transaction mode, 0 is returned.
  */
  if (rv == 0 || rv == 1) {
    TXINFDEF_REC->TRANSACTION_MODE = (int32_t)rv;
    rv = TX_OK;
  }
  TXINFDEF_REC->COMMIT_RETURN = (int32_t)txinfo.when_return;
  TXINFDEF_REC->TRANSACTION_CONTROL = (int32_t)txinfo.transaction_control;
  TXINFDEF_REC->TRANSACTION_TIMEOUT = (int32_t)txinfo.transaction_timeout;
  TXINFDEF_REC->TRANSACTION_STATE = (int32_t)txinfo.transaction_state;
  TXSTATUS_REC->TX_STATUS = rv;
}


extern "C" void TXOPEN(struct TXSTATUS_REC_s *TXSTATUS_REC) {
    TXSTATUS_REC->TX_STATUS = tx_open();
}


extern "C" void TXROLLBACK(struct TXSTATUS_REC_s *TXSTATUS_REC) {
  TXSTATUS_REC->TX_STATUS = tx_rollback();
}


extern "C" void TXSETCOMMITRET(struct TXINFDEF_REC_s *TXINFDEF_REC,
                               struct TXSTATUS_REC_s *TXSTATUS_REC) {
  TXSTATUS_REC->TX_STATUS = tx_set_commit_return(
      (COMMIT_RETURN)TXINFDEF_REC->COMMIT_RETURN);
}


extern "C" void TXSETTIMEOUT(struct TXINFDEF_REC_s *TXINFDEF_REC,
                             struct TXSTATUS_REC_s *TXSTATUS_REC) {
  TXSTATUS_REC->TX_STATUS = tx_set_transaction_timeout(
      (TRANSACTION_TIMEOUT)TXINFDEF_REC->TRANSACTION_TIMEOUT);
}


extern "C" void TXSETTRANCTL(struct TXINFDEF_REC_s *TXINFDEF_REC,
                             struct TXSTATUS_REC_s *TXSTATUS_REC) {
  TXSTATUS_REC->TX_STATUS = tx_set_transaction_control(
      (TRANSACTION_CONTROL)TXINFDEF_REC->TRANSACTION_CONTROL);
}

