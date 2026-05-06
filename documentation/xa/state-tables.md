# XA State Tables (Chapter 6)

This file contains the state tables from Chapter 6 of the X/Open XA specification, 
rendered as plain ASCII tables for convenient reference. For the full specification,
see [Distributed Transaction Processing: The XA Specification](https://pubs.opengroup.org/onlinepubs/009680699/toc.pdf).

---

## Table 6-1 – State Table for Resource Manager Initialisation

Resource Manager States

| XA routine | Un-initialised (R0) | Initialised (R1) |
|-----------:|---------------------|------------------|
| xa_open()  | R1                  | R1               |
| xa_close() | R0                  | R0               |

---

## Table 6-2 – State Table for Transaction Branch Association

Transaction Branch Association States

| XA routine          | Return        | Not Associated (T0) | Associated (T1) | Association suspended (T2) |
|--------------------:|---------------|---------------------|-----------------|----------------------------|
| xa_start()          |               |                     | T1              |                            |
| xa_start(TMRESUME)  |               |                     | T1              |                            |
| xa_start(TMRESUME)  | [XA_RB]       | T0                  |                 |                            |
| xa_end(TMSUSPEND)   |               |                     |                 | T2                         |
| xa_end(TMSUSPEND)   | [XA_RB]       | T0                  |                 |                            |
| xa_end(TMSUCCESS)   |               |                     | T0              | T0                         |
| xa_end(TMFAIL)      |               |                     | T0              | T0                         |
| xa_open()           |               | T0                  | T1              | T2                         |
| xa_recover()        |               | T0                  | T1              | T2                         |
| xa_close()          |               | R0                  | R0              |                            |
| xa_*()              | [XAER_RMFAIL] | R0                  | R0              | R0                         |

---

## Table 6-3 – State Table for Transaction Branch Association (Dynamic Registration)

Transaction Branch Association States

Resource Manager calls

| XA routine   | Not registered (D0) | Registration, valid XID (D1) | Registered, suspended (D2) | Registered with NULLXID (D3) | Return      |
|-------------:|---------------------|------------------------------|----------------------------|------------------------------|-------------|
| ax_reg       |                     | D1                           |                            |                              | valid XID   |
| ax_reg       |                     |                              |                            | D3                           | NULLXID     |
| ax_reg       |                     |                              | D1                         |                              | [TM_RESUME] |
| ax_unreg     | D0                  |                              |                            |                              |             |

Transaction Manager calls

| XA routine         | Return        | Not registered (D0) | Registration, valid XID (D1) | Registered, suspended (D2) | Registered with NULLXID (D3) |
|-------------------:|---------------|---------------------|------------------------------|----------------------------|------------------------------|
| xa_end(TMSUSPEND)  |               |                     | D2                           |                            |                              |
| xa_end(TMSUSPEND)  | [XA_RB]       |                     | D0                           |                            |                              |
| xa_end(TMSUCCESS)  |               |                     | D0                           | D0                         |                              |
| xa_end(TMFAIL)     |               |                     | D0                           | D0                         |                              |
| xa_open()          |               | D0                  | D1                           | D2                         | D3                           |
| xa_recover()       |               | D0                  | D1                           | D2                         | D3                           |
| xa_close()         |               | R0                  |                              | R0                         |                              |
| xa_*()             | [XAER_RMFAIL] | R0                  | R0                           | R0                         | R0                           |

---

## Table 6-4 – State Table for Transaction Branches

Transaction Branch States

| XA routine     | Return                       | Absent (S0) | Active (S1) | Idle (S2) | Prepared (S3) | RB only (S4) | Heuristically (S5) |
|---------------:|-----------------------------:|------------:|------------:|----------:|--------------:|-------------:|--------------------|
| xa_start()     |                              | S1          |             | S1        |               |              |                    |
| xa_start()     | [XA_RB]                      |             |             | S4        |               |              |                    |
| xa_end()       |                              |             | S2          |           |               |              |                    |
| xa_end()       | [XA_RB]                      |             | S4          |           |               |              |                    |
| xa_prepare()   |                              |             |             | S3        |               |              |                    |
| xa_prepare()   | [XA_RDONLY],[XA_RB]          |             |             | S0        |               |              |                    |
| xa_prepare()   | [XAER_RMERR]                 |             |             | S2        |               |              |                    |
| xa_commit()    | [XA_OK],[XAER_RMERR]         |             |             | S0        | S0            |              |                    |
| xa_commit()    | [XA_RB]                      |             |             | S0        |               |              |                    |
| xa_commit()    | [XA_RETRY]                   |             |             |           | S3            |              |                    |
| xa_commit()    | [XA_HEUR]                    |             |             | S5        | S5            |              | S5                 |
| xa_rollback()  | [XA_OK],[XA_RB],[XAER_RMERR] |             |             | S0        | S0            | S0           |                    |
| xa_rollback()  | [XA_HEUR]                    |             |             |           | S5            | S5           | S5                 |
| xa_forget()    |                              |             |             |           |               |              | S0                 |
| xa_forget()    | [XAER_RMERR]                 |             |             |           |               |              | S5                 |
| xa_open()      |                              | S0          | S1          | S2        | S3            | S4           | S5                 |
| xa_recover()   |                              | S0          | S1          | S2        | S3            | S4           | S5                 |
| xa_close()     |                              | R0          |             | R0        | R0            | R0           | R0                 |
| xa_*()         | [XAER_RMFAIL]                | R0          | R0          | R0        | R0            | R0           | R0                 |

Notes:

- † This row also applies when an applicable RM calls ax_reg and the TM informs the RM
  that its work is on behalf of a transaction branch.
- [XA_HEUR] denotes any of [XA_HEURCOM], [XA_HEURRB], [XA_HEURMIX], or [XA_HEURHAZ].
  [XA_RB] denotes any return value with a prefix [XA_RB.

---

## Table 6-5 – State Table for Asynchronous Operations

Asynchronous Operation States

| XA routine                  | Initial call (A0) | Operation pending (A1) |
|----------------------------:|-------------------|-------------------------|
| Any synchronous xa_* call   | A0                |                         |
| xa_rollback(TMASYNC)        |                   | A1                      |
| xa_close(TMASYNC)           |                   | A1                      |
| xa_commit(TMASYNC)          |                   | A1                      |
| xa_end(TMASYNC)             |                   | A1                      |
| xa_forget(TMASYNC)          |                   | A1                      |
| xa_open(TMASYNC)            |                   | A1                      |
| xa_prepare(TMASYNC)         |                   | A1                      |
| xa_start(TMASYNC)           |                   | A1                      |
| xa_complete()               | A0                |                         |
| xa_complete(TMNOWAIT)       | A0                |                         |
| xa_complete(TMNOWAIT)       |                   | A1 (on [XA_RETRY])      |
