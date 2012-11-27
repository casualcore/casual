//!
//! casual_buffer.cpp
//!
//! Created on: Apr 29, 2012
//!     Author: Lazan
//!

#include "common/buffer_context.h"

#include "utility/exception.h"
#include "utility/logger.h"

#include <stdexcept>

#include <algorithm>

namespace casual
{
   namespace common
   {


      namespace buffer
      {

         namespace local
         {
            struct FindBuffer
            {
               FindBuffer( const void* toFind) : m_toFind( toFind) {}

               bool operator () ( Buffer& buffer) const
               {
                  return m_toFind == buffer.raw();
               }

               const void* m_toFind;
            };


         }



         Context::Context()
         {

         }

         Context& Context::instance()
         {
            static Context singleton;
            return singleton;
         }

         common::raw_buffer_type Context::allocate(const std::string& type, const std::string& subtype, std::size_t size)
         {

            m_memoryPool.emplace_back( type, subtype, size);

            utility::logger::debug << "allocates type: " << type << " subtype: " << subtype << "adress: " << static_cast< const void*>( m_memoryPool.back().raw()) << " size: " << size;

            return m_memoryPool.back().raw();
         }


         Buffer& Context::create()
         {
            m_memoryPool.emplace_back();
            return m_memoryPool.back();
         }



         raw_buffer_type Context::reallocate( raw_buffer_type memory, std::size_t size)
         {
            Buffer& buffer = *get( memory);

            buffer.reallocate( size);

            utility::logger::debug << "reallocates from: " <<
                  static_cast< const void*>( memory) << " to: " << static_cast< const void*>( buffer.raw()) << " new size: " << buffer.size();

            return buffer.raw();
         }

         Buffer& Context::getBuffer( raw_buffer_type memory)
         {
            return *get( memory);
         }


         Buffer Context::extractBuffer( raw_buffer_type memory)
         {
            auto iter = get( memory);
            Buffer buffer = std::move( *iter);
            m_memoryPool.erase( iter);
            return buffer;
         }


         Buffer& Context::add( Buffer&& buffer)
         {
            m_memoryPool.push_back( std::move( buffer));
            return m_memoryPool.back();
         }

         void Context::clear()
         {
            Context::pool_type empty;
            empty.swap( m_memoryPool);
         }

         Context::pool_type::iterator Context::get( raw_buffer_type memory)
         {
            auto findIter = std::find_if(
               m_memoryPool.begin(),
               m_memoryPool.end(),
               local::FindBuffer( memory));

            if( findIter == m_memoryPool.end())
            {
               throw utility::exception::MemoryNotFound();
            }

            return findIter;
         }

         void Context::deallocate( raw_buffer_type memory)
         {
            utility::logger::debug << "deallocates: " << static_cast< const void*>( memory);

            if( memory != 0)
            {
               pool_type::iterator buffer = get( memory);

               m_memoryPool.erase( buffer);
            }

         }
      } // buffer
	} // common
} // casual



