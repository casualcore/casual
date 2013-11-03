//!
//! casual_buffer.cpp
//!
//! Created on: Apr 29, 2012
//!     Author: Lazan
//!

#include "common/buffer_context.h"

#include "common/exception.h"
#include "common/logger.h"

#include "casual_octet_buffer.h"
#include "casual_order_buffer.h"
#include "casual_string_buffer.h"


#include <stdexcept>

#include <algorithm>


//
// TODO: move from here
//
extern long CasualOctetCreate( char* buffer, long size);
extern long CasualOctetExpand( char* buffer, long size);
extern long CasualOctetReduce( char* buffer, long size);
extern long CasualOctetNeeded( char* buffer, long size);

extern long CasualOrderCreate( char* buffer, long size);
extern long CasualOrderExpand( char* buffer, long size);
extern long CasualOrderReduce( char* buffer, long size);
extern long CasualOrderNeeded( char* buffer, long size);

extern long CasualStringCreate( char* buffer, long size);
extern long CasualStringExpand( char* buffer, long size);
extern long CasualStringReduce( char* buffer, long size);
extern long CasualStringNeeded( char* buffer, long size);


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

         Callback Callback::create( const std::string& type, const std::string& subtype)
         {
            //
            // TODO: better
            //

            if( type == CASUAL_OCTET && subtype.empty())
            {
               return Callback( CasualOctetCreate, CasualOctetExpand, CasualOctetReduce, CasualOctetNeeded);
            }

            if( type == CASUAL_ORDER && subtype.empty())
            {
               return Callback( CasualOrderCreate, CasualOrderExpand, CasualOrderReduce, CasualOrderNeeded);
            }

            if( type == CASUAL_STRING && subtype.empty())
            {
               return Callback( CasualStringCreate, CasualStringExpand, CasualStringReduce, CasualStringNeeded);
            }

            //
            // TODO: throw if unknown buffer ... or ?
            //

            //throw common::exception::xatmi::SystemError();

            return Callback( CasualOctetCreate, CasualOctetExpand, CasualOctetReduce, CasualOctetNeeded);

         }


         Context::Context()
         {

         }

         Context& Context::instance()
         {
            static Context singleton;
            return singleton;
         }

         common::raw_buffer_type Context::allocate(const std::string& type, const std::string& subtype, const std::size_t size)
         {

            // TODO: create Callback from type+subtype
            m_memoryPool.emplace_back( type, subtype, size);

            common::logger::debug << "allocates type: " << type << " subtype: " << subtype << " @" << static_cast< const void*>( m_memoryPool.back().raw()) << " size: " << size;

            return m_memoryPool.back().raw();
         }



         raw_buffer_type Context::reallocate( const_raw_buffer_type memory, const std::size_t size)
         {
            auto& buffer = *getFromPool( memory);

            buffer.reallocate( size);

            common::logger::debug << "reallocates from: " <<
                  static_cast< const void*>( memory) << " to: " << static_cast< const void*>( buffer.raw()) << " new size: " << buffer.size();

            return buffer.raw();
         }



         Buffer& Context::get( const_raw_buffer_type memory)
         {
            return *getFromPool( memory);
         }


         Buffer Context::extract( const_raw_buffer_type memory)
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

         Context::pool_type::iterator Context::getFromPool( const_raw_buffer_type memory)
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

         void Context::deallocate( const_raw_buffer_type memory)
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



