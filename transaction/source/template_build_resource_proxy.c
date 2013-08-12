//!
//! template_build_resource_proxy.c
//!
//! Created on: Aug 2, 2013
//!     Author: Lazan
//!



#include <resource_proxy_server.h>
#include <xa.h>



#ifdef __cplusplus
extern "C" {
#endif


extern struct xa_switch_t casual_mockup_xa_switch_static;


int main( int argc, char** argv)
{


   struct casual_xa_switch_mapping xa_mapping[] = {
           { "db2", &casual_mockup_xa_switch_static},
           { 0, 0} /* null ending */
      };


   struct casual_resource_proxy_service_argument serverArguments = {
         argc,
         argv,
         xa_mapping
   };


   /*
   // Start the server
   */
   return casual_start_reource_proxy( &serverArguments);

}







#ifdef __cplusplus
}
#endif




