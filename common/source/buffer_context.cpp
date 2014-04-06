//!
//! casual_buffer.cpp
//!
//! Created on: Apr 29, 2012
//!     Author: Lazan
//!

#include "common/buffer_context.h"

#include "common/exception.h"
#include "common/internal/log.h"


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

         Buffer::Buffer() {}


         Buffer::Buffer( buffer::Type&& type, std::size_t size, implementation::Base& implementaion)
            : m_implemenation( &implementaion), m_type( std::move( type))
         {
            m_implemenation->create( *this, size);
         }

         Buffer::Buffer( buffer::Type&& type, std::size_t size)
            : Buffer( std::move( type), size, implementation::get( type)) {}

         Buffer::Buffer( Buffer&& rhs) = default;
         Buffer& Buffer::operator = ( Buffer&& rhs) = default;


         platform::raw_buffer_type Buffer::raw()
         {
            return m_memory.data();
         }

         std::size_t Buffer::size() const
         {
            return m_memory.size();
         }

         void Buffer::reallocate( std::size_t size)
         {
            m_implemenation->reallocate( *this, size);
         }

         const Type& Buffer::type() const
         {
            return m_type;
         }

         implementation::Base& Buffer::implementation()
         {
            return *m_implemenation;
         }

         const platform::binary_type& Buffer::memory() const
         {
            return m_memory;
         }
         platform::binary_type& Buffer::memory()
         {
            return m_memory;
         }



         namespace implementation
         {

            Base::~Base() {}

            void Base::create( Buffer& buffer, std::size_t size)
            {
               doCreate( buffer, size);
            }

            void Base::reallocate( Buffer& buffer, std::size_t size)
            {
               doReallocate( buffer, size);
            }

            void Base::network( Buffer& buffer)
            {
               doNetwork( buffer);
            }


            void Base::doCreate( Buffer& buffer, std::size_t size)
            {
               buffer.memory().resize( size);

            }
            void Base::doReallocate( Buffer& buffer, std::size_t size)
            {
               buffer.memory().resize( size);
            }

            void Base::doNetwork( Buffer& buffer)
            {
               // no op
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

            bool registrate( Base& implemenation, const std::vector< buffer::Type>& types)
            {
               for( auto& type : types)
               {
                  local::implementations().emplace( type, implemenation);
               }
               return true;
            }

            Base& get( const buffer::Type& type)
            {
               auto found = local::implementations().find( type);
               if( found == std::end( local::implementations()))
               {
                  throw exception::xatmi::buffer::TypeNotSupported( "buffer type not suported - type: " + type.type + " subtype: " + type.subtype);
               }
               return found->second;
            }


            //
            // Default implementation for standard X_OCTET
            //
            struct XOCTET : public Base
            {

               static const bool registration;
            };

            const bool XOCTET::registration = registrate< XOCTET>({
               { "X_OCTET", ""},
               {"X_OCTET", "binary"},
               {"X_OCTET", "YAML"},
               {"X_OCTET", "JSON"}});



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
            m_memoryPool.emplace_back( std::move( type), size);
            auto& buffer = m_memoryPool.back();

            common::log::internal::debug << "allocates type: " << buffer.type().type << " subtype: " << buffer.type().subtype << " @" << static_cast< const void*>( buffer.raw()) << " size: " << buffer.size() << std::endl;

            return buffer.raw();
         }



         platform::raw_buffer_type Context::reallocate( platform::const_raw_buffer_type memory, std::size_t size)
         {
            auto& buffer = *getFromPool( memory);
            buffer.reallocate( size);

            common::log::internal::debug << "reallocates from: " <<
                  static_cast< const void*>( memory) << " to: " << static_cast< const void*>( buffer.raw()) << " new size: " << buffer.size() << std::endl;

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
            common::log::internal::debug << "deallocates: " << static_cast< const void*>( memory) << std::endl;

            if( memory != 0)
            {
               auto buffer = getFromPool( memory);

               m_memoryPool.erase( buffer);
            }

         }
      } // buffer
	} // common
} // casual



