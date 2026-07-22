//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "casual/transaction/resource/proxy/server.h"
#include <xa.h>

extern "C" struct xa_switch_t casual_mockup_xa_switch_static;

int main( int argc, const char** argv)
{

   struct casual_xa_switch_mapping xa_mapping[] = {
      { "rm-mockup", &casual_mockup_xa_switch_static},
      { nullptr, nullptr} /* null ending */
   };

   struct casual_resource_proxy_service_argument serverArguments = {
         argc,
         argv,
         xa_mapping
   };


   /*
   // Start the server
   */
   return casual_start_resource_proxy( &serverArguments);
}





