/** 
 ** Copyright (c) 2015, The casual project
 **
 ** This software is licensed under the MIT license, https://opensource.org/licenses/MIT
 **/

#pragma once

/*
 * ax_() return codes (transaction manager reports to resource manager)
 */
#define TM_JOIN      2  /* caller is joining existing transaction branch */
#define TM_RESUME 1  /* caller is resuming association with suspended transaction branch */
#define TM_OK  0          /* normal execution */
#define TMER_TMERR   -1 /* an error occurred in the transaction manager */
#define TMER_INVAL   -2 /* invalid arguments were given */
#define TMER_PROTO   -3 /* routine invoked in an improper context */

/*
 * xa_() return codes (resource manager reports to transaction manager)
 */
#define XA_RBBASE 100      /* The inclusive lower bound of the rollback codes */
#define XA_RBROLLBACK   XA_RBBASE   /* The rollback was caused by an unspecified reason */
#define XA_RBCOMMFAIL   XA_RBBASE+1 /* The rollback was caused by a communication failure */
#define XA_RBDEADLOCK  XA_RBBASE+2 /* A deadlock was detected */
#define XA_RBINTEGRITY  XA_RBBASE+3 /* A condition that violates the integrity of the resources was detected */
#define XA_RBOTHER   XA_RBBASE+4 /* The resource manager rolled back the transaction for a reason not on this list */
#define XA_RBPROTO   XA_RBBASE+5 /* A protocal error occurred in the resource manager */
#define XA_RBTIMEOUT   XA_RBBASE+6 /* A transaction branch took too long*/
#define XA_RBTRANSIENT  XA_RBBASE+7 /* May retry the transaction branch */
#define XA_RBEND  XA_RBTRANSIENT /* The inclusive upper bound of the rollback codes */

#define XA_NOMIGRATE 9     /* resumption must occur where suspension occurred */
#define XA_HEURHAZ   8     /* the transaction branch may have been heuristically completed */
#define XA_HEURCOM   7     /* the transaction branch has been heuristically comitted */
#define XA_HEURRB 6     /* the transaction branch has been heuristically rolled back */
#define XA_HEURMIX   5     /* the transaction branch has been heuristically committed and rolled back */
#define XA_RETRY  4     /* routine returned with no effect and may be re-issued */
#define XA_RDONLY 3     /* the transaction was read-only and has been committed */
#define XA_OK     0     /* normal execution */
#define XAER_ASYNC   -2    /* asynchronous operation already outstanding */
#define XAER_RMERR   -3    /* a resource manager error occurred in the transaction branch */
#define XAER_NOTA -4    /* the XID is not valid */
#define XAER_INVAL   -5    /* invalid arguments were given */
#define XAER_PROTO   -6    /* routine invoked in an improper context */
#define XAER_RMFAIL  -7    /* resource manager unavailable */
#define XAER_DUPID   -8    /* the XID already exists */
#define XAER_OUTSIDE -9    /* resource manager doing work outside global transaction */


