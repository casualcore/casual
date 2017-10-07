/*
 * platuxedo_nginx_utility_caller.h
 *
 *  Created on: 2 okt. 2017
 *      Author: 40043017
 */

#ifndef PLATTFORMSTJANSTER_V_TUXEDO_NGINX_PLUGIN_UTILITIES_INC_PLATUXEDO_NGINX_UTILITY_CALLER_H_
#define PLATTFORMSTJANSTER_V_TUXEDO_NGINX_PLUGIN_UTILITIES_INC_PLATUXEDO_NGINX_UTILITY_CALLER_H_
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



#endif /* PLATTFORMSTJANSTER_V_TUXEDO_NGINX_PLUGIN_UTILITIES_INC_PLATUXEDO_NGINX_UTILITY_CALLER_H_ */
