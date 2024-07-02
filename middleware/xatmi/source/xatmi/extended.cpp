//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "casual/xatmi/extended.h"
#include "casual/xatmi/internal/log.h"

#include "common/instance.h"
#include "common/server/context.h"
#include "common/log/stream.h"

#include "common/execution/context.h"
#include "common/uuid.h"

#include <array>
#include <vector>

void casual_service_forward( const char* service, char* data, long size)
{
   casual::common::server::context().forward( service, data, size);
}

namespace local
{
   namespace
   {
      template< typename L>
      int vlog( L&& logger, const char* const format, va_list arglist)
      {
         std::array< char, 2048> buffer;
         std::vector< char> backup;

         va_list argcopy;
         va_copy( argcopy, arglist);
         auto written = vsnprintf( buffer.data(), buffer.max_size(), format, argcopy);
         va_end( argcopy );

         auto data = buffer.data();

         if( written >= static_cast< decltype( written)>( buffer.max_size()))
         {
            backup.resize( written + 1);
            va_copy( argcopy, arglist);
            written = vsnprintf( backup.data(), backup.size(), format, argcopy);
            va_end( argcopy );
            data = backup.data();
         }

         logger( data);

         return written;
      }

   } // <unnamed>
} // local

int casual_vlog( casual_log_category_t category, const char* const format, va_list arglist)
{

   auto catagory_logger = [=]( const char* data){

      switch( category)
      {
      case casual_log_category_t::c_log_debug:
         casual::common::log::stream::write( "debug", data);
         break;
      case casual_log_category_t::c_log_information:
         casual::common::log::stream::write( "information", data);
         break;
      case casual_log_category_t::c_log_warning:
         casual::common::log::stream::write( "warning", data);
         break;
      default:
         casual::common::log::stream::write( "error", data);
         break;
      }
   };

   return local::vlog( catagory_logger, format, arglist);
}

int casual_user_vlog( const char* category, const char* const format, va_list arglist)
{
   auto user_logger = [=]( const char* data){
      casual::common::log::stream::write( category, data);
   };

   return local::vlog( user_logger, format, arglist);
}

int casual_user_log( const char* category, const char* const message)
{
   casual::common::log::stream::write( category, message);

   return 0;
}

int casual_log( casual_log_category_t category, const char* const format, ...)
{
   va_list arglist;
   va_start( arglist, format );
   auto result = casual_vlog( category, format, arglist);
   va_end( arglist );

   return result;
}

const char* casual_instance_alias()
{
   if( auto& instance = casual::common::instance::information())
      return instance.value().alias.c_str();

   return nullptr;
}

long casual_instance_index()
{
   if( auto& instance = casual::common::instance::information())
      return instance.value().index;

   return -1;
}

const char* casual_execution_service_name()
{
   return casual::common::execution::context::get().service.c_str();
}

const char* casual_execution_parent_service_name()
{
   return casual::common::execution::context::get().parent.service.c_str();
}


void casual_execution_id_set( const uuid_t* id)
{
   casual::common::execution::context::id::set( casual::common::strong::execution::id( *id));
}

const uuid_t* casual_execution_id_get()
{
   return &casual::common::execution::context::get().id.underlying().get();
}

const uuid_t* casual_execution_id_reset()
{
   casual::common::execution::context::id::reset();
   return casual_execution_id_get();
}

void casual_instance_browse_services( casual_instance_browse_callback callback, void* context)
{
   using namespace casual;
   xatmi::Trace trace{ "casual_instance_browse_services"};

   const auto& services = common::server::context().state().services;

   common::algorithm::for_each_while( services, [callback, context]( auto& service)
   {
      casual_browsed_service state{ service.first.data()};
      return callback( &state, context) == 0;
   });
}
