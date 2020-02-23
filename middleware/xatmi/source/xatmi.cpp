//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "casual/xatmi.h"
#include "casual/tx.h"

#include "casual/xatmi/internal/code.h"

#include "common/buffer/pool.h"
#include "common/server/context.h"
#include "casual/platform.h"
#include "common/log.h"
#include "common/memory.h"

#include "common/string.h"
#include "common/execution.h"
#include "common/uuid.h"

#include <array>
#include <cstdarg>




int casual_get_tperrno()
{
   return static_cast< int>( casual::xatmi::internal::error::get());
}

long casual_get_tpurcode()
{
   return casual::xatmi::internal::user::code::get();
}



char* tpalloc( const char* type, const char* subtype, long size)
{
   casual::xatmi::internal::error::clear();

   try
   {
      // TODO: Shall we report size less than zero ?
      return casual::common::buffer::pool::Holder::instance().allocate( type, subtype, size < 0 ? 0 : size);
   }
   catch( ...)
   {
      casual::xatmi::internal::error::set( casual::common::exception::xatmi::handle());
      return nullptr;
   }
}

char* tprealloc( const char* ptr, long size)
{
   casual::xatmi::internal::error::clear();

   try
   {
      // TODO: Shall we report size less than zero ?
      return casual::common::buffer::pool::Holder::instance().reallocate( ptr, size < 0 ? 0 : size);
   }
   catch( ...)
   {
      casual::xatmi::internal::error::set( casual::common::exception::xatmi::handle());
      return nullptr;
   }

}


long tptypes( const char* const ptr, char* const type, char* const subtype)
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
      casual::xatmi::internal::error::set( casual::common::exception::xatmi::handle());
      return -1;
   }

}

void tpfree( const char* const ptr)
{
   try
   {
      casual::common::buffer::pool::Holder::instance().deallocate( ptr);
   }
   catch( ...)
   {
      casual::xatmi::internal::error::set( casual::common::exception::xatmi::handle());
   }
}

void tpreturn( const int rval, const long rcode, char* const data, const long len, const long /* flags for future use */)
{
   casual::xatmi::internal::error::wrap( [&](){
      casual::common::server::context().jump_return( 
         static_cast< casual::common::flag::xatmi::Return>( rval), rcode, data, len);
   });
}






int tpadvertise( const char* service, void (*function)( TPSVCINFO *))
{
   return casual::xatmi::internal::error::wrap( [&](){
      casual::common::server::context().advertise( service, function);
   });
}

int tpunadvertise( const char* const service)
{
   return casual::xatmi::internal::error::wrap( [&](){
      casual::common::server::context().unadvertise( service);
   });
}





const char* tperrnostring( int error)
{
   return casual::common::code::message( static_cast< casual::common::code::xatmi>( error));
}

int tpsvrinit( int argc, char **argv)
{
   casual::xatmi::internal::error::clear();
   return tx_open() == TX_OK ? 0 : -1;
}

void tpsvrdone()
{
   casual::xatmi::internal::error::clear();
   tx_close();
}

void casual_execution_id_set( const uuid_t* id)
{
   casual::common::execution::id( casual::common::Uuid( *id));
}

const uuid_t* casual_execution_id_get()
{
   return &casual::common::execution::id().get();
}

