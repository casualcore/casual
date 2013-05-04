//!
//! xatmi.h
//!
//! Created on: Mar 28, 2012
//!     Author: Lazan
//!
//! "stolen" from BlackTie for the time being..
//!


#ifndef XATMI_H_
#define XATMI_H_

#define TPNOBLOCK 0x00000001
#define TPSIGRSTRT 0x00000002
#define TPNOREPLY 0x00000004
#define TPNOTRAN 0x00000008
#define TPTRAN 0x00000010
#define TPNOTIME 0x00000020
#define TPGETANY 0x00000080
#define TPNOCHANGE 0x00000100
#define TPCONV 0x00000400
#define TPSENDONLY 0x00000800
#define TPRECVONLY 0x00001000

#define TPFAIL		0x00000001
#define TPSUCCESS	0x00000002

struct tpsvcinfo {
#define XATMI_SERVICE_NAME_LENGTH  128
	char name[ XATMI_SERVICE_NAME_LENGTH];
	char *data;
	long len;
	long flags;
	int cd;
};
typedef struct tpsvcinfo TPSVCINFO;

#define X_OCTET		"X_OCTET"
#define X_C_TYPE	"X_C_TYPE"
#define X_COMMON	"X_COMMON"

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

#define TPEV_DISCONIMM 0x0001
#define TPEV_SVCERR 0x0002
#define TPEV_SVCFAIL 0x0004
#define TPEV_SVCSUCC 0x0008
#define TPEV_SENDONLY 0x0020

// NOW STARTS THE MAIN OPERATION LIST


#ifdef __cplusplus
extern "C" {
#endif
typedef void( *tpservice)( TPSVCINFO *);

extern char* tpalloc( const char* type, const char* subtype, long size); // MEMORY
extern char* tprealloc( char * addr, long size); // MEMORY


extern int tpcall( const char * svc, char* idata, long ilen, char ** odata, long *olen, long flags); // COMMUNICATION
extern int tpacall( const char * svc, char* idata, long ilen, long flags); // COMMUNICATION
extern int tpgetrply(int *idPtr, char ** odata, long *olen, long flags); // COMMUNICATION
extern int tpcancel(int id); // COMMUNICATION
extern long tptypes( const char* ptr, char* type, char* subtype); // MEMORY
extern void tpfree(char* ptr); // MEMORY

extern void tpreturn(int rval, long rcode, char* data, long len, long flags); // TJJ ADDED

extern int tpadvertise( const char* svcname, void(*func)(TPSVCINFO *)); // SERVER
extern int tpunadvertise( const char* svcname); // SERVER

extern int tpsend(int id, char* idata, long ilen, long flags, long *revent); // COMMUNICATION
extern int tprecv(int id, char ** odata, long *olen, long flags, long* event); // COMMUNICATION
extern int tpconnect(char * svc, char* idata, long ilen, long flags); // COMMUNICATION
extern int tpdiscon(int id); // COMMUNICATION

extern int tperrno;
extern long tpurcode;
/*
 * Not sure what blacktie uses this for...
 *
extern  int _get_tperrno(void); // CLIENT
extern  long _get_tpurcode(void); // CLIENT
*/


/*
 * "extended"
 */
extern const char* tperrnostring( int error);

extern int tpsvrinit(int argc, char **argv);


extern void tpsvrdone();




#ifdef __cplusplus
}
#endif


#endif /* XATMI_H_ */
