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


            Buffer( Buffer&& rhs) = default;
            Buffer& operator = ( Buffer&& rhs) = default;


            Buffer( const Buffer&) = delete;
            Buffer& operator = ( const Buffer&) = delete;


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

            const std::string& type()
            {
               return m_type;
            }

            const std::string& subtype()
            {
               return m_subtype;
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

            Context( const Context&) = delete;
            Context& operator = ( const Context&) = delete;

            static Context& instance();

            raw_buffer_type allocate( const std::string& type, const std::string& subtype, std::size_t size);

            raw_buffer_type reallocate( raw_buffer_type memory, std::size_t size);

            void deallocate( raw_buffer_type memory);

            Buffer& get( raw_buffer_type memory);

            //!
            //! @return the buffer, after it has been erased from the pool
            //!
            //!
            Buffer extract( raw_buffer_type memory);

            Buffer& add( Buffer&& buffer);

            void clear();


         private:

            pool_type::iterator getFromPool( raw_buffer_type memory);

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
