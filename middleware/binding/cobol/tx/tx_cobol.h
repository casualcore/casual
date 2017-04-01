/*
 * X/Open CAE Specification
 * Distributed Transaction Processing:
 * The TX (Transaction Demarcation) Specification
 * ISBN: 1-85912-094-6
 * X/Open Document Number: C504
*/

/*
 * tx_cobol.h
*/

#ifndef TX_COBOL_H
#define TX_COBOL_H

/*
 * Map COBOL record to C struct
 * TXSTATUS.cpy
 *
 *  05 TX-STATUS            PIC S9(9) COMP-5.
*/
struct TXSTATUS_REC_s {
  int32_t TX_STATUS;
}; 


/*
 * Map COBOL record to C struct
 * TXINFDEF.cpy
 *
 *  05 XID-REC.
 *     10 FORMAT-ID     PIC S9(9) COMP-5.
 *     10 GTRID-LENGTH  PIC S9(9) COMP-5.
 *     10 BRANCH-LENGTH PIC S9(9) COMP-5.
 *     10 XID-DATA      PIC X(128). 
*/
#define XIDDATALEN 128
struct XID_REC_s {
  int32_t     FORMAT_ID;
  int32_t     GTRID_LENGTH;
  int32_t     BRANCH_LENGTH;
  char        XID_DATA[XIDDATALEN];
}; 
/*
 *  05 XID-REC. << Se struct XID_REC_s above >>
 *  05 TRANSACTION-MODE     PIC S9(9) COMP-5.
 *  05 COMMIT-RETURN        PIC S9(9) COMP-5.
 *  05 TRANSACTION-CONTROL  PIC S9(9) COMP-5.
 *  05 TRANSACTION-TIMEOUT  PIC S9(9) COMP-5.
 *  05 TRANSACTION-STATE    PIC S9(9) COMP-5.
*/
struct TXINFDEF_REC_s {
  struct XID_REC_s XID_REC;
  int32_t          TRANSACTION_MODE;
  int32_t          COMMIT_RETURN;
  int32_t          TRANSACTION_CONTROL;
  int32_t          TRANSACTION_TIMEOUT;
  int32_t          TRANSACTION_STATE;
};


extern "C" void TXBEGIN(struct TXSTATUS_REC_s *TXSTATUS_REC);
extern "C" void TXCLOSE(struct TXSTATUS_REC_s *TXSTATUS_REC);
extern "C" void TXCOMMIT(struct TXSTATUS_REC_s *TXSTATUS_REC);
extern "C" void TXINFORM(struct TXINFDEF_REC_s *TXINFDEF_REC,
                         struct TXSTATUS_REC_s *TXSTATUS_REC);
extern "C" void TXOPEN(struct TXSTATUS_REC_s *TXSTATUS_REC);
extern "C" void TXROLLBACK(struct TXSTATUS_REC_s *TXSTATUS_REC);
extern "C" void TXSETCOMMITRET(struct TXINFDEF_REC_s *TXINFDEF_REC,
                               struct TXSTATUS_REC_s *TXSTATUS_REC);
extern "C" void TXSETTIMEOUT(struct TXINFDEF_REC_s *TXINFDEF_REC,
                             struct TXSTATUS_REC_s *TXSTATUS_REC);
extern "C" void TXSETTRANCTL(struct TXINFDEF_REC_s *TXINFDEF_REC,
                             struct TXSTATUS_REC_s *TXSTATUS_REC);
#endif /* TX_COBOL_H */

