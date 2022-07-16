//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/process.h"
#include "common/pimpl.h"
#include "common/algorithm/container.h"


#include <vector>
#include <string>

namespace casual
{
   namespace domain::unittest
   {
      struct Manager 
      {
         Manager( std::vector< std::string_view> configuration);
         ~Manager();

         Manager( Manager&&) noexcept;
         Manager& operator = ( Manager&&) noexcept;

         const common::process::Handle& handle() const noexcept;
         
         //! tries to "activate" the domain, i.e. resets environment variables and such
         //! only useful if more than one instance is used
         void activate();

         friend std::ostream& operator << ( std::ostream& out, const Manager& value);

      private:
         struct Implementation;
         common::move::Pimpl< Implementation> m_implementation;
      };

      template< typename... Cs>
      [[nodiscard]] auto manager( Cs&&... configurations)
      {
         return Manager{ common::algorithm::container::emplace::initialize< std::vector< std::string_view>>( std::forward< Cs>( configurations)...)};
      }

   } // domain::unittest
} // casual