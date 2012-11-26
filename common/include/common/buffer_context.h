//!
//! casual_buffer.h
//!
//! Created on: Mar 28, 2012
//!     Author: Lazan
//!

#ifndef CASUAL_BUFFER_H_
#define CASUAL_BUFFER_H_

#include "common/types.h"

#include <string>
#include <list>

namespace casual
{
   namespace common
   {
      namespace buffer
      {
         struct Buffer
         {
            Buffer() {}

            Buffer( const std::string& type, const std::string& subtype, std::size_t size)
               : m_type( type), m_subtype( subtype), m_memory( size) {}


            Buffer( const Buffer&) = delete;
            Buffer& operator = ( const Buffer&) = delete;

            Buffer( Buffer&& rhs)
               : m_type{ std::move( rhs.m_type)},
                 m_subtype{ std::move( rhs.m_subtype)},
                 m_memory{ std::move( rhs.m_memory)}
            {
            }

            Buffer& operator = ( Buffer&& rhs)
            {
               m_type = std::move( rhs.m_type);
               m_subtype = std::move( rhs.m_subtype);
               m_memory = std::move( rhs.m_memory);
               return *this;
            }

            inline raw_buffer_type raw() const
            {
               return m_memory.data();
            }
            std::size_t size() const
            {
               return m_memory.size();
            }

            inline raw_buffer_type reallocate( std::size_t size)
            {
               if( m_memory.size() < size)
               {
                  m_memory.resize( size);
               }
               return raw();
            }

            template< typename A>
            void marshal( A& archive)
            {
               archive & m_type;
               archive & m_subtype;
               archive & m_memory;
            }

         private:
            std::string m_type;
            std::string m_subtype;
            common::binary_type m_memory;
         };

         class Context
         {
         public:

            typedef std::list< Buffer> pool_type;

            static Context& instance();

            raw_buffer_type allocate( const std::string& type, const std::string& subtype, std::size_t size);

            Buffer& create();

            raw_buffer_type reallocate( raw_buffer_type memory, std::size_t size);

            void deallocate( raw_buffer_type memory);

            Buffer& getBuffer( raw_buffer_type memory);

            void clear();


         private:

            pool_type::iterator get( raw_buffer_type memory);

            Context();
            pool_type m_memoryPool;
         };


         namespace scoped
         {
            struct Deallocator
            {
               Deallocator( char* buffer) : m_memory( buffer)
               {

               }

               ~Deallocator()
               {
                  Context::instance().deallocate( m_memory);
               }

               void release()
               {
                  m_memory = 0;
               }

            private:
               char* m_memory;
            };
         }

      } // buffer
	} // common
} // casual






#endif /* CASUAL_BUFFER_H_ */
