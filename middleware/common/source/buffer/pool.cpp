//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/buffer/pool.h"
#include "common/algorithm.h"
#include "common/log.h"

#include "common/code/raise.h"
#include "common/code/xatmi.h"
#include "common/exception/guard.h"

#include <functional>

namespace casual
{
   namespace common::buffer::pool
   {

      Holder::Concept* Holder::find_pool( std::string_view type)
      {
         if( auto found = algorithm::find_if( m_pools, manage_key( type)))
            return found->get();

         return nullptr;
      }

      Holder::Concept& Holder::get_pool( std::string_view type)
      {
         if( auto found = Holder::find_pool( type))
            return *found;

         code::raise::error( code::xatmi::buffer_input, "invalid buffer type: ", type);
      }

      Holder::Concept& Holder::get_pool( buffer::handle::type handle)
      {
         if( auto found = algorithm::find_if( m_pools, manage_buffer( handle)))
            return **found;

         code::raise::error( code::xatmi::argument, "buffer not valid - handle: ", handle);
      }

      const Payload& Holder::null_payload() const
      {
         static const Payload singleton{ nullptr};
         return singleton;
      }

      Holder& Holder::instance()
      {
         static Holder singleton;
         return singleton;
      }

      buffer::handle::mutate::type Holder::allocate( std::string_view type, platform::binary::size::type size)
      {
         log::line( log::category::buffer, "allocate: type: ", type, ", size: ", size);
         return get_pool( type).allocate( type, size);
      }

      buffer::handle::mutate::type Holder::reallocate( buffer::handle::type handle, platform::binary::size::type size)
      {
         auto buffer = get_pool( handle).reallocate( handle, size);
         log::line( log::category::buffer, "reallocate: buffer: ", buffer);

         return buffer;
      }

      const std::string& Holder::type( buffer::handle::type handle)
      {
         return get( handle).payload().type;
      }

      void Holder::deallocate( buffer::handle::type handle)
      {
         Trace trace{ "buffer::pool::deallocate"};
         log::line( log::category::buffer, "deallocate ", handle);

         if( handle)
            get_pool( handle).deallocate( handle);
      }

      buffer::handle::mutate::type Holder::adopt( Payload&& payload)
      {
         Trace trace{ "buffer::pool::Holder::adopt"};

         if( payload.null())
            return {};

         // This is the only place where a buffer is consumed by the pool, hence can only happen
         // during service-invocation.
         //
         // This is a 'special' buffer according to the XATMI-spec.
         return get_pool( payload.type).adopt( std::move( payload));
      }

      std::tuple< buffer::handle::mutate::type, platform::buffer::raw::size::type> Holder::insert( Payload&& payload)
      {
         Trace trace{ "buffer::pool::Holder::insert"};
         log::line( log::category::buffer, "insert payload: ", payload);

         if( payload.null())
            return { {}, {}};

         auto size = payload.data.size();
         auto& pool = get_pool( payload.type);
         return { pool.insert( std::move( payload)), size};
      }

      payload::Send Holder::get( buffer::handle::type handle, platform::binary::size::type user_size)
      {
         if( ! handle)
            return null_payload();

         return get_pool( handle).get( handle, user_size);
      }

      payload::Send Holder::get( buffer::handle::type handle)
      {
         if( ! handle)
            return null_payload();

         return get_pool( handle).get( handle);
      }


      Payload Holder::release( buffer::handle::type handle)
      {
         if( ! handle)
            return null_payload();

         auto result = get_pool( handle).release( handle);

         log::line( log::category::buffer, "released ", result);

         return result;
      }

      Payload Holder::release( buffer::handle::type handle, platform::binary::size::type size)
      {
         if( ! handle)
            return null_payload();

         auto result = get_pool( handle).release( handle, size);

         log::line( log::category::buffer, "released ", result);

         return result;
      }

      bool Holder::inbound( buffer::handle::type handle) const
      {
         if( auto found = algorithm::find_if( m_pools, manage_buffer( handle)))
            return (*found)->inbound( handle);
            
         return false;
      }


      void Holder::clear()
      {
         algorithm::for_each( m_pools, std::mem_fn( &Holder::Concept::clear));
      }

   } // casual.laz.se:80880/documentation/

} // casual
