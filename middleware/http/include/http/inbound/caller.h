
#ifndef HTTP_INBOUND_CALLER_
#define HTTP_INBOUND_CALLER_
#include <xatmi.h>

#ifdef __cplusplus

#include <map>
#include <iostream>

namespace std
{
   std::ostream& operator<<( std::ostream& stream, std::map< std::string, std::string> input);
}

extern "C" {
#endif

   typedef struct CasualHeaderS
   {
	   char key[80];
	   char value[80];
   } CasualHeader;

   typedef struct BufferS
   {
      char* data;
      long size;
   } Buffer;

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
	   Buffer payload;
	   Buffer parameter;
   } CasualBuffer;

   long casual_xatmi_send( CasualBuffer* data);
   long casual_xatmi_receive( CasualBuffer* data);
   long casual_xatmi_cancel( CasualBuffer* data);

   enum {OK, AGAIN, ERROR};
   enum xatmi_context {cTPINIT, cTPALLOC, cTPACALL, cTPGETRPLY};

#ifdef __cplusplus
}
#endif



#endif /* HTTP_INBOUND_CALLER_ */
