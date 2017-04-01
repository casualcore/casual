//!
//! casual 
//!

#ifndef CASUAL_MIDDLEWARE_XATMI_INCLUDE_XATMI_DEFINES_H_
#define CASUAL_MIDDLEWARE_XATMI_INCLUDE_XATMI_DEFINES_H_

#define TPNOBLOCK    0x00000001
#define TPSIGRSTRT   0x00000002
#define TPNOREPLY    0x00000004
#define TPNOTRAN     0x00000008
#define TPTRAN       0x00000010
#define TPNOTIME     0x00000020
#define TPGETANY     0x00000080
#define TPNOCHANGE   0x00000100
#define TPCONV       0x00000400
#define TPSENDONLY   0x00000800
#define TPRECVONLY   0x00001000

#define TPFAIL       0x00000001
#define TPSUCCESS    0x00000002


#define X_OCTET   "X_OCTET"
#define X_C_TYPE  "X_C_TYPE"
#define X_COMMON  "X_COMMON"

#define TPEBADDESC 2
#define TPEBLOCK 3
#define TPEINVAL 4
#define TPELIMIT 5
#define TPENOENT 6
#define TPEOS 7
#define TPEPROTO 9
#define TPESVCERR 10
#define TPESVCFAIL 11
#define TPESYSTEM 12
#define TPETIME 13
#define TPETRAN 14
#define TPGOTSIG 15
#define TPEITYPE 17
#define TPEOTYPE 18
#define TPEEVENT 22
#define TPEMATCH 23

#define TPEV_DISCONIMM  0x0001
#define TPEV_SVCERR     0x0002
#define TPEV_SVCFAIL    0x0004
#define TPEV_SVCSUCC    0x0008
#define TPEV_SENDONLY   0x0020


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
