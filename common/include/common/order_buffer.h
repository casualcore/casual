//
// order_buffer.h
//
//  Created on: 6 nov 2013
//      Author: Kristone
//

#ifndef ORDER_BUFFER_H_
#define ORDER_BUFFER_H_

#include "buffers/casual_order_buffer.h"

//
// Callback C-functions
//
// TODO: Make this C++ (perhaps)
//
long CasualOrderCreate( char* buffer, long size);
long CasualOrderExpand( char* buffer, long size);
long CasualOrderReduce( char* buffer, long size);
long CasualOrderNeeded( char* buffer, long size);


#endif /* ORDER_BUFFER_H_ */
