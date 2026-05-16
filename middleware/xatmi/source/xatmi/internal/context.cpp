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
   
      namespace descriptor
      {
         Context::Context()
         {
            m_correlations.reserve( 4);
         }

         platform::descriptor::type Context::map( const common::strong::correlation::id& correlation)
         {
            auto calculate_descriptor = [ &]( auto found)
            {
               auto index = std::distance( std::begin( m_correlations), std::begin( found));
               return index + 1;
            };

            if( auto found = common::algorithm::find( m_correlations, correlation))
               return calculate_descriptor( found);


            auto is_invalid = []( auto& correlation){ return ! correlation.valid();};

            if( auto found = common::algorithm::find_if( m_correlations, is_invalid))
            {
               *found = correlation;
               return calculate_descriptor( found);
            }

            m_correlations.push_back( correlation);
            return std::ssize( m_correlations);
         }

         const common::strong::correlation::id& Context::map( platform::descriptor::type descriptor) const
         {
            return m_correlations[ Context::index( descriptor)];
         }

         platform::descriptor::type Context::extract( const common::strong::correlation::id& correlation)
         {
            if( auto found = common::algorithm::find( m_correlations, correlation))
            {
               *found = {};
               auto index = std::distance( std::begin( m_correlations), std::begin( found));
               return index + 1;
            }

            return -1;
         }

         void Context::remove( platform::descriptor::type descriptor)
         {
            m_correlations[ Context::index( descriptor)] = {};
         }

         void Context::clear()
         {
            m_correlations.clear();
         }

         bool Context::empty() const
         {
            return common::algorithm::all_of( m_correlations, []( auto& correlation){ return ! correlation.valid();});
         }

         std::size_t Context::index( platform::descriptor::type descriptor) const
         {
            if( descriptor <= 0)
               common::code::raise::error( common::code::xatmi::argument, "invalid descriptor: ", descriptor);

            std::size_t index = descriptor -1;

            if( index >= std::size( m_correlations))
               common::code::raise::error( common::code::xatmi::descriptor, "invalid descriptor: ", descriptor);

            return index;
         }
         
      } // descriptor

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

         state.jump.state.value = rval;
         state.jump.state.code = rcode;
         state.jump.buffer.data = common::buffer::handle::type{ data};
         state.jump.buffer.size = len;
         state.jump.forward.service.clear();

         common::log::debug( "Context::jump_return - jump state: ", state.jump);

         std::longjmp( state.jump.environment, state::Jump::Location::c_return);
      }

      void Context::normal_return( common::flag::xatmi::Return rval, long rcode, char* data, long len)
      {
         // Prepare buffer.
         // Essentially the same as jump_return above, but instead of a longjmp
         // this variant returns to the caller. Used by the COBOL api TPRETURN
         // function that is expected to return to its caller, that ultimately
         // returns to the "communications manager" (Casual) without bypassing 
         // the COBOL runtime. 

         state.jump.state.value = rval;
         state.jump.state.code = rcode;
         state.jump.buffer.data = common::buffer::handle::type{ data};
         state.jump.buffer.size = len;
         state.jump.forward.service.clear();

         state.TPRETURN_called = true;

         common::log::debug( "Context::normal_return - jump state: ", state.jump);
      }


      void Context::forward( const char* service, char* data, long size)
      {
         state.jump.state.value = common::flag::xatmi::Return::success;
         state.jump.state.code = 0;
         state.jump.buffer.data = common::buffer::handle::type{ data};
         state.jump.buffer.size = size;

         state.jump.forward.service = service ? service : "";

         common::log::debug( "Context::forward - jump state: ", state.jump);

         std::longjmp( state.jump.environment, state::Jump::Location::c_forward);
      }

      void Context::finalize()
      {
         common::Trace trace{ "xatmi::internal::Context::finalize"};
         common::log::debug( "context: ", *this);

         descriptor.clear();
         state = {};
      }

      //! @returns the current header context
      Context& Context::instance()
      {
         static Context instance;
         return instance;
      }

   } // xatmi::internal
   
} // casual
