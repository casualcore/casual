//!
//! string.cpp
//!
//! Created on: Dec 26, 2013
//!     Author: Lazan
//!

#include "buffer/string.h"

#include "common/buffer_context.h"


/*
 * Old stuff... Don't really get why all of it is needed.

#include "common/string_buffer.h"

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

}


 */

namespace casual
{
   namespace buffer
   {
      namespace implementation
      {

         class String : public common::buffer::implementation::Base
         {

            void doCreate( common::buffer::Buffer& buffer, std::size_t size) override
            {
               // check size?

               buffer.memory().resize( size);

               // do stuff
            }

            void doReallocate( common::buffer::Buffer& buffer, std::size_t size) override
            {
               buffer.memory().resize( size);
            }

            static const bool initialized;
         };




         const bool String::initialized = common::buffer::implementation::registrate< String>( {{ CASUAL_STRING, ""}});



      } // implementation
   } // buffer
} // casual
