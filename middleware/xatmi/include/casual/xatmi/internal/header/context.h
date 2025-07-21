//!
//! Copyright (c) 2025, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "casual/header.h"

namespace casual
{
   namespace xatmi::internal::header
   {
      //! @attention Only used for XATMI.
      struct Context
      {
         //! @returns the current header context
         static Context& instance();

         void associate( common::buffer::handle::type handle, casual::header::Fields fields);
         void disassociate( common::buffer::handle::type handle) noexcept;

         const casual::header::Fields* find( common::buffer::handle::type handle) noexcept;

         //! find the `old_handle` in the context and update it to `new_handle`
         //! used when a buffer is reallocated, to keep the association
         void update_handle( common::buffer::handle::type old_handle, common::buffer::handle::type new_handle) noexcept;
         void clear() noexcept;

      private:
         Context() = default;
         
         struct Holder
         {
            common::buffer::handle::type handle;
            casual::header::Fields fields;

            friend bool operator == ( const Holder& lhs, common::buffer::handle::type rhs) { return lhs.handle == rhs; }
         };

         std::vector< Holder> m_fields;
      };

      //! @attention Only used for XATMI.
      inline Context& context() { return Context::instance();}

   } // xatmi::internal::header
   
} // casual
