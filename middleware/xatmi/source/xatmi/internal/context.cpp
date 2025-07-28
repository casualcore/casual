//!
//! Copyright (c) 2025, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "casual/xatmi/internal/context.h"

#include "common/buffer/pool.h"

namespace casual
{
   namespace xatmi::internal
   {
      namespace header
      {
 
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

      } // header

      namespace state
      {
         std::ostream& operator << ( std::ostream& out, const Jump& value)
         {
            return common::stream::write( out, "{ value: ", value.state.value,
               ", code: ", value.state.code,
               ", data: ", value.buffer.data,
               ", size: ", value.buffer.size,
               ", service: ", value.forward.service, '}');
         }
      } // state

       void Context::jump_return( common::flag::xatmi::Return rval, long rcode, char* data, long len)
      {
         // Prepare buffer.
         // We have to keep state, since there seems not to be any way to send information
         // via longjump...

         m_state.jump.state.value = rval;
         m_state.jump.state.code = rcode;
         m_state.jump.buffer.data = common::buffer::handle::type{ data};
         m_state.jump.buffer.size = len;
         m_state.jump.forward.service.clear();

         common::log::debug( "Context::jump_return - jump state: ", m_state.jump);

         std::longjmp( m_state.jump.environment, state::Jump::Location::c_return);
      }

      void Context::normal_return( common::flag::xatmi::Return rval, long rcode, char* data, long len)
      {
         // Prepare buffer.
         // Essentially the same as jump_return above, but instead of a longjmp
         // this variant returns to the caller. Used by the COBOL api TPRETURN
         // function that is expected to return to its caller, that ultimately
         // returns to the "communications manager" (Casual) without bypassing 
         // the COBOL runtime. 

         m_state.jump.state.value = rval;
         m_state.jump.state.code = rcode;
         m_state.jump.buffer.data = common::buffer::handle::type{ data};
         m_state.jump.buffer.size = len;
         m_state.jump.forward.service.clear();

         m_state.TPRETURN_called = true;

         common::log::debug( "Context::normal_return - jump state: ", m_state.jump);
      }


      void Context::forward( const char* service, char* data, long size)
      {
         m_state.jump.state.value = common::flag::xatmi::Return::success;
         m_state.jump.state.code = 0;
         m_state.jump.buffer.data = common::buffer::handle::type{ data};
         m_state.jump.buffer.size = size;

         m_state.jump.forward.service = service ? service : "";

         common::log::debug( "Context::forward - jump state: ", m_state.jump);

         std::longjmp( m_state.jump.environment, state::Jump::Location::c_forward);
      }

      void Context::finalize()
      {
         m_header.clear();
         m_state = {};
      }

      //! @returns the current header context
      Context& Context::instance()
      {
         static Context instance;
         return instance;
      }

   } // xatmi::internal
   
} // casual
