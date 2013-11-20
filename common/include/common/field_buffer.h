//
// field_buffer.h
//
//  Created on: 6 nov 2013
//      Author: Kristone
//

#ifndef FIELD_BUFFER_H_
#define FIELD_BUFFER_H_

#include "buffers/casual_field_buffer.h"

//
// Callback C-functions
//
// TODO: Make this C++ (perhaps)
//
long CasualFieldCreate( char* buffer, long size);
long CasualFieldExpand( char* buffer, long size);
long CasualFieldReduce( char* buffer, long size);
long CasualFieldNeeded( char* buffer, long size);


#endif /* FIELD_BUFFER_H_ */
