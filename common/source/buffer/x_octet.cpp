//!
//! x_octet.cpp
//!
//! Created on: Sep 18, 2014
//!     Author: Lazan
//!

#include "common/buffer/pool.h"


namespace casual
{

   namespace common
   {
      namespace buffer
      {

         struct x_octet : public pool::default_pool
         {
            static const std::vector< Type>& types()
            {
               static const std::vector< Type> result{
                     type::x_octet(), type::binary(), type::json(), type::yaml(), type::xml()
                  };
               return result;
            }
         };

         //
         // Registrate the pool to the pool-holder
         //
         template class pool::Registration< x_octet>;

      } // buffer
   } // common

} // casual
