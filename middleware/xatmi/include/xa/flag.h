#ifndef CASUAL_XA_FLAG_H_
#define CASUAL_XA_FLAG_H_

/*
 * Flag definition for the RM switch
 */
#define TMNOFLAGS    0x00000000L /* no resource manager features  selected */
#define TMREGISTER   0x00000001L /* resource manager dynamically registers */
#define TMNOMIGRATE  0x00000002L /* resource manager does not support  association migration */
#define TMUSEASYNC   0x00000004L /* resource manager supports  asynchronous operations */
/*
 * Flag definitions for xa_ and ax_ routines
 */
/* Use TMNOFLAGS, defined above, when not specifying other flags */
#define TMASYNC      0x80000000L /* perform routine asynchronously */
#define TMONEPHASE   0x40000000L /* caller is using one-phase commit optimisation */
#define TMFAIL       0x20000000L /* dissociates caller and marks transaction branch rollback-only */
#define TMNOWAIT     0x10000000L /* return if blocking condition exists */
#define TMRESUME     0x08000000L /* caller is resuming association with suspended transaction branch */
#define TMSUCCESS    0x04000000L /* dissociate caller from transaction branch */
#define TMSUSPEND    0x02000000L /* caller is suspending, not ending, association */
#define TMSTARTRSCAN 0x01000000L /* start a recovery scan */
#define TMENDRSCAN   0x00800000L /* end a recovery scan */
#define TMMULTIPLE   0x00400000L /* wait for any asynchronous operation */
#define TMJOIN       0x00200000L /* caller is joining existing transaction branch */
#define TMMIGRATE    0x00100000L /* caller intends to perform migration */

#endif
