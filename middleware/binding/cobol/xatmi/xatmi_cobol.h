/*
 * X/Open CAE Specification
 * Distributed Transaction Processing:
 * The XATMI Specification
 * ISBN: 1-85912-130-6
 * X/Open Document Number: C506
*/

/*
 * xatmi_cobol.h
*/

#ifndef XATMI_COBOL_H
#define XATMI_COBOL_H

/*
 * Map COBOL record to C struct
 * Application specific data record
*/
struct DATA_REC_s {
  /* char    *DATA; */
  char    DATA[];
};

/*
 * Map COBOL record to C struct
 * TPSTATUS.cpy
 *
 *  05 TP-STATUS            PIC S9(9) COMP-5.
 *     88 TPOK                      VALUE 0.
 *     88 TPEBADDESC                VALUE 2.
 *     88 TPEBLOCK                  VALUE 3.
 *     88 TPEINVAL                  VALUE 4.
 *     88 TPELIMIT                  VALUE 5.
 *     88 TPENOENT                  VALUE 6.
 *     88 TPEOS                     VALUE 7.
 *     88 TPEPROTO                  VALUE 9.
 *     88 TPESVCERR                 VALUE 10.
 *     88 TPESVCFAIL                VALUE 11.
 *     88 TPESYSTEM                 VALUE 12.
 *     88 TPETIME                   VALUE 13.
 *     88 TPETRAN                   VALUE 14.
 *     88 TPEGOTSIG                 VALUE 15.
 *     88 TPEITYPE                  VALUE 17.
 *     88 TPEOTYPE                  VALUE 18.
 *     88 TPEEVENT                  VALUE 22.
 *     88 TPEMATCH                  VALUE 23.
 *  05 TPEVENT              PIC S9(9) COMP-5.
 *     88 TPEV-NOEVENT              VALUE 0.
 *     88 TPEV-DISCONIMM            VALUE 1.
 *     88 TPEV-SENDONLY             VALUE 2.
 *     88 TPEV-SVCERR               VALUE 3.
 *     88 TPEV-SVCFAIL              VALUE 4.
 *     88 TPEV-SVCSUCC              VALUE 5.
 *  05 APPL-RETURN-CODE     PIC S9(9) COMP-5.
*/
#define TPOK 0
#define TPESVCFAIL 11
struct TPSTATUS_REC_s {
  int32_t TP_STATUS;
  int32_t TPEVENT;
  int32_t APPL_RETURN_CODE;
};


/*
 * Map COBOL record to C struct
 * TPTYPE.cpy
 *
 *  05 REC-TYPE             PIC X(8).
 *     88 X-OCTET                   VALUE "X_OCTET".
 *     88 X-COMMON                  VALUE "X_COMMON".
 *  05 SUB-TYPE             PIC X(16).
 *  05 LEN                  PIC S9(9) COMP-5.
 *     88 NO-LENGTH                 VALUE 0.
 *  05 TPTYPE-STATUS        PIC S9(9) COMP-5.
 *     88 TPTYPEOK                  VALUE 0.
 *     88 TPTRUNCATE                VALUE 1.
*/
#define REC_TYPE_LEN 8
#define SUB_TYPE_LEN 16
struct TPTYPE_REC_s {
  char    REC_TYPE[REC_TYPE_LEN];
  char    SUB_TYPE[SUB_TYPE_LEN];
  int32_t LEN;
  int32_t TPTYPE_STATUS;
};


/*
 * Map COBOL record to C struct
 * TPSVCDEF.cpy
 *
 *  05 COMM-HANDLE          PIC S9(9) COMP-5.
 *  05 TPBLOCK-FLAG         PIC S9(9) COMP-5.
 *     88 TPBLOCK                   VALUE 0.
 *     88 TPNOBLOCK                 VALUE 1.
 *  05 TPTRAN-FLAG          PIC S9(9) COMP-5.
 *     88 TPTRAN                    VALUE 0.
 *     88 TPNOTRAN                  VALUE 1.
 *  05 TPREPLY-FLAG         PIC S9(9) COMP-5.
 *     88 TPREPLY                   VALUE 0.
 *     88 TPNOREPLY                 VALUE 1.
 *  05 TPTIME-FLAG          PIC S9(9) COMP-5.
 *     88 TPTIME                    VALUE 0.
 *     88 TPNOTIME                  VALUE 1.
 *  05 TPSIGRSTRT-FLAG      PIC S9(9) COMP-5.
 *     88 TPNOSIGRSTRT              VALUE 0.
 *     88 TPSIGRSTRT                VALUE 1.
 *  05 TPGETANY-FLAG        PIC S9(9) COMP-5.
 *     88 TPGETHANDLE               VALUE 0.
 *     88 TPGETANY                  VALUE 1.
 *  05 TPSENDRECV-FLAG      PIC S9(9) COMP-5.
 *     88 TPSENDONLY                VALUE 0.
 *     88 TPRECVONLY                VALUE 1.
 *  05 TPNOCHANGE-FLAG      PIC S9(9) COMP-5.
 *     88 TPCHANGE                  VALUE 0.
 *     88 TPNOCHANGE                VALUE 1.
 *  05 TPSERVICETYPE-FLAG   PIC S9(9) COMP-5.
 *     88 TPREQRSP                  VALUE IS 0.
 *     88 TPCONV                    VALUE IS 1.
 *  05 SERVICE-NAME         PIC X(15).
*/
#define TPREPLY 0
#define SERVICE_NAME_LEN 15
struct TPSVCDEF_REC_s {
  int32_t COMM_HANDLE;
  int32_t TPBLOCK_FLAG;
  int32_t TPTRAN_FLAG;
  int32_t TPREPLY_FLAG;
  int32_t TPTIME_FLAG;
  int32_t TPSIGRSTRT_FLAG;
  int32_t TPGETANY_FLAG;
  int32_t TPSENDRECV_FLAG;
  int32_t TPNOCHANGE_FLAG;
  int32_t TPSERVICETYPE_FLAG;
  char    SERVICE_NAME[SERVICE_NAME_LEN];
};


/*
 * Map COBOL record to C struct
 * TPSVCRET.cpy
 *
 *  05 TP-RETURN-VAL        PIC S9(9) COMP-5.
 *     88 TPSUCCESS                 VALUE 0.
 *     88 TPFAIL                    VALUE 1.
 *  05 APPL-CODE            PIC S9(9) COMP-5.
*/
struct TPSVCRET_REC_s {
  int32_t TP_RETURN_VAL;
  int32_t APPL_CODE;
};


extern "C" void TPACALL(struct TPSVCDEF_REC_s *TPSVCDEF_REC,
                        struct TPTYPE_REC_s *TPTYPE_REC,
                        struct DATA_REC_s *DATA_REC,
                        struct TPSTATUS_REC_s *TPSTATUS_REC);

extern "C" void TPCALL(struct TPSVCDEF_REC_s *TPSVCDEF_REC,
                       struct TPTYPE_REC_s *ITPTYPE_REC,
                       struct DATA_REC_s *IDATA_REC,
                       struct TPTYPE_REC_s *OTPTYPE_REC,
                       struct DATA_REC_s *ODATA_REC,
                       struct TPSTATUS_REC_s *TPSTATUS_REC);

extern "C" void TPCANSEL(struct TPSVCDEF_REC_s *TPSVCDEF_REC,
                         struct TPSTATUS_REC_s *TPSTATUS_REC);

extern "C" void TPGETRPLY(struct TPSVCDEF_REC_s *TPSVCDEF_REC,
                        struct TPTYPE_REC_s *TPTYPE_REC,
                        struct DATA_REC_s *DATA_REC,
                        struct TPSTATUS_REC_s *TPSTATUS_REC);

#endif /* XATMI_COBOL_H */

