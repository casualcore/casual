/** 
 ** Copyright (c) 2018, The casual project
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

typedef int( *entrypoint_type)( int argc, char **argv);

struct casual_executable_arguments
{
   entrypoint_type entrypoint;

   int argc;
   char** argv;

   struct casual_xa_switch_map* xa_switches;
};


int casual_run_executable( struct casual_executable_arguments* arguments);


#ifdef __cplusplus
}
#endif