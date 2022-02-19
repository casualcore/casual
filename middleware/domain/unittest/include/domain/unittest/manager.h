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
   namespace domain::unittest
   {
      struct Manager 
      {
         Manager();
         Manager( std::vector< std::string_view> configuration);

         //! callback to be able to enable other _environment_ stuff before boot
         //! @attention `callback` has to be idempotent (if activate is used)
         Manager( std::vector< std::string_view> configuration, std::function< void( const std::string&)> callback);
         ~Manager();

         Manager( Manager&&) noexcept;
         Manager& operator = ( Manager&&) noexcept;

         const common::process::Handle& handle() const noexcept;
         
         //! tries to "activate" the domain, i.e. resets environment variables and such
         //! only usefull if more than one instance is used
         void activate();

         friend std::ostream& operator << ( std::ostream& out, const Manager& value);

      private:
         struct Implementation;
         common::move::basic_pimpl< Implementation> m_implementation;
      };

      template< typename... C>
      [[nodiscard]] auto manager( C&&... configurations)
      {
         std::vector< std::string_view> views;
         ( views.emplace_back( std::forward< C>( configurations)) , ... );

         return domain::unittest::Manager{ std::move( views)};
      }

   } // domain::unittest
} // casual