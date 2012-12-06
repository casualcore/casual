//!
//! types.h
//!
//! Created on: Nov 24, 2012
//!     Author: Lazan
//!

#ifndef TYPES_H_
#define TYPES_H_

#include <vector>

namespace casual
{
   namespace common
   {
      typedef std::vector< char> binary_type;

      typedef const char* raw_buffer_type;

      namespace transform
      {
         inline char* public_buffer( raw_buffer_type buffer)
         {
            return const_cast< char*>( buffer);
         }

      }

   }

}



#endif /* TYPES_H_ */
