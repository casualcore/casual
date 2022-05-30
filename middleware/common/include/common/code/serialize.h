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
   namespace common::code::serialize
   {
      namespace lookup
      {
         namespace detail
         {
            struct Entry
            {
               const std::error_category* category;
               Uuid id;
            };

            std::ostream& operator << ( std::ostream& out, const Entry& value);

            void registration( Entry entry);

            //! unittest only
            const std::vector< Entry>& state() noexcept;
                                       
         } // detail

         const Uuid& id( const std::error_category& category);
         
      } // lookup

      std::error_code create( const Uuid& id, int code);

      //! register the Category to an uuid, fpr serialization.
      //! @attention can only be used once per `Category` 
      template< typename Category>
      const Category& registration( const Uuid& id)
      {
         static const Category category;
         lookup::detail::registration( { &category, id});
         return category;
      }

   } // common::code::serialize
} // casual