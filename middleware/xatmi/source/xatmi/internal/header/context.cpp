//!
//! Copyright (c) 2025, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "casual/xatmi/internal/header/context.h"

#include "common/buffer/pool.h"

namespace casual
{
   namespace xatmi::internal::header
   {
      //! @returns the current header context
      Context& Context::instance()
      {
         static Context instance;
         return instance;
      }

      void Context::associate( common::buffer::handle::type handle, casual::header::Fields fields)
      {
         common::Trace trace{ "header::Context::associate"};

         if( !common::buffer::pool::holder().contains( handle))
            common::code::raise::error( common::code::xatmi::argument, "invalid buffer handle: ", handle);

         if( auto found = common::algorithm::find( m_fields, handle))
            found->fields = std::move( fields);
         else
            m_fields.push_back( { handle, std::move( fields)});
      }

      void Context::disassociate( common::buffer::handle::type handle) noexcept
      {
         if( auto found = common::algorithm::find( m_fields, handle))
            m_fields.erase( std::begin( found));
      }

      const casual::header::Fields* Context::find( common::buffer::handle::type handle) noexcept
      {
         if( auto found = common::algorithm::find( m_fields, handle))
            return &found->fields;

         return nullptr;
      }


      void Context::update_handle( common::buffer::handle::type old_handle, common::buffer::handle::type new_handle) noexcept
      {
         if( old_handle == new_handle)
            return;

         if( auto found = common::algorithm::find( m_fields, old_handle))
            found->handle = new_handle;
      }

      void Context::clear() noexcept
      {
         m_fields.clear();
      }

   } // xatmi::internal::header
   
} // casual
