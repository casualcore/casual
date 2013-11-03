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
#include <functional>

namespace casual
{
   namespace common
   {
      namespace buffer
      {

         struct Callback
         {
            //
            // Functions shall return 0 on succes
            //
            typedef std::function<long(char*,long)> interface;

            interface m_create;
            interface m_expand;
            interface m_reduce;
            //interface m_remove;
            interface m_needed;


            Callback()
            : m_create( nullptr),
              m_expand( nullptr),
              m_reduce( nullptr),
              //m_remove( nullptr),
              m_needed( nullptr)
            {}


            Callback(
               interface create_function,
               interface expand_function,
               interface reduce_function,
               //interface remove_function,
               interface needed_function)
             : m_create( create_function),
               m_expand( expand_function),
               m_reduce( reduce_function),
               //m_remove( remove_function),
               m_needed( needed_function)
            {}

            Callback( Callback&&) = default;
            Callback( const Callback&) = default;
            Callback& operator = ( Callback&&) = default;
            Callback& operator = ( const Callback&) = default;

            static Callback create( const std::string& type, const std::string& subtype);
         };

         struct Buffer
         {
            Buffer() {}

            Buffer( const std::string& type, const std::string& subtype, const std::size_t size)
               : m_type( type), m_subtype( subtype), m_memory( size), m_callback( Callback::create( m_type, m_subtype)) {}


            Buffer( Buffer&& rhs) = default;
            Buffer& operator = ( Buffer&& rhs) = default;


            Buffer( const Buffer&) = delete;
            Buffer& operator = ( const Buffer&) = delete;


            inline raw_buffer_type raw()
            {
               return m_memory.data();
            }
            std::size_t size() const
            {
               return m_memory.size();
            }

            inline raw_buffer_type reallocate( const std::size_t size)
            {
               m_memory.resize( size);
               return raw();
            }

            const std::string& type() const
            {
               return m_type;
            }

            const std::string& subtype() const
            {
               return m_subtype;
            }

            const Callback& callback() const
            {
               return m_callback;
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
            binary_type m_memory;
            Callback m_callback;
         };

         class Context
         {
         public:

            typedef std::list< Buffer> pool_type;

            Context( const Context&) = delete;
            Context& operator = ( const Context&) = delete;

            static Context& instance();

            raw_buffer_type allocate( const std::string& type, const std::string& subtype, std::size_t size);

            raw_buffer_type reallocate( const_raw_buffer_type memory, std::size_t size);

            void deallocate( const_raw_buffer_type memory);

            Buffer& get( const_raw_buffer_type memory);

            //!
            //! @return the buffer, after it has been erased from the pool
            //!
            //!
            Buffer extract( const_raw_buffer_type memory);

            Buffer& add( Buffer&& buffer);

            void clear();

         private:

            pool_type::iterator getFromPool( const_raw_buffer_type memory);

            Context();
            pool_type m_memoryPool;
         };


         namespace scoped
         {
            struct Deallocator
            {
               Deallocator( char* buffer) : m_memory( buffer)
               {}

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
