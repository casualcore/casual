//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/uuid.h"

#include <system_error>


namespace casual
{
   namespace common
   {
      namespace code
      {
         namespace serialize
         {
            namespace detail
            {
               namespace lookup
               {
                  struct Entry
                  {
                     const std::error_category* category;
                     Uuid id;
                  };

                  void registration( Entry entry);

               } // lookup
               
            } // detail

            namespace lookup
            {
               const Uuid& id( const std::error_category& category);
               
            } // lookup

            std::error_code create( const Uuid& id, int code);


            template< typename Category>
            const Category& registration( const Uuid& id)
            {
               static const Category category;
               detail::lookup::registration( { &category, id});
               return category;
            }

            
         } // serialize 
      } // code
   } // common
} // casual