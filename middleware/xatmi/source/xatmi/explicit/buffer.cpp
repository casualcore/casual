//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "casual/xatmi/explicit.h"
#include "casual/xatmi/internal/code.h"

#include "common/buffer/pool.h"
#include "common/server/context.h"
#include "casual/platform.h"
#include "common/memory.h"


char* casual_buffer_allocate( const char* type, const char* subtype, long size)
{
   casual::xatmi::internal::error::clear();

   try
   {
      // TODO: Shall we report size less than zero ?
      return casual::common::buffer::pool::Holder::instance().allocate( type, subtype, size < 0 ? 0 : size);
   }
   catch( ...)
   {
      casual::xatmi::internal::error::set( casual::xatmi::internal::exception::code());
      return nullptr;
   }
}

char* casual_buffer_reallocate( const char* ptr, long size)
{
   casual::xatmi::internal::error::clear();

   try
   {
      // TODO: Shall we report size less than zero ?
      return casual::common::buffer::pool::Holder::instance().reallocate( ptr, size < 0 ? 0 : size);
   }
   catch( ...)
   {
      casual::xatmi::internal::error::set( casual::xatmi::internal::exception::code());
      return nullptr;
   }
}

long casual_buffer_type( const char* const ptr, char* const type, char* const subtype)
{
   casual::xatmi::internal::error::clear();

   try
   {
      auto buffer = casual::common::buffer::pool::Holder::instance().get( ptr);

      auto combined = casual::common::buffer::type::dismantle( buffer.payload().type);


      // type is optional
      if( type)
      {
         auto destination = casual::common::range::make( type, 8);
         casual::common::memory::clear( destination);
         casual::common::algorithm::copy_max( std::get< 0>( combined), destination);
      }

      // subtype is optional
      if( subtype)
      {
         auto destination = casual::common::range::make( subtype, 16);
         casual::common::memory::clear( destination);
         casual::common::algorithm::copy_max( std::get< 1>( combined), destination);
      }

      return buffer.reserved();
   }
   catch( ...)
   {
      casual::xatmi::internal::error::set( casual::xatmi::internal::exception::code());
      return -1;
   }

}

void casual_buffer_free( const char* const ptr)
{
   try
   {
      casual::common::buffer::pool::Holder::instance().deallocate( ptr);
   }
   catch( ...)
   {
      casual::xatmi::internal::error::set( casual::xatmi::internal::exception::code());
   }
}

