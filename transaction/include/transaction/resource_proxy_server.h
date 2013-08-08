//!
//! resource_proxy_server.h
//!
//! Created on: Aug 2, 2013
//!     Author: Lazan
//!

#ifndef RESOURCE_PROXY_SERVER_H_
#define RESOURCE_PROXY_SERVER_H_

#include <xatmi_server.h>

#ifdef __cplusplus
extern "C" {
#endif



struct casual_resource_proxy_service_argument
{

   int argc;
   char** argv;

   struct casual_xa_switch_mapping* xaSwitches;
};

int casual_start_reource_proxy( struct casual_resource_proxy_service_argument* serverArguments);



#ifdef __cplusplus
}
#endif

#endif /* RESOURCE_PROXY_SERVER_H_ */
