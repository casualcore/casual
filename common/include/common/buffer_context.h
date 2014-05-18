//!
//! casual_buffer.h
//!
//! Created on: Mar 28, 2012
//!     Author: Lazan
//!

#ifndef CASUAL_BUFFER_H_
#define CASUAL_BUFFER_H_

#include "common/platform.h"
#include "common/algorithm.h"

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
            Buffer();
            Buffer( buffer::Type&& type, std::size_t size);

            Buffer( Buffer&& rhs) noexcept;
            Buffer& operator = ( Buffer&& rhs) noexcept;


            Buffer( const Buffer&)  = delete;
            Buffer& operator = ( const Buffer&) = delete;


            platform::raw_buffer_type raw();

            std::size_t size() const;

            void reallocate( std::size_t size);

            const Type& type() const;

            //implementation::Base& implementation();

            const platform::binary_type& memory() const;
            platform::binary_type& memory();

            template< typename A>
            void marshal( A& archive);

         private:
            Buffer( buffer::Type&& type, std::size_t size, implementation::Base& implementaion);

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

               void create( Buffer& buffer, std::size_t size);
               void reallocate( Buffer& buffer, std::size_t size);

               void network( Buffer& buffer);

            private:

               virtual void doCreate( Buffer& buffer, std::size_t size);
               virtual void doReallocate( Buffer& buffer, std::size_t size);
               virtual void doNetwork( Buffer& buffer);
            };

            bool registrate( Base& implemenation, const std::vector< buffer::Type>& types);

            template< typename I>
            bool registrate( const std::vector< buffer::Type>& types)
            {
               static I implemenation;
               return registrate( implemenation, types);
            }

            Base& get( const buffer::Type& type);

         } // implementation


         template< typename A>
         void Buffer::marshal( A& archive)
         {
            archive & m_type;
            archive & m_memory;

            if( ! m_implemenation)
            {
               m_implemenation = &implementation::get( m_type);
            }
         }


         class Context
         {
         public:

            typedef std::vector< Buffer> pool_type;

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
            using range_type = decltype( range::make( pool_type().begin(), pool_type().end()));

            range_type getFromPool( platform::const_raw_buffer_type memory);

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
