//
// casual_string_buffer.cpp
//
//  Created on: 22 okt 2013
//      Author: Kristone
//

#include "casual_string_buffer.h"

#include <cstring>

long CasualStringCreate( char* const buffer, const long size)
{
   if( size < 1)
   {
      // we need room for a '\0'
      return CASUAL_STRING_NO_SPACE;
   }

   //buffer[0] = '\0';

   std::memset( buffer, '\0', size);

   return CASUAL_STRING_SUCCESS;
}

long CasualStringExpand( char* const buffer, const long size)
{
   const auto current = std::strlen( buffer);

   if( current < size)
   {
      std::memset( buffer + current, '\0', size - current);
   }
   else
   {
      // someone have abused the buffer and we need to report some error
      return CASUAL_STRING_NO_PLACE;
   }

   return CASUAL_STRING_SUCCESS;
}

long CasualStringReduce( char* const buffer, const long size)
{
   const auto current = std::strlen( buffer);

   if( current < size)
   {
      return CASUAL_STRING_SUCCESS;
   }
   else
   {
      // the buffer cannot be reduced
      return CASUAL_STRING_NO_SPACE;
   }
}
long CasualStringNeeded( char* const buffer, const long size)
{
   return std::strlen( buffer) + 1;

/*
   const auto current = std::strlen( buffer);

   if( current < size)
   {
      return current + 1;
   }
   else
   {
      // someone have abused the buffer and we need to report some error
      return CASUAL_STRING_NO_PLACE;
   }
*/
}

