/*	(c) 2003 BEA Systems, Inc. All Rights Reserved. */
/*	Copyright (c) 1997 BEA Systems, Inc.
  	All rights reserved

  	THIS IS UNPUBLISHED PROPRIETARY
  	SOURCE CODE OF BEA Systems, Inc.
  	The copyright notice above does not
  	evidence any actual or intended
  	publication of such source code.
*/

/* #ident	"@(#) samples/atmi/simpapp/simpcl.c	$Revision: 1.5 $" */

#include <stdio.h>
// #include "atmi.h"		/* TUXEDO  Header File */
#include <stdlib.h>
#include <string.h>
#include <xatmi.h>


#if defined(__STDC__) || defined(__cplusplus)
main(int argc, char *argv[])
#else
main(argc, argv)
int argc;
char *argv[];
#endif

{

	char *sendbuf, *rcvbuf;
	long sendlen, rcvlen;
	int ret;
	char *tmpbuf;

	if(argc != 2) {
		(void) fprintf(stderr, "Usage: simpcl string\n");
		exit(1);
	}

#if 0
	/* Attach to System/T as a Client Process */
	if (tpinit((TPINIT *) NULL) == -1) {
		(void) fprintf(stderr, "Tpinit failed\n");
		exit(1);
	}
#endif /* #if 0 */
	
	sendlen = strlen(argv[1]);

	/* Allocate STRING buffers for the request and the reply */

/*
	if((sendbuf = (char *) tpalloc("STRING", NULL, sendlen+1)) == NULL) {
*/
	if((sendbuf = (char *) tpalloc("X_OCTET", NULL, sendlen+1)) == NULL) {
		(void) fprintf(stderr,"Error allocating send buffer\n");
/*
		tpterm();
*/
		exit(1);
	}

/*
	if((rcvbuf = (char *) tpalloc("STRING", NULL, sendlen+1)) == NULL) {
*/
	if((rcvbuf = (char *) tpalloc("X_OCTET", NULL, sendlen+1)) == NULL) {
		(void) fprintf(stderr,"Error allocating receive buffer\n");
		tpfree(sendbuf);
/*
		tpterm();
*/
		exit(1);
	}

	(void) strcpy(sendbuf, argv[1]);

	/* Request the service TOUPPER, waiting for a reply */
/*
	ret = tpcall("TOUPPER", (char *)sendbuf, 0, (char **)&rcvbuf, &rcvlen, (long)0);
*/
	ret = tpcall("TOUPPER", (char *)sendbuf, sendlen, (char **)&rcvbuf, &rcvlen, (long)0);

	if(ret == -1) {
		(void) fprintf(stderr, "Can't send request to service TOUPPER\n");
		(void) fprintf(stderr, "Tperrno = %d\n", tperrno);
		tpfree(sendbuf);
		tpfree(rcvbuf);
/*
		tpterm();
*/
		exit(1);
	}

/*
	(void) fprintf(stdout, "Returned string is: %s\n", rcvbuf);
*/
        tmpbuf = (char *)malloc(rcvlen);
        strncpy(tmpbuf, rcvbuf, rcvlen);
        tmpbuf[rcvlen] = '\0';
	(void) fprintf(stdout, "Returned string is: %s\n", tmpbuf);
	free(tmpbuf);

	/* Free Buffers & Detach from System/T */
	tpfree(sendbuf);
	tpfree(rcvbuf);
/*
	tpterm();
*/
	return(0);
}
