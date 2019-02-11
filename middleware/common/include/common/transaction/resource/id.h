//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/strong/id.h"
#include "common/compare.h"
#include "common/marshal/marshal.h"

namespace casual
{
   namespace common
   {
      namespace transaction
      {
         namespace resource
         {
            struct ID : Compare< ID>
            {
               ID() = default;
               inline ID( strong::resource::id resource, strong::process::id process) : resource{ resource}, process{ process} {}

               strong::resource::id resource;
               strong::process::id process;

               CASUAL_CONST_CORRECT_MARSHAL({
                  CASUAL_MARSHAL( resource);
                  CASUAL_MARSHAL( process);
               })

               //! for Compare
               inline auto tie() const noexcept { return std::tie( resource, process);}

               inline friend std::ostream& operator << ( std::ostream& out, ID value)
               {
                  return out << "{ resource: " << value.resource << ", process: " << value.process << '}';
               }
            };
         } // resource
      } // transaction
   } // common
} // casual