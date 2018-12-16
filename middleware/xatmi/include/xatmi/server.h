/** 
 ** Copyright (c) 2015, The casual project
 **
 ** This software is licensed under the MIT license, https://opensource.org/licenses/MIT
 **/

#pragma once



#include <xatmi.h>
#include <xatmi/xa.h>

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


struct casual_service_name_mapping
{
   tpservice function_pointer;
   const char* name;

   /* type of service */
   const char* category;

   /* transaction policy */
   uint64_t transaction;

};



typedef void( *tpsvrdone_type)();
typedef int( *tpsvrinit_type)( int argc, char **argv);


/** 
 ** @deprecated 
 **/
struct casual_server_argument
{
   struct casual_service_name_mapping* services;

   tpsvrinit_type server_init;
   tpsvrdone_type server_done;

   int argc;
   char** argv;

   struct casual_xa_switch_mapping* xa_switches;
};

struct casual_server_arguments
{
   struct casual_service_name_mapping* services;

   tpsvrinit_type server_init;
   tpsvrdone_type server_done;

   int argc;
   char** argv;

   struct casual_xa_switch_map* xa_switches;
};


int casual_initialize_server( int argc, char** argv, struct casual_service_name_mapping* mapping, size_t size);

/** 
 ** @deprecated 
 **/
int casual_start_server( struct casual_server_argument* arguments);

int casual_run_server( struct casual_server_arguments* arguments);


#ifdef __cplusplus
}
#endif

