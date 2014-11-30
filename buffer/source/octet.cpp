//!
//! octet.cpp
//!
//! Created on: Dec 26, 2013
//!     Author: Lazan
//!

#include "buffer/octet.h"

#include "common/buffer/pool.h"

namespace casual
{
   namespace buffer
   {
      namespace implementation
      {

         class Octet : public common::buffer::pool::default_pool
         {
         public:

            using types_type = common::buffer::pool::default_pool::types_type;

            static const types_type& types()
            {
               static const types_type result{ { CASUAL_OCTET, ""}};
               return result;
            }

            //
            // We rely on x_octet implementation
            //
         };



      } // implementation


   } // buffer

   template class common::buffer::pool::Registration< buffer::implementation::Octet>;

} // casual

