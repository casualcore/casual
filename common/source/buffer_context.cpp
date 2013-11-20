//!
//! casual_buffer.cpp
//!
//! Created on: Apr 29, 2012
//!     Author: Lazan
//!

#include "common/buffer_context.h"

#include "common/exception.h"
#include "common/logger.h"

#include "common/field_buffer.h"
#include "common/octet_buffer.h"
#include "common/order_buffer.h"
#include "common/string_buffer.h"


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

         Callback Callback::create( const std::string& type, const std::string& subtype)
         {
            //
            // TODO: better
            //

            if( type == CASUAL_FIELD && subtype.empty())
            {
               return Callback( CasualFieldCreate, CasualFieldExpand, CasualFieldReduce, CasualFieldNeeded);
            }

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

            auto& buffer = m_memoryPool.back();

            if( buffer.callback().m_create( buffer.raw(), buffer.size()) != 0)
            {
               throw exception::xatmi::SystemError();
            }

            return buffer.raw();
         }



         raw_buffer_type Context::reallocate( const_raw_buffer_type memory, const std::size_t size)
         {
            auto& buffer = *getFromPool( memory);

            // check if the user is about to increase size
            if( buffer.size() < size)
            {
               // do the actual expansion
               buffer.reallocate( size);

               // tell the buffer it has been expanded
               if( buffer.callback().m_expand( buffer.raw(), size) != 0)
               {
                  throw exception::xatmi::SystemError();
               }

            }

            // check if the user is about to decrease size
            if( buffer.size() > size)
            {
               // tell the buffer it is about to be reduced
               if( buffer.callback().m_reduce( buffer.raw(), size) != 0)
               {
                  throw exception::xatmi::SystemError();
               }

               // do the actual reduction
               buffer.reallocate( size);

            }


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



