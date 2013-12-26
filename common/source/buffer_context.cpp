//!
//! casual_buffer.cpp
//!
//! Created on: Apr 29, 2012
//!     Author: Lazan
//!

#include "common/buffer_context.h"

#include "common/exception.h"
#include "common/log.h"

#include "common/field_buffer.h"
#include "common/octet_buffer.h"
#include "common/order_buffer.h"
#include "common/string_buffer.h"


#include <stdexcept>

#include <algorithm>
#include <map>


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

         namespace implementation
         {

            Base::~Base() {}

            Buffer Base::create( buffer::Type&& type, std::size_t size)
            {
               return doCreate( std::move( type), size);
            }

            void Base::reallocate( Buffer& buffer, std::size_t size)
            {
               doReallocate( buffer, size);
            }

            void Base::network( Buffer& buffer)
            {
               doNetwork( buffer);
            }

            namespace local
            {
               namespace
               {
                  typedef std::map< buffer::Type, implementation::Base&> implementations_type;
                  implementations_type& implementations()
                  {
                     static implementations_type singleton;
                     return singleton;
                  }
               }

            } // local


            bool registrate( Base& implemenation, const buffer::Type& type)
            {
               local::implementations().emplace( type, implemenation);
               return true;
            }

            Base& get( const buffer::Type& type)
            {
               auto found = local::implementations().find( type);
               if( found == std::end( local::implementations()))
               {
                  throw exception::xatmi::buffer::TypeNotSupported( "type: " + type.type + " subtype: " + type.subtype);
               }
               return found->second;
            }



         } // implementation


         Context::Context()
         {

         }

         Context& Context::instance()
         {
            static Context singleton;
            return singleton;
         }

         platform::raw_buffer_type Context::allocate( buffer::Type&& type, std::size_t size)
         {
            auto& impl = implementation::get( type);

            m_memoryPool.push_back( impl.create( std::move( type), size));
            auto& buffer = m_memoryPool.back();


            common::log::debug << "allocates type: " << buffer.type() << " subtype: " << buffer.subtype() << " @" << static_cast< const void*>( buffer.raw()) << " size: " << buffer.size() << std::endl;

            return buffer.raw();
         }



         platform::raw_buffer_type Context::reallocate( platform::const_raw_buffer_type memory, std::size_t size)
         {
            auto& buffer = *getFromPool( memory);

            buffer.implementation().reallocate( buffer, size);


            common::log::debug << "reallocates from: " <<
                  static_cast< const void*>( memory) << " to: " << static_cast< const void*>( buffer.raw()) << " new size: " << buffer.size();

            return buffer.raw();
         }



         Buffer& Context::get( platform::const_raw_buffer_type memory)
         {
            return *getFromPool( memory);
         }


         Buffer Context::extract( platform::const_raw_buffer_type memory)
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

         Context::pool_type::iterator Context::getFromPool( platform::const_raw_buffer_type memory)
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

         void Context::deallocate( platform::const_raw_buffer_type memory)
         {
            common::log::debug << "deallocates: " << static_cast< const void*>( memory) << std::endl;

            if( memory != 0)
            {
               auto buffer = getFromPool( memory);

               m_memoryPool.erase( buffer);
            }

         }
      } // buffer
	} // common
} // casual



