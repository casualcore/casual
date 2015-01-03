//
// casual_order_buffer.cpp
//
//  Created on: 20 okt 2013
//      Author: Kristone
//

#include "buffer/order.h"

#include "common/buffer/pool.h"
#include "common/network_byteorder.h"

#include "common/log.h"

#include <cstring>







namespace local
{
   namespace
   {

      namespace arithmetic
      {
         template<typename T>
         inline void write( char* const where, const T& value)
         {
            const auto encoded = casual::common::network::byteorder<T>::encode( value);
            std::memcpy( where, &encoded, sizeof( encoded));
         }

         template<typename T>
         inline void parse( const char* const where, T& value)
         {
            value = casual::common::network::byteorder<T>::decode( *reinterpret_cast< const casual::common::network::type<T>*>( where));
         }

         template< typename T>
         constexpr long bytes()
         {
            return casual::common::network::bytes<T>();
         }

      }


      namespace header
      {
         enum position : long
         {
            reserved, inserter, selector, beoyond
         };

         constexpr auto size() -> decltype(arithmetic::bytes<long>())
         {
            return arithmetic::bytes< long>() * position::beoyond;
         }

         namespace select
         {
            template< position offset>
            inline long parse( const char* const buffer)
            {
               const char* const where = (buffer + arithmetic::bytes<long>() * offset);
               const auto encoded = *reinterpret_cast< const casual::common::network::type<long>*>( where);
               return casual::common::network::byteorder<long>::decode( encoded);
            }

            long reserved( const char* const buffer)
            {
               return parse< position::reserved>( buffer);
            }

            long inserter( const char* const buffer)
            {
               return parse< position::inserter>( buffer);
            }

            long selector( const char* const buffer)
            {
               return parse< position::selector>( buffer);
            }
         }

         namespace update
         {
            template< position offset>
            inline void write( char* const buffer, const long value)
            {
               const auto encoded = casual::common::network::byteorder<long>::encode( value);
               char* const where = buffer + arithmetic::bytes< long>() * offset;
               std::memcpy( where, &encoded, sizeof( encoded));
            }

            void reserved( char* const buffer, const long value)
            {
               write< position::reserved>( buffer, value);
            }

            void inserter( char* const buffer, const long value)
            {
               write< position::inserter>( buffer, value);
            }

            void selector( char* const buffer, const long value)
            {
               write< position::selector>( buffer, value);
            }

         }
      }


      namespace add
      {

         //
         // TODO: make pod+string+binary a bit more generic
         //

         template<typename T>
         int pod( char* const buffer, const T& value)
         {
            const auto reserved = header::select::reserved( buffer);
            const auto inserter = header::select::inserter( buffer);

            constexpr auto length = arithmetic::bytes<T>();

            if( (reserved - inserter) < length)
            {
               return CASUAL_ORDER_NO_SPACE;
            }

            arithmetic::write( buffer + inserter, value);

            header::update::inserter( buffer, inserter + length);

            return CASUAL_ORDER_SUCCESS;
         }

         int string( char* const buffer, const char* const value)
         {
            const auto reserved = header::select::reserved( buffer);
            const auto inserter = header::select::inserter( buffer);

            const long length = std::strlen( value) + 1;

            if( (reserved - inserter) < length)
            {
               return CASUAL_ORDER_NO_SPACE;
            }

            std::memcpy( buffer + inserter, value, length);

            header::update::inserter( buffer, inserter + length);

            return CASUAL_ORDER_SUCCESS;
         }

         int binary( char* const buffer, const char* const data, const long size)
         {
            const auto reserved = header::select::reserved( buffer);
            const auto inserter = header::select::inserter( buffer);

            constexpr auto bytes = arithmetic::bytes<long>();
            const auto length = bytes + size;

            if( (reserved - inserter) < length)
            {
               return CASUAL_ORDER_NO_SPACE;
            }

            arithmetic::write( buffer + inserter, size);

            std::memcpy( buffer + inserter + bytes, data, size);

            header::update::inserter( buffer, inserter + length);

            return CASUAL_ORDER_SUCCESS;
         }

      }

      namespace get
      {

         //
         // TODO: make pod+string+binary a bit more generic
         //

         template<typename T>
         int pod( char* const buffer, T& value)
         {
            const auto inserter = header::select::inserter( buffer);
            const auto selector = header::select::selector( buffer);

            arithmetic::parse( buffer + selector, value);

            constexpr auto length = arithmetic::bytes<T>();

            if( (inserter - selector) < length)
            {
               // the buffer is abused and we need to report it
               return CASUAL_ORDER_NO_PLACE;
            }

            header::update::selector( buffer, selector + length);

            return CASUAL_ORDER_SUCCESS;
         }

         int string( char* const buffer, const char*& value)
         {
            const auto inserter = header::select::inserter( buffer);
            const auto selector = header::select::selector( buffer);

            value = buffer + selector;

            const long length = std::strlen( value) + 1;

            if( (inserter - selector) < length)
            {
               // the buffer is abused and we need to report it
               return CASUAL_ORDER_NO_PLACE;
            }

            header::update::selector( buffer, selector + length);

            return CASUAL_ORDER_SUCCESS;
         }


         int binary( char* const buffer, const char*& data, long& size)
         {
            const auto inserter = header::select::inserter( buffer);
            const auto selector = header::select::selector( buffer);

            constexpr auto bytes = arithmetic::bytes<long>();

            arithmetic::parse( buffer + selector, size);
            data = buffer + selector + bytes;

            const auto length = bytes + size;

            if( (inserter - selector) < length)
            {
               // the buffer is abused and we need to report it
               return CASUAL_ORDER_NO_PLACE;
            }

            header::update::selector( buffer, selector + length);

            return CASUAL_ORDER_SUCCESS;
         }

      }

   } // <unamed>
} // local


const char* CasualOrderDescription( const int code)
{
   switch( code)
   {
      case CASUAL_ORDER_SUCCESS:
         return "Success";
      case CASUAL_ORDER_NO_SPACE:
         return "No space";
      case CASUAL_ORDER_NO_PLACE:
         return "No place";
      case CASUAL_ORDER_INTERNAL_FAILURE:
         return "Internal failure";
      default:
         return "Uknown code";
   }
}

int CasualOrderAddPrepare( char* const buffer)
{
   local::header::update::inserter( buffer, local::header::size());

   return CASUAL_ORDER_SUCCESS;
}

int CasualOrderAddBool( char* const buffer, const bool value)
{
   return local::add::pod( buffer, value);
}

int CasualOrderAddChar( char* const buffer, const char value)
{
   return local::add::pod( buffer, value);
}

int CasualOrderAddShort( char* const buffer, const short value)
{
   return local::add::pod( buffer, value);
}

int CasualOrderAddLong( char* const buffer, const long value)
{
   return local::add::pod( buffer, value);
}

int CasualOrderAddFloat( char* const buffer, const float value)
{
   return local::add::pod( buffer, value);
}

int CasualOrderAddDouble( char* const buffer, const double value)
{
   return local::add::pod( buffer, value);
}

int CasualOrderAddString( char* const buffer, const char* const value)
{
   return local::add::string( buffer, value);
}

int CasualOrderAddBinary( char* const buffer, const char* const data, const long size)
{
   return local::add::binary( buffer, data, size);
}

int CasualOrderGetPrepare( char* const buffer)
{
   local::header::update::selector( buffer, local::header::size());

   return CASUAL_ORDER_SUCCESS;
}

int CasualOrderGetBool( char* const buffer, bool* const value)
{
   return local::get::pod( buffer, *value);
}

int CasualOrderGetChar( char* const buffer, char* const value)
{
   return local::get::pod( buffer, *value);
}

int CasualOrderGetShort( char* const buffer, short* const value)
{
   return local::get::pod( buffer, *value);
}

int CasualOrderGetLong( char* const buffer, long* const value)
{
   return local::get::pod( buffer, *value);
}

int CasualOrderGetFloat( char* const buffer, float* const value)
{
   return local::get::pod( buffer, *value);
}

int CasualOrderGetDouble( char* const buffer, double* const value)
{
   return local::get::pod( buffer, *value);
}

int CasualOrderGetString( char* const buffer, const char** value)
{
   return local::get::string( buffer, *value);
}

int CasualOrderGetBinary( char* const buffer, const char** data, long* const size)
{
   return local::get::binary( buffer, *data, *size);
}

int CasualOrderCopyBuffer( char* const target, const char* const source)
{
   const auto target_reserved = local::header::select::reserved( target);
   const auto source_inserter = local::header::select::inserter( source);

   if( target_reserved < source_inserter)
   {
      return CASUAL_ORDER_NO_SPACE;
   }

   constexpr auto bytes = local::arithmetic::bytes< long>();

   // copy inserter and selector (and data) but leave reserved
   std::memcpy( target + bytes, source + bytes, source_inserter - bytes);

   return CASUAL_ORDER_SUCCESS;
}

int CasualOrderUsed( const char* const buffer, long* const size)
{
   *size = local::header::select::inserter( buffer);
   return CASUAL_ORDER_SUCCESS;
}

/*
long CasualOrderCreate( char* const buffer, const long size)
{
   constexpr auto bytes = local::header::size();

   if( size < bytes)
   {
      return CASUAL_ORDER_NO_SPACE;
   }

   local::header::update::reserved( buffer, size);
   local::header::update::inserter( buffer, bytes);
   local::header::update::selector( buffer, bytes);

   return CASUAL_ORDER_SUCCESS;
}

long CasualOrderExpand( char* const buffer, const long size)
{
   local::header::update::reserved( buffer, size);

   return CASUAL_ORDER_SUCCESS;
}

long CasualOrderReduce( char* const buffer, const long size)
{
   const auto inserter = local::header::select::inserter( buffer);

   if( size < inserter)
   {
      return CASUAL_ORDER_NO_SPACE;
   }

   local::header::update::reserved( buffer, size);

   return CASUAL_ORDER_SUCCESS;
}
*/

//  TODO: Don't understand what this is...
long CasualOrderNeeded( char* const buffer, const long size)
{
   return local::header::select::inserter( buffer);
}


namespace casual
{
   namespace buffer
   {
      namespace implementation
      {

         struct OrderBuffer : public common::buffer::Buffer
         {
            // has to have the two ctors like common::buffer::Buffer
            using common::buffer::Buffer::Buffer;

            // TODO: some offset stuff...
         };


         class Order : public common::buffer::pool::basic_pool< OrderBuffer>
         {
         public:

            using types_type = common::buffer::pool::default_pool::types_type;

            static const types_type& types()
            {
               static const types_type result{ { CASUAL_ORDER, ""}};
               return result;
            }

            common::platform::raw_buffer_type allocate( const common::buffer::Type& type, std::size_t size)
            {
               // TODO: this should not be needed if we're using custom buffer-type in the generic pool


               constexpr auto bytes = local::header::size();

               if( size < bytes)
               {
                  // throw if this is an error...
                  size = bytes;
               }

               OrderBuffer buffer( type, size);

               local::header::update::reserved( buffer.payload.memory.data(), size);
               local::header::update::inserter( buffer.payload.memory.data(), bytes);
               local::header::update::selector( buffer.payload.memory.data(), bytes);

               m_pool.push_back( std::move( buffer));
               return m_pool.back().payload.memory.data();

            }

            common::platform::raw_buffer_type reallocate( common::platform::const_raw_buffer_type handle, std::size_t size)
            {
               // TODO: this should not be needed if we're using custom buffer-type in the generic pool

               auto buffer = find( handle);

               if( buffer != std::end( m_pool))
               {
                  buffer->payload.memory.resize( size);
                  local::header::update::reserved( buffer->payload.memory.data(), size);
                  return buffer->payload.memory.data();
               }

               return nullptr;
            }
         };

      } // implementation


   } // buffer

   //
   // Registrate and define the type that can be used to get the custom pool
   //

   template class common::buffer::pool::Registration< buffer::implementation::Order>;

   namespace buffer
   {
      namespace order
      {
         using pool_type = common::buffer::pool::Registration< implementation::Order>;

         //
         // Ex:
         //
         int some_C_function( char* handle)
         {
            try
            {
               //
               // get the buffer for this handle
               //
               implementation::OrderBuffer& buffer = pool_type::pool.get( handle);

               //
               // Do stuff with custom buffer
               //
               common::log::debug << buffer.payload.type.subtype << std::endl;

               return 0;
            }
            catch( ...)
            {
               // sets tperror and write to log it there are severe errors,
               return common::error::handler();
            }
         }

      } // order
   } // buffer
} // casual



