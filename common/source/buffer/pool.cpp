//!
//! pool.cpp
//!
//! Created on: Sep 18, 2014
//!     Author: Lazan
//!

#include "common/buffer/pool.h"
#include "common/algorithm.h"
#include "common/exception.h"


#include <functional>


namespace casual
{

   namespace common
   {
      namespace buffer
      {
         namespace pool
         {

            Holder::Base& Holder::find( const Type& type)
            {
               auto pool = range::find_if( m_pools, [&]( std::unique_ptr< Base>& b){
                  return b->manage( type);
               });

               if( ! pool)
               {
                  throw exception::xatmi::buffer::TypeNotSupported{};
               }
               return **pool;
            }

            Holder::Base& Holder::find( platform::const_raw_buffer_type handle)
            {
               auto pool = range::find_if( m_pools, [&]( std::unique_ptr< Base>& b){
                  return b->manage( handle);
               });

               if( ! pool)
               {
                  throw exception::xatmi::InvalidArguments{ "buffer not valid"};
               }
               return **pool;
            }

            platform::raw_buffer_type Holder::allocate( const Type& type, std::size_t size)
            {
               auto buffer = find( type).allocate( type, size);

               if( log::internal::buffer)
               {
                  log::internal::buffer << "allocate type: " << type << " size: " << size << " @" << static_cast< const void*>( buffer) << '\n';
               }
               return buffer;
            }

            platform::raw_buffer_type Holder::reallocate( platform::const_raw_buffer_type handle, std::size_t size)
            {
               auto buffer = find( handle).reallocate( handle, size);

               if( log::internal::buffer)
               {
                  log::internal::buffer << "reallocate size: " << size
                        << " from @" << static_cast< const void*>( handle) << " to " << static_cast< const void*>( buffer) << '\n';
               }

               return buffer;
            }

            const Type& Holder::type( platform::const_raw_buffer_type handle)
            {
               return get( handle).type;
            }

            void Holder::deallocate( platform::const_raw_buffer_type handle)
            {
               if( handle != nullptr)
               {
                  find( handle).deallocate( handle);

                  if( log::internal::buffer)
                  {
                     log::internal::buffer << "deallocate @" << static_cast< const void*>( handle) << '\n';
                  }
               }
            }

            platform::raw_buffer_type Holder::insert( Payload&& payload)
            {
               if( log::internal::buffer)
               {
                  log::internal::buffer << "insert type: " << payload.type << " size: " << payload.memory.size() << " @" << static_cast< const void*>( payload.memory.data()) << '\n';
               }

               auto buffer = find( payload.type).insert( std::move( payload));

               return buffer;
            }

            Payload& Holder::get( platform::const_raw_buffer_type handle)
            {
               return find( handle).get( handle);
            }

            Payload Holder::extract( platform::const_raw_buffer_type handle)
            {
               auto result = find( handle).extract( handle);

               if( log::internal::buffer)
               {
                  log::internal::buffer << "extract type: " << result.type << " size: " << result.memory.size() << " @" << static_cast< const void*>( result.memory.data()) << '\n';
               }
               return result;
            }

            void Holder::clear()
            {
               range::for_each( m_pools, std::mem_fn( &Base::clear));
            }



         } // pool
      } // buffer
   } // common

} // casual
