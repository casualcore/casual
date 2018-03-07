//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#ifndef CASUAL_XATMI_TX_ERROR_H_
#define CASUAL_XATMI_TX_ERROR_H_


#define TX_NOT_SUPPORTED   1   /* normal execution */
#define TX_OK              0   /* normal execution */
#define TX_OUTSIDE        -1   /* application is in an RM local transaction */
#define TX_ROLLBACK       -2   /* transaction was rolled back */
#define TX_MIXED          -3   /* transaction was partially committed and partially rolled back */
#define TX_HAZARD         -4   /* transaction may have been partially committed and partially rolled back*/
#define TX_PROTOCOL_ERROR -5   /* routine invoked in an improper context */
#define TX_ERROR          -6   /* transient error */
#define TX_FAIL           -7   /* fatal error */
#define TX_EINVAL         -8   /* invalid arguments were given */
#define TX_COMMITTED      -9   /* the transaction was heuristically committed */
#define TX_NO_BEGIN       -100 /* transaction committed plus new transaction could not be started */

#define TX_ROLLBACK_NO_BEGIN (TX_ROLLBACK+TX_NO_BEGIN) /* transaction rollback plus new transaction could not be started */
#define TX_MIXED_NO_BEGIN (TX_MIXED+TX_NO_BEGIN) /* mixed plus transaction could not be started */
#define TX_HAZARD_NO_BEGIN (TX_HAZARD+TX_NO_BEGIN) /* hazard plus transaction could not be started */
#define TX_COMMITTED_NO_BEGIN (TX_COMMITTED+TX_NO_BEGIN) /* heuristically committed plus transaction could not be started */

#endif
