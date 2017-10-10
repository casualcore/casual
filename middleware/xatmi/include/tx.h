//!
//! casual
//!

/*
 * JBoss, Home of Professional Open Source
 * Copyright 2008, Red Hat, Inc., and others contributors as indicated
 * by the @authors tag. All rights reserved.
 * See the copyright.txt in the distribution for a
 * full listing of individual contributors.
 * This copyrighted material is made available to anyone wishing to use,
 * modify, copy, or redistribute it subject to the terms and conditions
 * of the GNU Lesser General Public License, v. 2.1.
 * This program is distributed in the hope that it will be useful, but WITHOUT A
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
 * You should have received a copy of the GNU Lesser General Public License,
 * v.2.1 along with this distribution; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301, USA.
 */

#ifndef TX_H
#define TX_H

#include "tx/code.h"
#include "xa.h"

/*
 * Definitions for tx_*() routines
 */

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

#endif // END TX_H
