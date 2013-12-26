//
// string_buffer.h
//
//  Created on: 6 nov 2013
//      Author: Kristone
//

#ifndef STRING_BUFFER_H_
#define STRING_BUFFER_H_

#include "buffers/casual_string_buffer.h"


//
// Callback C-functions
//
// TODO: Make this C++ (perhaps)
//
long CasualStringCreate( char* buffer, long size);
long CasualStringExpand( char* buffer, long size);
long CasualStringReduce( char* buffer, long size);
long CasualStringNeeded( char* buffer, long size);



#endif /* STRING_BUFFER_H_ */
