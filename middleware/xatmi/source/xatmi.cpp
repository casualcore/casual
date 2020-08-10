//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "casual/xatmi.h"
#include "casual/tx.h"

#include "casual/xatmi/internal/code.h"
#include "casual/xatmi/explicit.h"

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
   return casual_buffer_allocate( type, subtype, size);
}

char* tprealloc( const char* ptr, long size)
{
   return casual_buffer_reallocate( ptr, size);
}

long tptypes( const char* buffer, char* type, char* subtype)
{
   return casual_buffer_type( buffer, type, subtype);
}

void tpfree( const char* ptr)
{
   casual_buffer_free( ptr);
}

void tpreturn( int rval, long rcode, char* data, long len, long flags)
{
   casual_service_return( rval, rcode, data, len, flags);
}

int tpadvertise( const char* service, void (*function)( TPSVCINFO *))
{
   return casual_service_advertise( service, function);
}

int tpunadvertise( const char* const service)
{
   return casual_service_unadvertise( service);
}

int tpcall( const char* const service, char* idata, const long ilen, char** odata, long* olen, const long bitmap)
{
   return casual_service_call( service, idata, ilen, odata, olen, bitmap);
}

int tpacall( const char* const service, char* idata, const long ilen, const long flags)
{
   return casual_service_asynchronous_send( service, idata, ilen, flags);
}

int tpgetrply( int *const descriptor, char** odata, long* olen, const long bitmap)
{
   return casual_service_asynchronous_receive( descriptor, odata, olen, bitmap);
}

int tpcancel( int id)
{
   return casual_service_asynchronous_cancel( id);
}



const char* tperrnostring( int error)
{
   return casual::common::code::description( static_cast< casual::common::code::xatmi>( error));
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

