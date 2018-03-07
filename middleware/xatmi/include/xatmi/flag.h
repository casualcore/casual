//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#ifndef CASUAL_MIDDLEWARE_XATMI_INCLUDE_XATMI_FLAG_H_
#define CASUAL_MIDDLEWARE_XATMI_INCLUDE_XATMI_FLAG_H_

#define TPFAIL       0x00000001
#define TPSUCCESS    0x00000002

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


#define TPEV_DISCONIMM  0x0001
#define TPEV_SVCERR     0x0002
#define TPEV_SVCFAIL    0x0004
#define TPEV_SVCSUCC    0x0008
#define TPEV_SENDONLY   0x0020


#endif // CASUAL_MIDDLEWARE_XATMI_INCLUDE_XATMI_FLAG_H_
