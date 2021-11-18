/** 
 ** Copyright (c) 2015, The casual project
 **
 ** This software is licensed under the MIT license, https://opensource.org/licenses/MIT
 **/

#pragma once

#define TPOK 0
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

#ifdef __cplusplus
extern "C" {
#endif

/*  
exposed for users to be able to set explict tperrno
could be usefull for 'integrations' with other xatmi-implementations
*/
extern void casual_set_tperrno( int code);

#ifdef __cplusplus
}
#endif
