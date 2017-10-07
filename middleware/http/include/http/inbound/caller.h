
#ifndef HTTP_INBOUND_CALLER_
#define HTTP_INBOUND_CALLER_
#include <xatmi.h>

#ifdef __cplusplus
extern "C" {
#endif

   typedef struct CasualHeaderS
   {
	   char key[80];
	   char value[80];
   } CasualHeader;

   typedef struct PayloadS
   {
      char* data;
      long size;
   } Payload;

   typedef struct CasualBufferS
   {
	   CasualHeader* header;
	   long headersize;
	   long context;
	   char service[XATMI_SERVICE_NAME_LENGTH];
	   long calldescriptor;
	   long errorcode;
	   long format;
	   char protocol[80];
	   Payload payload;
   } CasualBuffer;

   long casual_xatmi_send( CasualBuffer* data);
   long casual_xatmi_receive( CasualBuffer* data);

   enum {OK, AGAIN, ERROR};

#ifdef __cplusplus
}
#endif



#endif /* HTTP_INBOUND_CALLER_ */
