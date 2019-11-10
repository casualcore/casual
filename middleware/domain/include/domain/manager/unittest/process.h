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

               //! callback to be able to enable other _environment_ stuff before boot
               //! @attention `callback` has to be idempotent (if activate is used)
               Process( const std::vector< std::string>& configuration, std::function< void( const std::string&)> callback);
               ~Process();

               Process( Process&&);
               Process& operator = ( Process&&);

               const common::process::Handle& handle() const noexcept;
               
               //! tries to "activate" the domain, i.e. resets environment variables and such
               //! only usefull if more than one instance is used
               void activate();

               friend std::ostream& operator << ( std::ostream& out, const Process& value);

            private:
               struct Implementation;
               common::move::basic_pimpl< Implementation> m_implementation;
            };

            namespace process
            {
               //! Waits for the domain manager to boot
               common::process::Handle wait();
            } // process

         } // unittest
      } // manager
   } // domain
} // casual