      * X/Open CAE Specification
      * Distributed Transaction Processing:
      * The TX (Transaction Demarcation) Specification
      * ISBN: 1-85912-094-6
      * X/Open Document Number: C504

      *
      * TXSTATUS.cpy
      *

       05 TX-STATUS PIC S9(9) COMP-5.
          88 TX-NOT-SUPPORTED       VALUE 1.
      *      Normal execution
          88 TX-OK                  VALUE 0.
      *      Normal execution
          88 TX-OUTSIDE             VALUE -1.
      *      Application is in an RM local transaction
          88 TX-ROLLBACK            VALUE -2.
      *      Transaction was rolled back
          88 TX-MIXED               VALUE -3.
      *      Transaction was partially committed and partially rolled back
          88 TX-HAZARD              VALUE -4.
      *      Transaction may have been partially committed and partially
      *      rolled back
          88 TX-PROTOCOL-ERROR      VALUE -5.
      *      Routine invoked in an improper context
          88 TX-ERROR               VALUE -6.
      *      Transient error
          88 TX-FAIL                VALUE -7.
      *      Fatal error
          88 TX-EINVAL              VALUE -8.
      *      Invalid arguments were given 
          88 TX-COMMITTED           VALUE -9.
      *      The transaction was heuristically committed
          88 TX-NO-BEGIN            VALUE -100.
      *      Transaction committed plus new transaction could not be started
          88 TX-ROLLBACK-NO-BEGIN   VALUE -102.
      *      Transaction rollback plus new transaction could not be started
          88 TX-MIXED-NO-BEGIN      VALUE -103.
      *      Mixed plus new transaction could not be started
          88 TX-HAZARD-NO-BEGIN     VALUE -104.
      *      Hazard plus new transaction could not be started
          88 TX-COMMITTED-NO-BEGIN  VALUE -109.
      *      Heuristically committed plus transaction could not be started

