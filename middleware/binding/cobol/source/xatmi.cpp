/*
 * X/Open CAE Specification
 * Distributed Transaction Processing:
 * The XATMI Specification
 * ISBN: 1-85912-130-6
 * X/Open Document Number: C506
*/

/*
 * xatmi_cobol.c
*/

#include "cobol/xatmi.h"
#include "casual/xatmi.h"
#include "casual/xatmi/cobol.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>


/*
 * Internal support functions
*/
void cobstr_to_cstr(char *, const char *, int);
void cstr_to_cobstr(char *dest, const char *src, int len);


extern "C" void TPACALL(struct TPSVCDEF_REC_s *TPSVCDEF_REC,
                        struct TPTYPE_REC_s *TPTYPE_REC,
                        const char *DATA_REC,
                        struct TPSTATUS_REC_s *TPSTATUS_REC) {

   char service_name[SERVICE_NAME_LEN +1];
   char rec_type[REC_TYPE_LEN + 1];
   char sub_type[SUB_TYPE_LEN + 1];
   char *data_rec;
   int rv;
   long flags;

   /* Copy COBOL string to C string and null terminate C string */
   cobstr_to_cstr(service_name, TPSVCDEF_REC->SERVICE_NAME, SERVICE_NAME_LEN);
   cobstr_to_cstr(rec_type, TPTYPE_REC->REC_TYPE, REC_TYPE_LEN);
   cobstr_to_cstr(sub_type, TPTYPE_REC->SUB_TYPE, SUB_TYPE_LEN);

   /* Allocate typed buffers */
   if ((data_rec = (char *)tpalloc(rec_type,
                                   sub_type,
                                   TPTYPE_REC->LEN)) == nullptr) {
      TPSTATUS_REC->TP_STATUS = (int32_t)tperrno;
      /* printf("error TPACALL --> tpalloc: %d\n", tperrno); */
      return;
   }

   /* Copy data from COBOL record to buffer allocated with tpalloc */
   memcpy(data_rec, DATA_REC, TPTYPE_REC->LEN);

   /* Convert COBOL flags (record) to C flags (bit map) */
   flags = 0;
   if (TPSVCDEF_REC->TPTRAN_FLAG)
      flags = flags | TPNOTRAN;
   if (TPSVCDEF_REC->TPREPLY_FLAG)
      flags = flags | TPNOREPLY;
   if (TPSVCDEF_REC->TPBLOCK_FLAG)
      flags = flags | TPNOBLOCK;
   if (TPSVCDEF_REC->TPTIME_FLAG)
      flags = flags | TPNOTIME;
   if (TPSVCDEF_REC->TPSIGRSTRT_FLAG)
      flags = flags | TPSIGRSTRT;

   if ((rv = tpacall(service_name,
                     data_rec,
                     TPTYPE_REC->LEN,
                     flags)) == -1) {
      TPSTATUS_REC->TP_STATUS = (int32_t)tperrno;
      /* printf("error TPACALL --> tpacall: %d\n", tperrno); */
   } else {
      TPSTATUS_REC->TP_STATUS = (int32_t)TPOK;
      if (TPSVCDEF_REC->TPREPLY_FLAG ==  TPREPLY)
         TPSVCDEF_REC->COMM_HANDLE = (int32_t)rv;
   }

   tpfree(data_rec);
   return;
}


#if 0
TPADVERTISE
Only used by server/service
/*
 * extern "C" void TPADVERTISE(...
 *
 * Not possible to implement this api without additional support from C-api. COBOL
 * and C api arguments are incompatible.
 *
 * COBOL api declare a string holding name of program as executable for service.
 * C api declare a pointer to a function as executable for service.
 *
 * ! Possible solution could be to use a combination of COBOL subroutines and
 * C functions.
 * A possible approach is to use dlopen and dlsym routines. Reasonable COBOL
 * implementations expose entry points as * "global symbols" that can be found
 * by dlsym. Filenames may need to be constructed from symbol. Otherwise the
 * responsibility for making sure relevant symbols/libraries are present (has
 * been loaded) can be assigned to the caller of TPADVERTISE.
 * Initial tests using dlopen/dlysym to call procedures in cobol from C-wrapper
 * were successful. Tested against Cobol shared library compiled with microfocus cobol.
*/
#endif /* #if 0 */


extern "C" void TPCALL(struct TPSVCDEF_REC_s *TPSVCDEF_REC,
                       struct TPTYPE_REC_s *ITPTYPE_REC,
                       const char* IDATA_REC,
                       struct TPTYPE_REC_s *OTPTYPE_REC,
                       char*  ODATA_REC,
                       struct TPSTATUS_REC_s *TPSTATUS_REC) {

   char service_name[SERVICE_NAME_LEN + 1];
   char rec_type[REC_TYPE_LEN + 1];
   char sub_type[SUB_TYPE_LEN + 1];
   char *idata_rec;
   char *odata_rec;
   long olen;
   int rv;
   long flags;

   /* Copy COBOL string to C string and null terminate C string */
   cobstr_to_cstr(service_name, TPSVCDEF_REC->SERVICE_NAME, SERVICE_NAME_LEN);
   cobstr_to_cstr(rec_type, ITPTYPE_REC->REC_TYPE, REC_TYPE_LEN);
   cobstr_to_cstr(sub_type, ITPTYPE_REC->SUB_TYPE, SUB_TYPE_LEN);

   /* Allocate typed buffers */
   if ((idata_rec = (char *)tpalloc(rec_type,
                                    sub_type,
                                    ITPTYPE_REC->LEN)) == nullptr) {
      TPSTATUS_REC->TP_STATUS = (int32_t)tperrno;
      /* printf("error TPCALL --> idata_rec = tpalloc: %d\n", tperrno); */
      return;
   }
   if ((odata_rec = (char *)tpalloc(rec_type,
                                    sub_type,
                                    OTPTYPE_REC->LEN)) == nullptr) {
      TPSTATUS_REC->TP_STATUS = (int32_t)tperrno;
      /* printf("error TPCALL --> odata_rec = tpalloc: %d\n", tperrno); */
      tpfree(idata_rec);
      return;
   }

   /* Copy data from COBOL record to buffer allocated with tpalloc */
   memcpy(idata_rec, IDATA_REC, ITPTYPE_REC->LEN);

   /* Convert COBOL flags (record) to C flags (bit map) */
   flags = 0;
   if (TPSVCDEF_REC->TPTRAN_FLAG)
      flags = flags | TPNOTRAN;
   if (TPSVCDEF_REC->TPNOCHANGE_FLAG)
      flags = flags | TPNOCHANGE;
   if (TPSVCDEF_REC->TPBLOCK_FLAG)
      flags = flags | TPNOBLOCK;
   if (TPSVCDEF_REC->TPTIME_FLAG)
      flags = flags | TPNOTIME;
   if (TPSVCDEF_REC->TPSIGRSTRT_FLAG)
      flags = flags | TPSIGRSTRT;

   if ((rv = tpcall(service_name,
                    idata_rec,
                    ITPTYPE_REC->LEN,
                    &odata_rec,
                    &olen,
                    flags)) == -1) {
      TPSTATUS_REC->TP_STATUS = (int32_t)tperrno;
      /* printf("error TPCALL --> tpcall: %d\n", tperrno); */
      tpfree(odata_rec);
      tpfree(idata_rec);
      return;
   } else {
      TPSTATUS_REC->TP_STATUS = (int32_t)TPOK;
      TPSTATUS_REC->APPL_RETURN_CODE = (int32_t)tpurcode;
   }

   /* Copy data from buffer allocated with tpalloc to COBOL record */
   if (olen < OTPTYPE_REC->LEN)
      OTPTYPE_REC->LEN = (int32_t)olen;
   memcpy(ODATA_REC, odata_rec, OTPTYPE_REC->LEN);

   /* Error code in buffer descriptor if data does not fit in callers buffer? */
   if (OTPTYPE_REC->LEN < olen ) {
     OTPTYPE_REC->TPTYPE_STATUS = (int32_t)TPTRUNCATE;
   }
   else {
     OTPTYPE_REC->TPTYPE_STATUS = (int32_t)TPTYPEOK;
   }

   tpfree(odata_rec);
   tpfree(idata_rec);

   return;
}

/* not tested */
extern "C" void TPCANCEL(struct TPSVCDEF_REC_s *TPSVCDEF_REC,
                         struct TPSTATUS_REC_s *TPSTATUS_REC) {

   int rv;

   if ((rv = tpcancel(TPSVCDEF_REC->COMM_HANDLE)) == -1) {
      TPSTATUS_REC->TP_STATUS = (int32_t)tperrno;
      /* printf("error TPCANCEL --> tpcancel: %d\n", tperrno); */
   } else {
      TPSTATUS_REC->TP_STATUS = (int32_t)TPOK;
   }

   return;
}

/* not tested */
extern "C" void TPCONNECT(struct TPSVCDEF_REC_s *TPSVCDEF_REC,
                          struct TPTYPE_REC_s *TPTYPE_REC,
                          char *DATA_REC,
                          struct TPSTATUS_REC_s *TPSTATUS_REC) {

   char service_name[SERVICE_NAME_LEN +1];
   char rec_type[REC_TYPE_LEN + 1];
   char sub_type[SUB_TYPE_LEN + 1];
   char *data_rec;
   int rv;
   long flags;

   /* Copy COBOL string to C string and null terminate C string */
   cobstr_to_cstr(service_name, TPSVCDEF_REC->SERVICE_NAME, SERVICE_NAME_LEN);
   cobstr_to_cstr(rec_type, TPTYPE_REC->REC_TYPE, REC_TYPE_LEN);
   cobstr_to_cstr(sub_type, TPTYPE_REC->SUB_TYPE, SUB_TYPE_LEN);
   // Spaces in TPTYPE_REC means no buffer, becomes zero length string
   if (*rec_type != '\000') {
      // Does Casual have support for byffer types with
      // implicit length? Current code only support explicit length.
      /* Allocate typed buffers */
      if ((data_rec = (char *)tpalloc(rec_type,
                                    sub_type,
                                    TPTYPE_REC->LEN)) == nullptr) {
         TPSTATUS_REC->TP_STATUS = (int32_t)tperrno;
         /* printf("error TPCONNECT --> tpalloc: %d\n", tperrno); */
         return;
      }

      /* Copy data from COBOL record to buffer allocated with tpalloc */
      memcpy(data_rec, DATA_REC, TPTYPE_REC->LEN);
   } else {
      // no user data with connect!
      data_rec = NULL;
   }
   /* Convert COBOL flags (record) to C flags (bit map) */
   flags = 0;
   if (TPSVCDEF_REC->TPTRAN_FLAG == 1)
      flags = flags | TPNOTRAN;
   // Do NOT set TPTRAN flag when/if 88-variable TPTRAN is true
   // (TPSVCDEF_REC->TPTRAN_FLAG == 0, or anything !=1).
   // TPTRAN in C-api is unrelated to TPTRAN in TPCONNECT Cobol api.
   // It is not legal to set TPTRAN in tpconnect() flags.
   if (TPSVCDEF_REC->TPSENDRECV_FLAG)
      flags = flags | TPRECVONLY;
   else
      flags = flags | TPSENDONLY;
   if (TPSVCDEF_REC->TPBLOCK_FLAG)
      flags = flags | TPNOBLOCK;
   if (TPSVCDEF_REC->TPTIME_FLAG)
      flags = flags | TPNOTIME;
   if (TPSVCDEF_REC->TPSIGRSTRT_FLAG)
      flags = flags | TPSIGRSTRT;

   if ((rv = tpconnect(service_name,
                     data_rec,
                     TPTYPE_REC->LEN,
                     flags)) == -1) {
      TPSTATUS_REC->TP_STATUS = (int32_t)tperrno;
      /* printf("error TPCONNECT --> tpconnect: %d\n", tperrno); */
   } else {
      TPSTATUS_REC->TP_STATUS = (int32_t)TPOK;
      TPSVCDEF_REC->COMM_HANDLE = (int32_t)rv;
   }

   tpfree(data_rec);
   return;
}

/*not tested */
extern "C" void TPDISCON(struct TPSVCDEF_REC_s *TPSVCDEF_REC,
                         struct TPSTATUS_REC_s *TPSTATUS_REC) {

   int rv;

   if ((rv = tpdiscon(TPSVCDEF_REC->COMM_HANDLE)) == -1) {
      TPSTATUS_REC->TP_STATUS = (int32_t)tperrno;
      /* printf("error TPDISCON --> tpdiscon: %d\n", tperrno); */
   } else {
      TPSTATUS_REC->TP_STATUS = (int32_t)TPOK;
   }

   return;
}



extern "C" void TPGETRPLY(struct TPSVCDEF_REC_s *TPSVCDEF_REC,
                        struct TPTYPE_REC_s *TPTYPE_REC,
         //struct DATA_REC_s *DATA_REC,
                        char *DATA_REC,
                        struct TPSTATUS_REC_s *TPSTATUS_REC) {

   char service_name[SERVICE_NAME_LEN +1];
   char rec_type[REC_TYPE_LEN + 1];
   char sub_type[SUB_TYPE_LEN + 1];
   char *data_rec;
   int rv;
   long flags;
   long len;

   /* Copy COBOL string to C string and null terminate C string */
   cobstr_to_cstr(service_name, TPSVCDEF_REC->SERVICE_NAME, SERVICE_NAME_LEN);
   cobstr_to_cstr(rec_type, TPTYPE_REC->REC_TYPE, REC_TYPE_LEN);
   cobstr_to_cstr(sub_type, TPTYPE_REC->SUB_TYPE, SUB_TYPE_LEN);

   /* Allocate typed buffers */
   if ((data_rec = (char *)tpalloc(rec_type,
                                   sub_type,
                                   TPTYPE_REC->LEN)) == nullptr) {
      TPSTATUS_REC->TP_STATUS = (int32_t)tperrno;
      /* printf("error TPGETRPLY --> tpalloc: %d\n", tperrno); */
      return;
   }

   /* Convert COBOL flags (record) to C flags (bit map) */
   flags = 0;
   if (TPSVCDEF_REC->TPGETANY_FLAG)
      flags = flags | TPGETANY;
   if (TPSVCDEF_REC->TPNOCHANGE_FLAG)
      flags = flags | TPNOCHANGE;
   if (TPSVCDEF_REC->TPBLOCK_FLAG)
      flags = flags | TPNOBLOCK;
   if (TPSVCDEF_REC->TPTIME_FLAG)
      flags = flags | TPNOTIME;
   if (TPSVCDEF_REC->TPSIGRSTRT_FLAG)
      flags = flags | TPSIGRSTRT;

   if ((rv = tpgetrply(&TPSVCDEF_REC->COMM_HANDLE,
                       &data_rec,
                       &len,
                       flags)) == -1) {
      TPSTATUS_REC->TP_STATUS = (int32_t)tperrno;
      /* printf("error TPGETRPLY --> tpgetrply: %d\n", tperrno); */
   } else {
      TPSTATUS_REC->TP_STATUS = (int32_t)TPOK;
      TPSTATUS_REC->APPL_RETURN_CODE = (int32_t)tpurcode;
   }

   /* Copy data from buffer allocated with tpalloc to COBOL record */
   /* shall the rest of the callers buffer be filled with space,   */
   /* when the reply is smaller than the buffer? This is a COBOLish*/
   /* way of copying data from a short to longer variable.         */
   /* If the data does not fit in the callers buffer, what shall   */
   /* be done? Copy as much as possible and set a status return    */
   /* (TPTRUNCATE?) is a possibility.                              */
   /* Note that the buffer "descriptor" in TPTYPE_REC has a status */
   /* field                                                        */
   if (len < TPTYPE_REC->LEN)
      TPTYPE_REC->LEN = (int32_t)len;
   memcpy(DATA_REC, data_rec, TPTYPE_REC->LEN);

   if (TPTYPE_REC->LEN < len ) {
     TPTYPE_REC->TPTYPE_STATUS = (int32_t)TPTRUNCATE;
   }
   else {
     TPTYPE_REC->TPTYPE_STATUS = (int32_t)TPTYPEOK;
   }

   tpfree(data_rec);
   return;
}

/* Not tested! */
extern "C" void TPRECV(struct TPSVCDEF_REC_s *TPSVCDEF_REC,
                       struct TPTYPE_REC_s *TPTYPE_REC,
                       char *DATA_REC,
                       struct TPSTATUS_REC_s *TPSTATUS_REC) {

   char rec_type[REC_TYPE_LEN + 1];
   char sub_type[SUB_TYPE_LEN + 1];
   char *data_rec;
   int rv;
   long flags;
   long len;
   long revent;

   /* Copy COBOL string to C string and null terminate C string */
   //   cobstr_to_cstr(service_name, TPSVCDEF_REC->SERVICE_NAME, SERVICE_NAME_LEN);
   cobstr_to_cstr(rec_type, TPTYPE_REC->REC_TYPE, REC_TYPE_LEN);
   cobstr_to_cstr(sub_type, TPTYPE_REC->SUB_TYPE, SUB_TYPE_LEN);

   /* Allocate typed buffers */
   if ((data_rec = (char *)tpalloc(rec_type,
                                   sub_type,
                                   TPTYPE_REC->LEN)) == nullptr) {
      TPSTATUS_REC->TP_STATUS = (int32_t)tperrno;
      /* printf("error TPGETRPLY --> tpalloc: %d\n", tperrno); */
      return;
   }
   len=TPTYPE_REC->LEN;

   /* Convert C flags (bit map) to COBOL flags (record)  */
   flags = 0;
   if (TPSVCDEF_REC->TPNOCHANGE_FLAG)
      flags = flags | TPNOCHANGE;
   if (TPSVCDEF_REC->TPBLOCK_FLAG)
      flags = flags | TPNOBLOCK;
   if (TPSVCDEF_REC->TPTIME_FLAG)
      flags = flags | TPNOTIME;
   if (TPSVCDEF_REC->TPSIGRSTRT_FLAG)
      flags = flags | TPSIGRSTRT;

   if ((rv = tprecv(TPSVCDEF_REC->COMM_HANDLE,
                    &data_rec,
                    &len,
                    flags,
          &revent)) == -1) {
      TPSTATUS_REC->TP_STATUS = (int32_t)tperrno;
      /* printf("error TPRECV --> tprecv: %d\n", tperrno); */
      // tpurcode has an application return status for events
      // TPEV_SVCSUCC and TPEV_SVCFAIL
      if (tperrno == TPEEVENT) {
         TPSTATUS_REC->TP_STATUS = (int32_t)TPEEVENT;
         // Cobol event codes do not match those in the C interface.
         // Need to map them.
         switch (revent) {
         case TPEV_DISCONIMM:
            TPSTATUS_REC->TPEVENT = 1;
            break;
         case TPEV_SENDONLY:
            TPSTATUS_REC->TPEVENT = 2;
            break;
         case TPEV_SVCERR:
            TPSTATUS_REC->TPEVENT = 3;
            break;
         case TPEV_SVCFAIL:
            TPSTATUS_REC->TPEVENT = 4;
            break;
         case TPEV_SVCSUCC:
            TPSTATUS_REC->TPEVENT = 5;
            break;
         }
         if (revent == TPEV_SVCSUCC ||
             revent == TPEV_SVCFAIL) {
                TPSTATUS_REC->APPL_RETURN_CODE=(int32_t)tpurcode;
             }
      }
   } else {
      TPSTATUS_REC->TP_STATUS = (int32_t)TPOK;
      // Set TPEVENT to 0 oe leave it unchanged? (Copy has
      // 88 TPEV-NOEVENT VALUE 0)
      TPSTATUS_REC->TPEVENT = 0;
      if (tperrno == TPEEVENT) {
         // Should not happen, events give rv == -1
         printf("error TPRECV: tperrno == TPEEVENT but rv != -1, rv=%d", rv);
         TPSTATUS_REC->TP_STATUS=(int32_t)TPEEVENT;
         TPSTATUS_REC->TPEVENT = (int32_t)revent;
      }
   }

   /* Copy data from buffer allocated with tpalloc to COBOL record */
   /* shall the rest of the callers buffer be filled with space,   */
   /* when the reply is smaller than the buffer? this is a COBOLish*/
   /* way of copying data from a short to longer variable.         */
   /* If the data does not fit in the callers buffer, what shall   */
   /* be done? Copy as much as possible and set a status return    */
   /* (TPTRUNCATE?) is a possibility.                              */
   /* Note that the buffer "descriptor" in TPTYPE_REC has a status */
   /* field. It is used to signal a possible truncation            */
   //
   // To investigate: Data can be returned with events
   // TPEV_SVCSUCC, TPEV_SVCFAIL and TPEV_SENDONLY, but not with
   // other events (TPEV_DISCONIMM, TPEV_SVCERR). It is OK
   // set TPTYPE_REC->TPTYPE_STATUS to TPTYPEOK for events that
   // can't carry data? And to modify TPTYPE_REC->LEN? Or should they
   // be left unchanged?
   // Code currently sets them, and assumes that len for events
   // that can not carry data has been set to 0.
   if (len < TPTYPE_REC->LEN)
      TPTYPE_REC->LEN = (int32_t)len;
   memcpy(DATA_REC, data_rec, TPTYPE_REC->LEN);

   if (TPTYPE_REC->LEN < len ) {
     TPTYPE_REC->TPTYPE_STATUS = (int32_t)TPTRUNCATE;
   }
   else {
     TPTYPE_REC->TPTYPE_STATUS = (int32_t)TPTYPEOK;
   }

   tpfree(data_rec);
   return;
}



/* According to XATMI spec a service should use a COPY TPRETURN */
/* statement to finish processing. Possibly with REPLACING      */
/* clasuses. The TPRETURN copy file has code that calls TPRETURN*/
/* followed by an "EXIT PROGRAM" statement to do an immediate   */
/* return to the caller, that should be the "communications     */
/* resource mnanager" (E.g. Casual).                            */
/* NOTE: The C-function "tpreturn" unwind the stack to          */
/* the calling site in Casual (longjmp), A special              */
/* supporting variant of tpreturn is used to support the        */
/* COBOL API. This to avoid bypassing any processing needed in  */
/* the COBOL runtime as part of returning to the caller. This   */
/* variant of tpreturn does a normal return!                    */
/* Bypassing the COBOL runtime may (appear to?) work, but it is */
/* bad citizenship... The XATMI COBOL api is constructed this   */
/* way for a reason .                                           */
/* NOTE. What about buffer management. Any issues there? This   */
/* routine allocates a buffer with tpalloc and assumes that it  */
/* will be freed by Casual when it gets control.                */
/* NOTE: Is it useful/a good idea for TPRETURN and TPSVCSTART   */
/* to cooperate and resuse the input buffer for the reply?      */
/* (Perhaps also TPFORWARD. It exist in Tuxedo, but not in      */
/*  XATMI?)                                                     */
extern "C" void TPRETURN(struct TPSVCRET_REC_s *TPSVCRET_REC,
                         struct TPTYPE_REC_s *TPTYPE_REC,
                         char *DATA_REC,
                         struct TPSTATUS_REC_s *TPSTATUS_REC) {
   char rec_type[REC_TYPE_LEN + 1];
   char sub_type[SUB_TYPE_LEN + 1];
   char *data_rec;
   long flags;
   int rval;

   /* Copy COBOL string to C string and null terminate C string */
   cobstr_to_cstr(rec_type, TPTYPE_REC->REC_TYPE, REC_TYPE_LEN);
   cobstr_to_cstr(sub_type, TPTYPE_REC->SUB_TYPE, SUB_TYPE_LEN);

   /* Allocate typed buffers */
   if ((data_rec = (char *)tpalloc(rec_type,
                                   sub_type,
                                   TPTYPE_REC->LEN)) == nullptr) {
      TPSTATUS_REC->TP_STATUS = (int32_t)tperrno;
      /* printf("error TPCONNECT --> tpalloc: %d\n", tperrno); */
      return;
   }
   /* Copy data from COBOL record to buffer allocated with tpalloc */
   memcpy(data_rec, DATA_REC, TPTYPE_REC->LEN);

   /*
    * The Cobol TPSVCRET structure uses the 88 varaiables TPSUCCESS
    * with value 0 and TPFAIL with value 1 to signal success or failure
    * in the TP_RETURN_VAL member. This is NOT the same values as the
    * TPSUCCESS (=2) and TPFAIL (=1) defined in the C-interface to tpreturn()!
    * The interface say that (cobol) TPSUCCESS is succesful return,
    * and that (Cobol) TPFAIL and all other values are treated as a
    * service failure. (ontroduce contants in the header that defines
    * the C TPSVCRET_REC might be nicer, avoid compare with explicit 0)
    * Note:
    * There are different TPSUCCESS values defined in different
    * header files so I currently use hardcoded values here.
   */
   rval= (TPSVCRET_REC->TP_RETURN_VAL == 0) ? 2 : 1;

   /* flags for tpreturn is reserved for future use an shall be 0 */
   flags = 0;
   /* may need special version of tpreturn to support COBOL api   */
   /* error handling not needed. Calling program/routine          */
   /* will/should exit immediately after the return. C function   */
   /* tpreturn is void.                                           */
   tpreturn_cobol_support(rval,    /* int */
       TPSVCRET_REC->APPL_CODE,        /* expands to long */
       data_rec,                       /* char *          */
            TPTYPE_REC->LEN,                /* expands to long */
            flags);                         /* long            */
   return;
}

/*
 * A dummy for TPFORWARD
 * For now this just prints a message and does nothing!
 * TPFORWARD is a Tuxedo extension to XATMI. To some extent
 * it is similar to TPRETURN. E.g. The C version of it never
 * returns to the caller. The COBOL TPFORWARD is (like TPRETURN)
 * a COBOL copy that contains a call and an "exit".
*/

extern "C" void TPFORWARD(struct TPSVCDEF_REC_s *TPSVCDEF_REC,
                          struct TPTYPE_REC_s *TPTYPE_REC,
                          char *DATA_REC,
                          struct TPSTATUS_REC_s *TPSTATUS_REC) {
   printf("TPFORWARD called, not implemented, doing nothing!");
   return;
}

#if 1
extern "C" void TPSEND(struct TPSVCDEF_REC_s *TPSVCDEF_REC,
                       struct TPTYPE_REC_s *TPTYPE_REC,
                       char *DATA_REC,
                       struct TPSTATUS_REC_s *TPSTATUS_REC) {
// outline
// set up for call to tpsend:
//    determine flags
//    allocate buffer
//    copy data to buffer
// call tpsend
//   call tpsend
//   fill in TPSTATUS_REC
//   Handle event flags
//   free buffer (I assume this should be done here and not by casual
//                check sample code!)
//
long flags=0;
// Valid flags in tpsend:
// TPRECVONLY (change direction of conversation)
// TPNOBLOCK
// TPNOTIME
// TPSIGRSTRT
   if (TPSVCDEF_REC->TPSENDRECV_FLAG)
      flags = flags | TPRECVONLY;
   if (TPSVCDEF_REC->TPBLOCK_FLAG)
      flags = flags | TPNOBLOCK;
   if (TPSVCDEF_REC->TPTIME_FLAG)
      flags = flags | TPNOTIME;
   if (TPSVCDEF_REC->TPSIGRSTRT_FLAG)
      flags = flags | TPSIGRSTRT;

   char rec_type[REC_TYPE_LEN + 1];
   char sub_type[SUB_TYPE_LEN + 1];

   cobstr_to_cstr(rec_type, TPTYPE_REC->REC_TYPE, REC_TYPE_LEN);
   cobstr_to_cstr(sub_type, TPTYPE_REC->SUB_TYPE, SUB_TYPE_LEN);

   char *data_rec;
   if ((data_rec = (char *)tpalloc(rec_type,
                                   sub_type,
                                   TPTYPE_REC->LEN)) == nullptr) {
      TPSTATUS_REC->TP_STATUS = (int32_t)tperrno;
      /* printf("error TPSEND --> tpalloc: %d\n", tperrno); */
      return;
   }
   // Copy data to the allocated buffer
   //
   // This copy may not work correctly in the case that
   // the buffer type is of a type that does not require a
   // length value, AND the length has not been filled in (or do not match
   // the actual length).
   // This code assumes that TPTYPE_REC->LEN is always filled in.
   // Getting it to work with buffer types that do not require this
   // would require logic to look up the length associated with the
   // buffer type & subtype. I do not know how to do this (or if
   // Casual has the mechanisms to define new buffer types).
   // Not easy to detect this case as the spec say that the
   // length value is ignored for buffer types that do not require
   // it. Also length = 0 is perfectly normal. Can be used to hand
   // over control of conection without sending anny application data.
   // The buffer type X_OCTET always requires an explicit length,
   // so this buffer type should be safe.
   // X_COMMON (that is the other type that "always" should be
   // supported) requires a subtype, and probably does not require
   // a length.
   // For now I assume that length is always present and
   // reflects the actual size.
   // Note: the buffer allocation above also assumes that length
   // is filled in and is "correct".

   long data_length = TPTYPE_REC->LEN;
   memcpy(data_rec, DATA_REC, data_length);

   int rv;
   long revent;
   if ((rv = tpsend(TPSVCDEF_REC->COMM_HANDLE,
                  data_rec,
                  data_length,
                  flags,
                  &revent)) == -1) {
         TPSTATUS_REC->TP_STATUS = (int32_t)tperrno;
         /* printf("error TPSEND --> tpsend: %d\n", tperrno); */
   } else {
      TPSTATUS_REC->TP_STATUS = (int32_t)TPOK;
      if (tperrno == TPEEVENT) {
         TPSTATUS_REC->TP_STATUS=(int32_t)TPEEVENT;
         TPSTATUS_REC->TPEVENT = (int32_t)revent;
         if (TPSTATUS_REC->TPEVENT == TPEV_SVCFAIL) {
            TPSTATUS_REC->APPL_RETURN_CODE=(int32_t)tpurcode;
         }
      }
   }
   tpfree(data_rec);
}
#endif /* #if 1 */

#if 1
/* A cobol specific service routine need to be created to support */
/* TPSVCSTART. The normal way to invoke a service in C is to call */
/* it with a TPSVCINFO struct as an argument. In the general case */
/* the "communications resource manager" (e.g. Casual) does not   */
/* know the language of the service routine. It is therefore not  */
/* possible to know if a COBOL adapted struct should be used.     */
/* This is probably why TPSVCSTART has been inroduced in the      */
/* Cobol API. The special support routine need to retrive the     */
/* TPSVCINFO structure passed when the service was invoked.       */
/* TPSVCSTART can then construct the TPSVCDEF_REC structure from  */
/* this data.                                                     */
/* NOTE: It may be a good idea to have TPSVCSTART and TPRETURN    */
/* cooperate in handling the data buffer for input and output     */
/* data. I.e. reuse the input data for output, possibly after a   */
/* tprealloc if a larger buffer is needed for the return data.    */
/* NOTE:                                                          */
/* The comment above is the "original" comment in the Casual code */
/* branch feature/1.4 when more cobol support was added           */
/* For now, I have not implemented any cooperation in use of the  */
/* data buffer. It is "normal" to have a service routine in C     */
/* that calls code in COBOL that calls TPSVCSTART (possibly       */
/* multiple times in the same invocation).                        */
/* In at least one usage scenario it is normal that the           */
/* service entry point is in C, and that it dynamically loads     */
/* the COBOL code that then calls TPSVCSTART. Normally the COBOL  */
/* code uses the COBOL TPRETURN mechanism. I think it can happen  */
/* (error cases) that C code can allocate a response buffer and   */
/* call tpreturn() even if TPSVCSTART has been called.            */
/* Any cooperation in handling the data buffer should be robust   */
/* in exotic scenarios. I believe the rules for C routines is     */
/* that the service data buffer may not be passed to tpfree(),    */
/* but it may be passed to tprealloc(). In theory scenarios with  */
/* a tprealloc of the service input data buffer before a TPRETURN */
/* (from Cobol) may be possible. They are not likely!                          */
/* The tprealloc() routine may need to be aware of the saved      */
/* pointer to the service input buffer, and possibly update it.   */
/* Need more thinking!                                            */
void TPSVCSTART(struct TPSVCDEF_REC_s *TPSVCDEF_REC,
      struct TPTYPE_REC_s *TPTYPE_REC,
      char* DATA_REC,
      struct TPSTATUS_REC_s *TPSTATUS_REC) {
   int rv;
   // Outline:
   // copy service name to TPSVCDEF_REC
   // Retrive service call data (using special C-api function).
   // translate flags in TPSVCINFO to corresponding flags in
   //   TPSVCDEF_REC
   // copy COMM-HANDLE if present (conversational...)
   // copy data to COBOL data record, set TPOK or TPTRUNCATE
   //   in TPTYPE_REC depending on if data fits or not. LEN
   //   is set to length of data copied. (Should trailing
   //   part of callers buffer be set to SPACE? XATMI does not
   //   say, but it is a "COBOL-ish" thing to do... Probably not needed)
   // Possibly, save information about data buffer for use in TPRETURN.
   //

   if (TPTYPE_REC->LEN == 0) {
     TPSTATUS_REC->TP_STATUS = TPEPROTO;
     return;
   }

   const TPSVCINFO* tpsvcinfo;
   const char* buffer_type;
   const char* buffer_subtype;

   rv = tpsvcinfo_cobol_support(&tpsvcinfo, &buffer_type, &buffer_subtype);
   if (rv != TPOK) {
     TPSTATUS_REC->TP_STATUS = rv;
   }
   // SERVICENAME in TPSVCINFO is 127 characters COBOL style
   // name in TPSVCINFO is a C-style string in a 128 char
   // array. COBOL style name should not have a "null" in it,
   // and should be space padded.
   cstr_to_cobstr(TPSVCDEF_REC->SERVICE_NAME, tpsvcinfo->name, SERVICE_NAME_LEN);
   // Map flags.
   //
   // Only set flags that have meaning on service invocation.
   // A number of "flags" are not used/relevant when invoking a service.
   // Set to something predictable. The altertnative is to leave them
   // unchanged. The spec does not say what to do. It only specifies
   // what flags have meaning on return from TPSVCSTART.
   TPSVCDEF_REC->TPBLOCK_FLAG = 0;
   TPSVCDEF_REC->TPTIME_FLAG = 0;
   TPSVCDEF_REC->TPSIGRSTRT_FLAG = 0;
   TPSVCDEF_REC->TPGETANY_FLAG = 0;
   TPSVCDEF_REC->TPNOCHANGE_FLAG = 0;
   // TPTRAN_FLAG == 0 corresponds to active transaction, 1 is no transaction. 
   TPSVCDEF_REC->TPTRAN_FLAG       = (tpsvcinfo->flags & TPTRAN) ? 0 : 1;
   // TPREPLY_FLAG == 0 correponds to reply required, 1 for no reply
   // (can only occur for async req/response where caller specified 
   // TPNOTRAN and TPNOREPLY)   
   TPSVCDEF_REC->TPREPLY_FLAG      = (tpsvcinfo->flags & TPREPLY)  ? 0 : 1;
   TPSVCDEF_REC->TPSERVICETYPE_FLAG = (tpsvcinfo->flags & TPCONV)? 1 : 0;
   // C api TPSENDONLY and TPRECONLY are only relevant for conversational
   // services and are mutually exclusive. The COBOL api uses a single flag
   // while the C api has two flags (that never should be set at the same time).
   // If the client specifies TPSENDONLY it means that the client
   // intends to send more data (tpsend/TPSEND) and the service should get a 
   // TPRECVONLY flag (and should call tprecv/TPRECV). 
   // The coding in the Cobol API is that TPSVCDEF_REC->TPSENDRECV_FLAG has
   // the value 0 when indicating that the service is allowed to send data  
   // 
   TPSVCDEF_REC->TPSENDRECV_FLAG=0;
   if (tpsvcinfo->flags & TPCONV) {
     if (tpsvcinfo->flags & TPRECVONLY) {
        TPSVCDEF_REC->TPSENDRECV_FLAG = (tpsvcinfo->flags & TPRECVONLY) ? 1 : 0;
     }
   }

   // The COMM_HANDLE is only useful for conversational services
   // The purist way would be to set it to 0 and change it if
   // the service is conversational. The C api sets cd to 0
   // for request/response services so this is good enough. 
   TPSVCDEF_REC->COMM_HANDLE = tpsvcinfo->cd; 
   
   TPSVCDEF_REC->TPSERVICETYPE_FLAG=0;
   if (tpsvcinfo->flags & TPCONV) {
      TPSVCDEF_REC->TPSERVICETYPE_FLAG = 1;
   }
   // C API now passes TPTRAN flag. In the COBOL API
   // the TPTRAN_FLAG has the value 1 for TPNOTRAN. TPTRAN corresponds to 0.
   TPSVCDEF_REC->TPTRAN_FLAG = (tpsvcinfo->flags & TPTRAN) ? 0 : 1;

   // TPTYPE_REC. On return from TPSVCSTART:
   // REC_TYPE buffer type (X_OCTET or X_COMMON), size is REC_TYPE_LEN (8)
   // SUB-TYPE Buffer subtype, size is SUB_TYPE_LEN (16)
   // LEN Buffer length (amount of data received)
   // TPTYPE_STATUS (TPTYPEOK or TPTRUNCATE)
   // on entry the LEN is the size of the data buffer

   // Fill in buffer type and subtype. TPSVCSTART is specified to return
   // this information . (It is not supplied to the service entrypoint in
   // the C API!)
   cstr_to_cobstr(TPTYPE_REC->REC_TYPE, buffer_type, REC_TYPE_LEN);
   cstr_to_cobstr(TPTYPE_REC->SUB_TYPE, buffer_subtype, SUB_TYPE_LEN);

   // Copy data to callers buffer
   if (tpsvcinfo->len <= TPTYPE_REC->LEN) {
     TPTYPE_REC->TPTYPE_STATUS = TPTYPEOK;
     memcpy(DATA_REC, tpsvcinfo->data, tpsvcinfo->len);
     // Need to space fill buffer? <untested Casual developer version of TPRECV
     // does not space fill.)
     TPTYPE_REC->LEN = tpsvcinfo->len;
   } else {
     // Not enough space, copy as much as possible and indicate data truncated
     memcpy(DATA_REC, tpsvcinfo->data, TPTYPE_REC->LEN);
     TPTYPE_REC->TPTYPE_STATUS=TPTRUNCATE;
   }

   TPSTATUS_REC->TP_STATUS = rv;
}
#endif /* #if 0/1 */

#if 0
TPUNADVERTISE
Only used by server/service
#endif /* #if 0 */


/*
 * Copy string (COBOL) and terminate it with null character
*/
#define ASCII_SPACE 0x20
void cobstr_to_cstr(char *dest, const char *src, int len) {
   int i;

   i = 0;
   while ((src[i] > ASCII_SPACE) && (i < len)) {
      dest[i] = src[i];
      i++;
   }
   dest[i] = '\0';

   return;
}

/*
 * Copy C-string to a COBOL space padded string
 * The len argument is the target size. It is assumed that
 * the source is well formed and will fit in the target
*/
#define ASCII_SPACE 0x20
void cstr_to_cobstr(char *dest, const char *src, int len) {
   int i;

   i = 0;
   while ( (i < len) && (src[i] != 0) ) {
      dest[i] = src[i];
      i++;
   }
   // the above loop terminates on full output buffer or
   // a terminating null character in the src. Should it
   // instead stop on src[i] <= ASCII_SPACE ?.
   // The cobstr_to_cstr helper stops on first space
   // or other character <= ASCII_SPACE. This prevents
   // most non-printable characters from being copied.
   // If char is signed it also prevents copy of
   // characters >=0x80...
   while (i < len) {
     dest[i] = ASCII_SPACE;
     i++;
   }
   return;
}

