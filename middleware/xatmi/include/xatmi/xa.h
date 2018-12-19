/** 
 ** Copyright (c) 2015, The casual project
 **
 ** This software is licensed under the MIT license, https://opensource.org/licenses/MIT
 **/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/** 
 ** @deprecated 
 **/
struct casual_xa_switch_mapping
{
   const char* key;
   struct xa_switch_t* xa_switch;
};

struct casual_xa_switch_map
{
   const char* key;
   const char* name;
   struct xa_switch_t* xa_switch;
};

#ifdef __cplusplus
}
#endif