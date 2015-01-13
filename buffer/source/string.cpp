//!
//! string.cpp
//!
//! Created on: Dec 26, 2013
//!     Author: Lazan
//!

#include "buffer/string.h"

#include "common/buffer/pool.h"

#include <cstring>

namespace casual
{
   namespace buffer
   {
      namespace string
      {

         namespace
         {

            // TODO: Doesn't have to inherit from anything, but has to implement a set of functions (like pool::basic)
            class Allocator : public common::buffer::pool::default_pool
            {
            public:

               using types_type = common::buffer::pool::default_pool::types_type;

               static const types_type& types()
               {
                  // The types this pool can manage
                  static const types_type result{{ CASUAL_STRING, "" }};
                  return result;
               }

               common::platform::raw_buffer_type allocate( const common::buffer::Type& type, const std::size_t size)
               {
                  m_pool.emplace_back( type, size > 0 ? size : 1);

                  m_pool.back().payload.memory.front() = '\0';

                  return m_pool.back().payload.memory.data();
               }

               common::platform::raw_buffer_type reallocate( const common::platform::const_raw_buffer_type handle, const std::size_t size)
               {
                  const auto result = find( handle);

                  if( result == std::end( m_pool))
                  {
                     // TODO: shouldn't this be an error (exception) ?
                     return nullptr;
                  }

                  //const auto count = std::strlen( result->payload.memory.data()) + 1;
                  //result->payload.memory.resize( size > count ? size : count);

                  result->payload.memory.resize( size > 0 ? size : 1);

                  result->payload.memory.back() = '\0';

                  return result->payload.memory.data();
               }
            };

         } //

      } // string

   } // buffer


   //
   // Register and define the type that can be used to get the custom pool
   //
   template class common::buffer::pool::Registration< buffer::string::Allocator>;

   namespace buffer
   {
      namespace string
      {
         using pool_type = common::buffer::pool::Registration< Allocator>;

         namespace
         {
            typedef common::platform::raw_buffer_type data_type;
            typedef common::platform::raw_buffer_size size_type;


            class Buffer
            {
            public:

               Buffer( data_type data, size_type size) : m_data( data), m_size( size) {}

               explicit operator bool() const
               {
                  return m_data != nullptr;
               }

               const data_type data() const
               {return m_data;}

               data_type data()
               {return m_data;}

               size_type size() const
               {return m_size;}

            private:
               data_type m_data;
               size_type m_size;

            };

            Buffer find_buffer( const char* const handle)
            {
               try
               {
                  auto& buffer = pool_type::pool.get( handle);

                  if( buffer.payload.type.type != CASUAL_STRING)
                  {
                     //
                     // TODO: This should be some generic check
                     //
                     // TODO: Shall this be logged ?
                     //
                  }
                  else
                  {
                     return Buffer( buffer.payload.memory.data(), buffer.payload.memory.size());
                  }

               }
               catch( ...)
               {
                  //
                  // TODO: Perhaps have some dedicated string-logging ?
                  //
                  common::error::handler();
               }

               return Buffer( nullptr, 0);

            }
         }

      } // string

   } // buffer

} // casual

const char* CasualStringDescription( const int code)
{

   switch( code)
   {
      case CASUAL_STRING_SUCCESS:
         return "Success";
      case CASUAL_STRING_NO_SPACE:
         return "No space";
      case CASUAL_STRING_NO_PLACE:
         return "No place";
      case CASUAL_STRING_INVALID_BUFFER:
         return "Invalid buffer";
      case CASUAL_STRING_INVALID_ARGUMENT:
         return "Invalid argument";
      case CASUAL_STRING_INTERNAL_FAILURE:
         return "Internal failure";
      default:
         return "Unknown code";
   }

}

int CasualStringExploreBuffer( const char* const handle, long* const size, long* const used)
{
   const auto buffer = casual::buffer::string::find_buffer( handle);

   if( buffer)
   {
      const auto reserved = buffer.size();
      const auto utilized = std::strlen( buffer.data()) + 1;

      if( size) *size = reserved;
      if( used) *used = utilized;

      if( utilized > reserved)
      {
         // We need to report this
         return CASUAL_STRING_NO_PLACE;
      }

   }
   else
   {
      return CASUAL_STRING_INVALID_BUFFER;
   }

   return CASUAL_STRING_SUCCESS;
}

int CasualStringWriteString( char* const handle, const char* const value)
{
   auto buffer = casual::buffer::string::find_buffer( handle);

   if( buffer)
   {
      if( value)
      {
         const auto count = std::strlen( value) + 1;

         if( count > buffer.size())
         {
            return CASUAL_STRING_NO_SPACE;
         }
         else
         {
            std::memcpy( buffer.data(), value, count);
         }

      }
      else
      {
         return CASUAL_STRING_INVALID_ARGUMENT;
      }

   }
   else
   {
      return CASUAL_STRING_INVALID_BUFFER;
   }

   return CASUAL_STRING_SUCCESS;

}

int CasualStringParseString( const char* handle, const char** value)
{
   const auto buffer = casual::buffer::string::find_buffer( handle);

   if( buffer)
   {
      if( value)
      {
         const auto count = std::strlen( buffer.data()) + 1;

         if( count > buffer.size())
         {
            return CASUAL_STRING_NO_PLACE;
         }
         else
         {
            *value = buffer.data();
         }

      }
      else
      {
         return CASUAL_STRING_INVALID_ARGUMENT;
      }

   }
   else
   {
      return CASUAL_STRING_INVALID_BUFFER;
   }

   return CASUAL_STRING_SUCCESS;

}

