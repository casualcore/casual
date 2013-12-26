//!
//! casual_buffer.h
//!
//! Created on: Mar 28, 2012
//!     Author: Lazan
//!

#ifndef CASUAL_BUFFER_H_
#define CASUAL_BUFFER_H_

#include "common/platform.h"

#include <string>
#include <list>

namespace casual
{
   namespace common
   {
      namespace buffer
      {
         struct Type
         {
            Type() = default;
            Type( const std::string& type, const std::string& subtype) : type( type), subtype( subtype) {}
            Type( const char* type, const char* subtype) : type( type ? type : ""), subtype( subtype ? subtype : "") {}

            std::string type;
            std::string subtype;

            template< typename A>
            void marshal( A& archive)
            {
               archive & type;
               archive & subtype;
            }
         };

         inline bool operator < ( const Type& lhs, const Type& rhs)
         {
            if( lhs.type < rhs.type)
               return true;
            if( rhs.type < lhs.type)
               return false;
            return lhs.subtype < rhs.subtype;

         }

         namespace implementation
         {
            class Base;
         }


         struct Buffer
         {
            Buffer() {}

            Buffer( const std::string& type, const std::string& subtype, std::size_t size)
               : m_type{ type, subtype}, m_memory( size) {}

            Buffer( buffer::Type&& type, std::size_t size)
               : m_type( std::move( type)), m_memory( size) {}


            Buffer( Buffer&& rhs) = default;
            Buffer& operator = ( Buffer&& rhs) = default;


            Buffer( const Buffer&) = delete;
            Buffer& operator = ( const Buffer&) = delete;


            inline platform::raw_buffer_type raw()
            {
               return m_memory.data();
            }

            inline std::size_t size() const
            {
               return m_memory.size();
            }

            inline platform::raw_buffer_type reallocate( std::size_t size)
            {
               if( m_memory.size() < size)
               {
                  m_memory.resize( size);
               }
               return raw();
            }

            inline const std::string& type() const
            {
               return m_type.type;
            }

            inline const std::string& subtype() const
            {
               return m_type.subtype;
            }

            template< typename A>
            void marshal( A& archive)
            {
               archive & m_type;
               archive & m_memory;
            }

            inline implementation::Base& implementation()
            {
               return *m_implemenation;
            }


         private:
            implementation::Base* m_implemenation = nullptr;
            Type m_type;
            platform::binary_type m_memory;

         };



         namespace implementation
         {
            class Base
            {
            public:
               virtual ~Base();

               Buffer create( buffer::Type&& type, std::size_t size);
               void reallocate( Buffer& buffer, std::size_t size);

               void network( Buffer& buffer);

            private:

               virtual Buffer doCreate( buffer::Type&& type, std::size_t size) = 0;
               virtual void doReallocate( Buffer& buffer, std::size_t size) = 0;
               virtual void doNetwork( Buffer& buffer) = 0;
            };

            bool registrate( Base& implemenation, const buffer::Type& type);

            template< typename I>
            bool registrate( const buffer::Type& type)
            {
               static I implemenation;
               return registrate( implemenation, type);
            }


         } // implementation

         class Context
         {
         public:

            typedef std::list< Buffer> pool_type;

            Context( const Context&) = delete;
            Context& operator = ( const Context&) = delete;




            static Context& instance();

            platform::raw_buffer_type allocate( buffer::Type&& type, std::size_t size);

            platform::raw_buffer_type reallocate( platform::const_raw_buffer_type memory, std::size_t size);

            void deallocate( platform::const_raw_buffer_type memory);

            Buffer& get( platform::const_raw_buffer_type memory);

            //!
            //! @return the buffer, after it has been erased from the pool
            //!
            //!
            Buffer extract( platform::const_raw_buffer_type memory);

            Buffer& add( Buffer&& buffer);

            void clear();


         private:

            pool_type::iterator getFromPool( platform::const_raw_buffer_type memory);

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
