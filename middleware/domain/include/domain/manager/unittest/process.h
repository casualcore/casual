//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/process.h"
#include "common/pimpl.h"

#include <vector>
#include <string>

namespace casual
{
   namespace domain
   {
      namespace manager
      {
         namespace unittest
         {
            struct Process 
            {
               Process();
               Process( const std::vector< std::string>& configuration);
               ~Process();

               const common::process::Handle& handle() const noexcept;

               friend std::ostream& operator << ( std::ostream& out, const Process& value);

            private:
               struct Implementation;
               common::move::basic_pimpl< Implementation> m_implementation;
            };
         } // unittest
      } // manager
   } // domain
} // casual