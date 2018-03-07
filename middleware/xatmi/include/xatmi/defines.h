//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#ifndef CASUAL_MIDDLEWARE_XATMI_INCLUDE_XATMI_DEFINES_H_
#define CASUAL_MIDDLEWARE_XATMI_INCLUDE_XATMI_DEFINES_H_


#define X_OCTET   "X_OCTET"
#define X_C_TYPE  "X_C_TYPE"
#define X_COMMON  "X_COMMON"


struct tpsvcinfo {
#define XATMI_SERVICE_NAME_LENGTH  128
   char name[ XATMI_SERVICE_NAME_LENGTH];
   char *data;
   long len;
   long flags;
   int cd;
};
typedef struct tpsvcinfo TPSVCINFO;

#endif // CASUAL_MIDDLEWARE_XATMI_INCLUDE_XATMI_DEFINES_H_
