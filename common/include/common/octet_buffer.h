//
// octet_buffer.h
//
//  Created on: 6 nov 2013
//      Author: Kristone
//

#ifndef OCTET_BUFFER_H_
#define OCTET_BUFFER_H_

#include "buffers/casual_octet_buffer.h"


//
// Callback C-functions
//
// TODO: Make this C++ (perhaps)
//
long CasualOctetCreate( char* buffer, long size);
long CasualOctetExpand( char* buffer, long size);
long CasualOctetReduce( char* buffer, long size);
long CasualOctetNeeded( char* buffer, long size);



#endif /* OCTET_BUFFER_H_ */
