/** 
 ** Copyright (c) 2015, The casual project
 **
 ** This software is licensed under the MIT license, https://opensource.org/licenses/MIT
 **/

#pragma once



#include <casual/xatmi.h>
#include <casual/xatmi/xa.h>

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


struct casual_service_definition
{
   tpservice function_pointer;
   const char* name;

   /* type of service */
   const char* category;

   /* transaction policy */
   uint64_t transaction;

   /* service visibilit*/
   uint64_t visibility;
};

typedef void( *tpsvrdone_type)();
typedef int( *tpsvrinit_type)( int argc, char **argv);

struct casual_server_arguments_v2
{
   struct casual_service_definition* services;

   tpsvrinit_type server_init;
   tpsvrdone_type server_done;

   int argc;
   char** argv;

   struct casual_xa_switch_map* xa_switches;
};


int casual_run_server_v2( struct casual_server_arguments_v2* arguments);



/** 
 ** @deprecated stuff that should be removed in 2.0
 **/

struct casual_service_name_mapping
{
   tpservice function_pointer;
   const char* name;

   /* type of service */
   const char* category;

   /* transaction policy */
   uint64_t transaction;
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

int casual_run_server( struct casual_server_arguments* arguments);

struct casual_server_argument
{
   struct casual_service_name_mapping* services;

   tpsvrinit_type server_init;
   tpsvrdone_type server_done;

   int argc;
   char** argv;

   struct casual_xa_switch_mapping* xa_switches;
};

int casual_start_server( struct casual_server_argument* arguments);

#ifdef __cplusplus
}
#endif

