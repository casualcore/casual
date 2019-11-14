/** 
 ** Copyright (c) 2015, The casual project
 **
 ** This software is licensed under the MIT license, https://opensource.org/licenses/MIT
 **/

#pragma once

#include "casual/tx/code.h"
#include "casual/xa.h"


/* commit return values */
typedef long COMMIT_RETURN;
#define TX_COMMIT_COMPLETED 0
#define TX_COMMIT_DECISION_LOGGED 1

/* transaction control values */
typedef long TRANSACTION_CONTROL;
#define TX_UNCHAINED 0
#define TX_CHAINED 1
/* casual extension */
#define TX_STACKED 42


/* type of transaction timeouts */
typedef long TRANSACTION_TIMEOUT;

/* transaction state values */
typedef long TRANSACTION_STATE;
#define TX_ACTIVE 0
#define TX_TIMEOUT_ROLLBACK_ONLY 1
#define TX_ROLLBACK_ONLY 2

/* structure populated by tx_info() */
struct tx_info_t {
 XID                 xid;
 COMMIT_RETURN       when_return;
 TRANSACTION_CONTROL transaction_control;
 TRANSACTION_TIMEOUT transaction_timeout;
 TRANSACTION_STATE   transaction_state;
};
typedef struct tx_info_t TXINFO;

#ifdef __cplusplus
extern "C" {
#endif
   extern int tx_begin(void);
   extern int tx_close(void);
   extern int tx_commit(void);
   extern int tx_open(void);
   extern int tx_rollback(void);
   extern int tx_set_commit_return(COMMIT_RETURN);
   extern int tx_set_transaction_control(TRANSACTION_CONTROL control);
   extern int tx_set_transaction_timeout(TRANSACTION_TIMEOUT timeout);
   extern int tx_info(TXINFO *);

   /* casual extension */
   extern int tx_suspend( XID* xid);
   extern int tx_resume( const XID* xid);
   extern COMMIT_RETURN tx_get_commit_return();

#ifdef __cplusplus
}
#endif

