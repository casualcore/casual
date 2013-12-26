//
// casual_octet_buffer.cpp
//
//  Created on: 22 okt 2013
//      Author: Kristone
//

#include "common/octet_buffer.h"

long CasualOctetCreate( char* const buffer, const long size)
{
   // nothing can go wrong with creation
   return CASUAL_OCTET_SUCCESS;
}

long CasualOctetExpand( char* const buffer, const long size)
{
   // nothing can go wrong with expansion and it's up to the user to decide
   return CASUAL_OCTET_SUCCESS;
}

long CasualOctetReduce( char* const buffer, const long size)
{
   // nothing can go wrong with reduction and it's up to the user to decide
   return CASUAL_OCTET_SUCCESS;
}

long CasualOctetNeeded( char* const buffer, const long size)
{
   // don't know the size by self and this stops the system to reduce it
   return size;
}
