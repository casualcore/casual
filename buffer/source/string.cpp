//!
//! string.cpp
//!
//! Created on: Dec 26, 2013
//!     Author: Lazan
//!

#include "buffer/string.h"

#include "common/buffer/pool.h"



namespace casual
{
   namespace buffer
   {
      namespace implementation
      {

         class String : public common::buffer::pool::default_pool
         {
         public:

            using types_type = common::buffer::pool::default_pool::types_type;

            static const types_type& types()
            {
               static const types_type result{ { CASUAL_STRING, ""}};
               return result;
            }


            /*
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

            */


         };


      } // implementation
   } // buffer

   //
   // Registration
   //
   template class common::buffer::pool::Registration< buffer::implementation::String>;
} // casual
