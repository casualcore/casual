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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <xatmi.h>

/*
 * Internal support functions
*/
void cobstr_to_cstr(char *, const char *, int);


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
                                   TPTYPE_REC->LEN)) == NULL) {
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
   else
      flags = flags | TPTRAN;
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
                                    ITPTYPE_REC->LEN)) == NULL) {
      TPSTATUS_REC->TP_STATUS = (int32_t)tperrno;
      /* printf("error TPCALL --> idata_rec = tpalloc: %d\n", tperrno); */
      return;
   }
   if ((odata_rec = (char *)tpalloc(rec_type,
                                    sub_type,
                                    OTPTYPE_REC->LEN)) == NULL) {
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
   else
      flags = flags | TPTRAN;
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
      if (tperrno == TPESVCFAIL)
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

   /* Allocate typed buffers */
   if ((data_rec = (char *)tpalloc(rec_type,
                                   sub_type,
                                   TPTYPE_REC->LEN)) == NULL) {
      TPSTATUS_REC->TP_STATUS = (int32_t)tperrno;
      /* printf("error TPCONNECT --> tpalloc: %d\n", tperrno); */
      return;
   }

   /* Copy data from COBOL record to buffer allocated with tpalloc */
   memcpy(data_rec, DATA_REC, TPTYPE_REC->LEN);

   /* Convert COBOL flags (record) to C flags (bit map) */
   flags = 0;
   if (TPSVCDEF_REC->TPTRAN_FLAG)
      flags = flags | TPNOTRAN;
   else
      flags = flags | TPTRAN;
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
      if (TPSVCDEF_REC->TPREPLY_FLAG ==  TPREPLY)
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
                                   TPTYPE_REC->LEN)) == NULL) {
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
      if (tperrno == TPESVCFAIL)
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
                                   TPTYPE_REC->LEN)) == NULL) {
      TPSTATUS_REC->TP_STATUS = (int32_t)tperrno;
      /* printf("error TPGETRPLY --> tpalloc: %d\n", tperrno); */
      return;
   }

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
   } else {
      TPSTATUS_REC->TP_STATUS = (int32_t)TPOK;
      if (tperrno == TPEEVENT) {
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



/* According to XATMI spec a service should use a COPY TPRETURN */
/* statement to finish processing. Possibly with REPLACING      */
/* clasuses. The TPRETURN copy file has code that calls TPRETURN*/
/* followed by an "EXIT PROGRAM" statement to do an immediate   */
/* return to the caller, that should be the "communications     */
/* resource mnanager" (E.g. Casual).                            */
/* NOTE: Does the C-function "tpreturn" unwind the stack to     */
/* the calling site in Casual (longjmp)? If so a special        */
/* supporting variant of tpreturn may be needed to support th   */
/* COBOL API. This to avoid bypassing any processing needed in  */
/* the COBOL runtime as part of returning to the caller. This   */
/* variant of tpreturn should do a normal return!               */
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

   /* Copy COBOL string to C string and null terminate C string */
   cobstr_to_cstr(rec_type, TPTYPE_REC->REC_TYPE, REC_TYPE_LEN);
   cobstr_to_cstr(sub_type, TPTYPE_REC->SUB_TYPE, SUB_TYPE_LEN);

   /* Allocate typed buffers */
   if ((data_rec = (char *)tpalloc(rec_type,
                                   sub_type,
                                   TPTYPE_REC->LEN)) == NULL) {
      TPSTATUS_REC->TP_STATUS = (int32_t)tperrno;
      /* printf("error TPCONNECT --> tpalloc: %d\n", tperrno); */
      return;
   }
   /* Copy data from COBOL record to buffer allocated with tpalloc */
   memcpy(data_rec, DATA_REC, TPTYPE_REC->LEN);

   /* flags for tpreturn is reserved for future use an shall be 0 */
   flags = 0;
   /* may need special version of tpreturn to support COBOL api   */
   /* error handling not needed. Calling program/routine          */
   /* will/should exit immediately after the return. C function   */
   /* tpreturn is void.                                           */
   tpreturn(TPSVCRET_REC->TP_RETURN_VAL,    /* int             */
	    TPSVCRET_REC->APPL_CODE,        /* expands to long */
	    data_rec,                       /* char *          */
            TPTYPE_REC->LEN,                /* expands to long */
            flags);                         /* long            */
   return;
}



#if 0
TPSEND
#endif /* #if 0 */

#if 0
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
void TPSVCSTART(struct TPSVCDEF_REC_s *TPSVCDEF_REC,
		struct TPTYPE_REC_s *TPTYPE_REC,
		char* DATA_REC,
		struct TPSTATUS_REC_s *TPSTATUS_REC) {
   char rec_type[REC_TYPE_LEN + 1];
   char sub_type[SUB_TYPE_LEN + 1];
   char *data_rec;
   int rv;
   long flags;
   long len;
   long revent;
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
   printf("TPSVCSTART not yet implemented\n");
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

