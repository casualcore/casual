//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include <casual/xatmi/server.h>

#ifdef __cplusplus
extern "C" {
#endif



struct casual_resource_proxy_service_argument
{

   int argc;
   char** argv;

   struct casual_xa_switch_mapping* xaSwitches;
};

int casual_start_resource_proxy( struct casual_resource_proxy_service_argument* serverArguments);


#ifdef __cplusplus
}
#endif


