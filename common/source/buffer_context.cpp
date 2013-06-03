//!
//! casual_buffer.cpp
//!
//! Created on: Apr 29, 2012
//!     Author: Lazan
//!

#include "common/buffer_context.h"

#include "common/exception.h"
#include "common/logger.h"

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

            common::logger::debug << "allocates type: " << type << " subtype: " << subtype << " @" << static_cast< const void*>( m_memoryPool.back().raw()) << " size: " << size;

            return m_memoryPool.back().raw();
         }



         raw_buffer_type Context::reallocate( raw_buffer_type memory, std::size_t size)
         {
            auto& buffer = *getFromPool( memory);

            buffer.reallocate( size);

            common::logger::debug << "reallocates from: " <<
                  static_cast< const void*>( memory) << " to: " << static_cast< const void*>( buffer.raw()) << " new size: " << buffer.size();

            return buffer.raw();
         }



         Buffer& Context::get( raw_buffer_type memory)
         {
            return *getFromPool( memory);
         }


         Buffer Context::extract( raw_buffer_type memory)
         {
            auto iter = getFromPool( memory);
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

         Context::pool_type::iterator Context::getFromPool( raw_buffer_type memory)
         {
            auto findIter = std::find_if(
               m_memoryPool.begin(),
               m_memoryPool.end(),
               local::FindBuffer( memory));

            if( findIter == m_memoryPool.end())
            {
               throw common::exception::MemoryNotFound();
            }

            return findIter;
         }

         void Context::deallocate( raw_buffer_type memory)
         {
            common::logger::debug << "deallocates: " << static_cast< const void*>( memory);

            if( memory != 0)
            {
               auto buffer = getFromPool( memory);

               m_memoryPool.erase( buffer);
            }

         }
      } // buffer
	} // common
} // casual



