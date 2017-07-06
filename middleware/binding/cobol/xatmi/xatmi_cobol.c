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

#include "../xatmi/xatmi_cobol.h"

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
                        struct DATA_REC_s *DATA_REC,
                        struct TPSTATUS_REC_s *TPSTATUS_REC) {

   char service_name[SERVICE_NAME_LEN +1];
   char rec_type[REC_TYPE_LEN + 1];
   char sub_type[SUB_TYPE_LEN + 1];
   char *data_rec;
   int i;
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
   for (i = 0; i < TPTYPE_REC->LEN; i++)
      data_rec[i] = DATA_REC->DATA[i];

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
*/
#endif /* #if 0 */


extern "C" void TPCALL(struct TPSVCDEF_REC_s *TPSVCDEF_REC,
                       struct TPTYPE_REC_s *ITPTYPE_REC,
                       struct DATA_REC_s *IDATA_REC,
                       struct TPTYPE_REC_s *OTPTYPE_REC,
                       struct DATA_REC_s *ODATA_REC,
                       struct TPSTATUS_REC_s *TPSTATUS_REC) {

   char service_name[SERVICE_NAME_LEN + 1];
   char rec_type[REC_TYPE_LEN + 1];
   char sub_type[SUB_TYPE_LEN + 1];
   char *idata_rec;
   char *odata_rec;
   long olen;
   int i;
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
   for (i = 0; i < ITPTYPE_REC->LEN; i++)
      idata_rec[i] = IDATA_REC->DATA[i];

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
   for (i = 0; i < OTPTYPE_REC->LEN; i++)
      ODATA_REC->DATA[i] = odata_rec[i];

   tpfree(odata_rec);
   tpfree(idata_rec);

   return; 
}


extern "C" void TPCANSEL(struct TPSVCDEF_REC_s *TPSVCDEF_REC,
                         struct TPSTATUS_REC_s *TPSTATUS_REC) {

   int rv;

   if ((rv = tpcancel(TPSVCDEF_REC->COMM_HANDLE)) == -1) {
      TPSTATUS_REC->TP_STATUS = (int32_t)tperrno;
      /* printf("error TPCANSEL --> tpcansel: %d\n", tperrno); */
   } else {
      TPSTATUS_REC->TP_STATUS = (int32_t)TPOK;
   }

   return; 
}


#if 0  /* undefined reference to `tpconnect' */
extern "C" void TPCONNECT(struct TPSVCDEF_REC_s *TPSVCDEF_REC,
                          struct TPTYPE_REC_s *TPTYPE_REC,
                          struct DATA_REC_s *DATA_REC,
                          struct TPSTATUS_REC_s *TPSTATUS_REC) {

   char service_name[SERVICE_NAME_LEN +1];
   char rec_type[REC_TYPE_LEN + 1];
   char sub_type[SUB_TYPE_LEN + 1];
   char *data_rec;
   int i;
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
   for (i = 0; i < TPTYPE_REC->LEN; i++)
      data_rec[i] = DATA_REC->DATA[i];

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
#endif /* #if 0 */


#if 0  /* undefined reference to `tpdiscon' */
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
#endif /* #if 0 */


extern "C" void TPGETRPLY(struct TPSVCDEF_REC_s *TPSVCDEF_REC,
                        struct TPTYPE_REC_s *TPTYPE_REC,
                        struct DATA_REC_s *DATA_REC,
                        struct TPSTATUS_REC_s *TPSTATUS_REC) {

   char service_name[SERVICE_NAME_LEN +1];
   char rec_type[REC_TYPE_LEN + 1];
   char sub_type[SUB_TYPE_LEN + 1];
   char *data_rec;
   int i;
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
   if (len < TPTYPE_REC->LEN)
      TPTYPE_REC->LEN = (int32_t)len;
   for (i = 0; i < TPTYPE_REC->LEN; i++) 
      DATA_REC->DATA[i] = data_rec[i];

   tpfree(data_rec);
   return;
}


#if 0
TPRECV
#endif /* #if 0 */

#if 0
TPRETURN
Only used by service
#endif /* #if 0 */

#if 0
TPSEND
#endif /* #if 0 */

#if 0
TPSVCSTART
Only used by service
#endif /* #if 0 */

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

