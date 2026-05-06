# Mapping of Return Codes

Source: [X/Open CAE Specification — Distributed Transaction Processing: The TX (Transaction Demarcation) Specification](https://pubs.opengroup.org/onlinepubs/9694999599/toc.pdf), Appendix B.

---

## B.3 General Rules for Mapping of Return Codes

These rules are implicit in all of the mapping tables that follow. Section B.4 describes mappings
from XA error codes to TX error codes in the case where only one RM is involved. The case
where several RMs are involved and return different results is covered in Section B.5.

### Reporting Success

The RM return code `[XA_OK]` maps directly to the TM return code `[TX_OK]`, and is not
included in the tables. However, the notion of success depends on the context of the call:

- If an AP requests `tx_rollback()` and all RMs roll back their work, `tx_rollback()` reports
  success despite any RM failures or heuristic decisions that may have occurred.
- Conversely, if an AP requests `tx_commit()` and the TM determines that it must call
  `xa_rollback()` at some RMs, then despite the fact that these rollbacks succeeded, the TM must
  still return `[TX_ROLLBACK]` to notify the AP that the requested commitment failed.
- The `tx_commit()` and `tx_rollback()` calls typically cause the TM to issue a series of XA
  calls. Success at any one of these calls does not constitute success of the TX call.

### Origin of Error

- `[TX_ERROR]` — reports that an RM failed temporarily. The exact nature of the error may be reported in an RM-specific manner.
- `[TX_FAIL]` — reports that the TM or an RM failed and the TM should not be called further.

A TM should return `[TX_FAIL]` only when it can no longer perform work on behalf of the AP,
otherwise it should return `[TX_ERROR]`.

### Heuristic Outcomes

Heuristic outcomes that match the outcome the AP requested (`XA_HEURCOM` during
`tx_commit()`, and `XA_HEURRB` during `tx_rollback()`) are **not** reported to the AP as
heuristic outcomes.

### Invalid Arguments

`[XAER_INVAL]` from an RM reflects a synchronisation failure between RM and TM (never an
application coding error), because there is no TX routine where the AP gives the TM an argument
to pass directly to the RM.

### Protocol Violations

`[XAER_PROTO]` reflects a synchronisation failure between an RM and a TM. When an
improperly-coded AP causes a protocol error, the TM returns `[TX_PROTOCOL_ERROR]`.

### Too Many Asynchronous Operations

`[XAER_ASYNC]` informs the TM that it has exceeded the RM's limit for outstanding asynchronous
requests. The TM adapts in a product-specific manner. `[XAER_ASYNC]` is **not** included in
the following tables.

---

## B.4 Suggested Mapping of Return Codes: Single RM

This section suggests a mapping of the return codes for a single RM where the TM employs a
one-phase commit optimisation.

### Return Codes from `xa_open()`

The TM opens the RM when the AP calls `tx_open()`.

| `xa_open()`      | → | `tx_open()`   |
|------------------|---|---------------|
| `[XAER_RMERR]`  | → | `[TX_ERROR]`   |
| `[XAER_INVAL]`  | → | `[TX_FAIL]`    |
| `[XAER_PROTO]`  | → | `[TX_FAIL]`    |

### Return Codes from `xa_close()`

The TM closes the RM when the AP calls `tx_close()`.

| `xa_close()`     | → | `tx_close()`  |
|------------------|---|---------------|
| `[XAER_RMERR]`  | → | `[TX_ERROR]`   |
| `[XAER_INVAL]`  | → | `[TX_FAIL]`    |
| `[XAER_PROTO]`  | → | `[TX_FAIL]`    |

### Return Codes from `xa_start()`

The TM starts a new global transaction at the RM when the AP calls `tx_begin()`, or when the AP
calls `tx_commit()` or `tx_rollback()` and the `transaction_control` characteristic is set to
`TX_CHAINED`.

| `xa_start()`      | → | `tx_begin()`         | `tx_commit()`             | `tx_rollback()`            |
|-------------------|---|----------------------|---------------------------|----------------------------|
| `[XA_RETRY]`     | → | [1]                  | [1]                        | [1]                        |
| `[XA_RB*]`       | → | [2]                  | [2]                        | [2]                        |
| `[XAER_NOTA]`    | → | [2]                  | [2]                        | [2]                        |
| `[XAER_RMERR]`   | → | `[TX_ERROR]`         | `[TX_*_NO_BEGIN]` [4]      | `[TX_*_NO_BEGIN]` [5]      |
| `[XAER_DUPID]`   | → | `[TX_ERROR]` [3]     | `[TX_*_NO_BEGIN]` [3,4]    | `[TX_*_NO_BEGIN]` [3,5]    |
| `[XAER_INVAL]`   | → | `[TX_FAIL]`          | `[TX_*_NO_BEGIN]` [4]      | `[TX_*_NO_BEGIN]` [5]      |
| `[XAER_PROTO]`   | → | `[TX_ERROR]` [3]     | `[TX_*_NO_BEGIN]` [3,4]    | `[TX_*_NO_BEGIN]` [3,5]    |
| `[XAER_RMFAIL]`  | → | `[TX_FAIL]`          | `[TX_*_NO_BEGIN]` [4]      | `[TX_*_NO_BEGIN]` [5]      |
| `[XAER_OUTSIDE]` | → | `[TX_OUTSIDE]` [6]   | [6]                        | [6]                        |

**Notes:**

1. `[XA_RETRY]` tells the TM to reissue the call. The result depends on the ultimate outcome. If the RM keeps returning `[XA_RETRY]`, the TM may return `[TX_ERROR]`.
2. These return codes indicate the global transaction has been marked rollback-only. An RM would never return them when the TM calls `xa_start()` to start a *new* global transaction — only when resuming or joining an existing transaction the RM has marked rollback-only. Not applicable for new transactions.
3. Receipt of `[XAER_DUPID]` or `[XAER_PROTO]` is a strong indication of a failure between the TM and RM. The TM may try to resynchronise by issuing `xa_end()` and `xa_rollback()`. If unsuccessful, `[TX_FAIL]` may be returned. See *Origin of Error*.
4. May be any one of `[TX_NO_BEGIN]`, `[TX_ROLLBACK_NO_BEGIN]`.
5. May be any one of `[TX_NO_BEGIN]`, `[TX_COMMITTED_NO_BEGIN]`.
6. Can only occur if the AP is participating in a local transaction. Since an AP cannot be within both a global and a local transaction simultaneously, this situation is not applicable here.

### Return Codes from `xa_end()`

Return codes from `xa_end()` when the TM calls it during `tx_commit()` or `tx_rollback()`.

| `xa_end()`       | → | `tx_commit()` or `tx_rollback()` |
|------------------|---|----------------------------------|
| `[XA_NOMIGRATE]` | → | [1]                              |
| `[XA_RB*]`       | → | [2]                              |
| `[XAER_NOTA]`    | → | [2]                              |
| `[XAER_RMERR]`   | → | [2]                              |
| `[XAER_RMFAIL]`  | → | `[TX_FAIL]`                      |
| `[XAER_INVAL]`   | → | `[TX_FAIL]`                      |
| `[XAER_PROTO]`   | → | `[TX_FAIL]` [3]                  |

**Notes:**

1. `[XA_NOMIGRATE]` is only returned when the TM uses `xa_end(TMSUSPEND)`, which is not applicable here.
2. These return codes indicate the RM has requested the TM to roll back all work on behalf of the global transaction. If the AP called `tx_commit()`, it receives `[TX_ROLLBACK]`; if `tx_rollback()`, it receives `[TX_OK]`.
3. Receipt of `[XAER_PROTO]` is a strong indication of a failure between the TM and RM. The TM may try to resynchronise by issuing `xa_rollback()`. If unsuccessful, `[TX_FAIL]` may be returned. See *Origin of Error*.

### Return Codes from `xa_commit()` and `xa_rollback()`

Return codes from `xa_commit()` or `xa_rollback()` when called by the TM during `tx_commit()` or `tx_rollback()`.

| `xa_commit()` or `xa_rollback()` | → | `tx_commit()` or `tx_rollback()` |
|----------------------------------|---|----------------------------------|
| `[XA_HEURCOM]`                   | → | [1]                              |
| `[XA_HEURRB]`                    | → | [1]                              |
| `[XA_HEURHAZ]`                   | → | `[TX_HAZARD]`                    |
| `[XA_HEURMIX]`                   | → | `[TX_MIXED]`                     |
| `[XA_RB*]`                       | → | [2]                              |
| `[XAER_RMERR]`                   | → | [2]                              |
| `[XAER_NOTA]`                    | → | [2]                              |
| `[XAER_RMFAIL]`                  | → | `[TX_FAIL]`                      |
| `[XAER_INVAL]`                   | → | `[TX_FAIL]`                      |
| `[XAER_PROTO]`                   | → | `[TX_FAIL]`                      |
| `[XA_RETRY]`                     | → | [3]                              |

**Notes:**

1. If a heuristic outcome matches the outcome the AP requested, the TM reports success (`[TX_OK]`) to the AP. If the outcome is not what the AP requested, the TM reports that disposition: `tx_commit()` returns `[TX_ROLLBACK]`; `tx_rollback()` returns `[TX_COMMITTED]`.
2. These return codes indicate the RM rolled back work done on behalf of the global transaction. If the AP called `tx_commit()`, it receives `[TX_ROLLBACK]`; if `tx_rollback()`, it receives `[TX_OK]`.
3. `[XA_RETRY]` tells the TM to reissue the call. The result depends on the ultimate outcome. If the RM keeps returning `[XA_RETRY]`, the TM may return `[TX_ERROR]`.

---

## B.5 Suggested Mapping of Return Codes: Multiple RMs

The TM considers return status from all associated RMs to generate a return code for the AP. In
general, the TM's return code reflects the **most severe** error an RM reported.

### Suggested Error Severity Hierarchy

| Severity  | TX Return Code                                                                       |
|-----------|--------------------------------------------------------------------------------------|
| Highest   | `[TX_FAIL]` — TM or RM failure                                                       |
|           | `[TX_MIXED]` — Mixed heuristic outcome                                               |
|           | `[TX_HAZARD]` — Heuristic hazard                                                     |
|           | `[TX_ERROR]` — Possibly-recoverable RM failure                                       |
| Lowest    | `[TX_OUTSIDE]`, `[TX_ROLLBACK]`, `[TX_COMMITTED]` — Warnings; `[TX_OK]` — Success    |

> `[TX_EINVAL]` and `[TX_PROTOCOL_ERROR]` do not appear above because they involve only
> the AP–TM interface; the TM generates these without calling any RMs.

### Return Codes from `xa_open()`

The TM opens all RMs when the AP calls `tx_open()`.

| `xa_open()`     | → | `tx_open()`       |
|-----------------|---|-------------------|
| `[XAER_RMERR]`  | → | `[TX_ERROR]` [1]  |
| `[XAER_INVAL]`  | → | `[TX_FAIL]` [1]   |
| `[XAER_PROTO]`  | → | `[TX_FAIL]` [1]   |

**Notes:**

1. The TM is free to return `[TX_OK]` in the case where one or more of the RMs returns `[XA_OK]`.

### Return Codes from `xa_prepare()`

The TM prepares the global transaction at the RM when the AP calls `tx_commit()`.

| `xa_prepare()`  | → | `tx_commit()`        |
|-----------------|---|----------------------|
| `[XA_RDONLY]`   | → | `[TX_OK]` [1]        |
| `[XA_RB*]`      | → | `[TX_ROLLBACK]` [2]  |
| `[XAER_NOTA]`   | → | `[TX_ROLLBACK]` [2]  |
| `[XAER_RMERR]`  | → | `[TX_ROLLBACK]` [3]  |
| `[XAER_RMFAIL]` | → | `[TX_FAIL]`          |
| `[XAER_INVAL]`  | → | `[TX_FAIL]`          |
| `[XAER_PROTO]`  | → | `[TX_ROLLBACK]` [3]  |

**Notes:**

1. `[XA_RDONLY]` ends that RM's participation in the commitment protocol. If all RMs return `[XA_RDONLY]`, the TM returns `[TX_OK]`. Otherwise, the TM returns a code based on the full commitment protocol at other RMs.
2. These return codes indicate the RM rolled back work done on behalf of the global transaction.
3. `[XAER_RMERR]` or `[XAER_PROTO]` from `xa_prepare()` makes no assertion about whether the RM successfully prepared its work. The TM should call all RMs with `xa_rollback()` to attempt to roll back the transaction. If unsuccessful, the TM could return `[TX_HAZARD]` or `[TX_FAIL]`.

### Combined Outcomes During Commit — `xa_commit()`

When two or more RMs report different outcomes during `tx_commit()`, the TM maps the XA result from each RM to a potential TX result using this table, then returns the most severe TX result to the AP.

| RM1              | RM2             | Result to AP    |
|------------------|-----------------|------------------|
| `[XAER_RMFAIL]`  | any             | `[TX_FAIL]`      |
| `[XAER_INVAL]`   | any             | `[TX_FAIL]`      |
| `[XAER_PROTO]`   | any             | `[TX_FAIL]` [2]  |
| `[XAER_NOTA]`    | any             | `[TX_FAIL]` [2]  |
| `[XA_HEURMIX]`   | any             | `[TX_MIXED]`     |
| `[XA_HEURHAZ]`   | any             | `[TX_HAZARD]`    |
| `[XA_RETRY]`     | any             | [1]              |
| any commit †     | any commit †    | `[TX_OK]`        |
| any commit †     | any rollback ‡  | `[TX_MIXED]`     |
| any rollback ‡   | any rollback ‡  | `[TX_ROLLBACK]`  |

† Commitment indications: `[XA_HEURCOM]`, and `[XA_OK]` from `xa_commit()`.  
‡ Rollback indications: `[XA_HEURRB]` and `[XAER_RMERR]`.

**Notes:**

1. `[XA_RETRY]` tells the TM to reissue the call. The result depends on the ultimate outcome. If the RM keeps returning `[XA_RETRY]`, the TM may return `[TX_HAZARD]`, or assume it cannot use the RM and return `[TX_ERROR]` or `[TX_FAIL]`.
2. `[XAER_PROTO]` or `[XAER_NOTA]` returned on `xa_commit()` indicates a serious synchronisation failure between the TM and the RM.

> If `transaction_control` is set to `TX_CHAINED`, `xa_start()` may fail after `xa_commit()` completes.
> In that case `[TX_NO_BEGIN]`, `[TX_ROLLBACK_NO_BEGIN]`, `[TX_MIXED_NO_BEGIN]`, or
> `[TX_HAZARD_NO_BEGIN]` is returned, corresponding respectively to what would have been
> `[TX_OK]`, `[TX_ROLLBACK]`, `[TX_MIXED]`, and `[TX_HAZARD]`.

### Combined Outcomes During Rollback — `xa_rollback()`

When two or more RMs report different outcomes during the second phase of a two-phase commit or during `tx_rollback()`:

| RM1             | RM2             | Result to AP     |
|-----------------|-----------------|------------------|
| `[XAER_RMFAIL]` | any             | `[TX_FAIL]`      |
| `[XAER_INVAL]`  | any             | `[TX_FAIL]`      |
| `[XAER_PROTO]`  | any             | `[TX_FAIL]`      |
| `[XA_HEURMIX]`  | any             | `[TX_MIXED]`     |
| `[XA_HEURHAZ]`  | any             | `[TX_HAZARD]`    |
| `[XA_RETRY]`    | any             | [1]              |
| `[XA_HEURCOM]`  | `[XA_HEURCOM]`  | [3]              |
| `[XA_HEURCOM]`  | any rollback †  | `[TX_MIXED]` [2] |
| any rollback †  | any rollback †  | [4]              |

† Rollback indications: `[XA_HEURRB]`, `[XAER_NOTA]`, `[XAER_RMERR]`, and `[XAER_OK]`.

**Notes:**

1. `[XA_RETRY]` tells the TM to reissue the call. The result depends on the ultimate outcome. If the RM keeps returning `[XA_RETRY]`, the TM may return `[TX_HAZARD]`, or assume it cannot use the RM and return `[TX_ERROR]` or `[TX_FAIL]`.
2. If `[XAER_NOTA]` is returned on `xa_rollback()` following a successful `xa_prepare()`, this indicates a serious synchronisation failure between the TM and the RM. The TM should return `[TX_FAIL]`.
3. The RM heuristically committed the work on behalf of the global transaction. If the AP called `tx_commit()`, it receives `[TX_OK]`; if `tx_rollback()`, it receives `[TX_COMMITTED]`.
4. If the AP called `tx_commit()`, it receives `[TX_ROLLBACK]`; if `tx_rollback()`, it receives `[TX_OK]`.

> If `transaction_control` is set to `TX_CHAINED`, `xa_start()` may fail after `xa_rollback()` completes.
> In that case `[TX_NO_BEGIN]`, `[TX_COMMITTED_NO_BEGIN]`, `[TX_MIXED_NO_BEGIN]`, or
> `[TX_HAZARD_NO_BEGIN]` is returned, corresponding respectively to what would have been
> `[TX_OK]`, `[TX_COMMITTED]`, `[TX_MIXED]`, and `[TX_HAZARD]`.
