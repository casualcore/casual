//!
//! casual_buffer.cpp
//!
//! Created on: Apr 29, 2012
//!     Author: Lazan
//!

#include "common/buffer_context.h"

#include "utility/exception.h"

#include <stdexcept>

#include <algorithm>

namespace casual
{
	namespace buffer
	{

		namespace local
		{
			struct FindBuffer
			{
				FindBuffer( void* toFind) : m_toFind( toFind) {}

				bool operator () ( Buffer& buffer) const
				{
					return m_toFind == buffer.raw();
				}

				void* m_toFind;
			};


		}



		Context::Context()
		{

		}

		Context& Context::instance()
		{
			static Context singleton;
			return singleton;
		}

		Buffer& Context::allocate(const std::string& type, const std::string& subtype, std::size_t size)
		{
			m_memoryPool.push_back( Buffer( type, subtype, size));
			return m_memoryPool.back();
		}


		Buffer& Context::create()
		{
			m_memoryPool.push_back( Buffer());
			return m_memoryPool.back();
		}



		Buffer& Context::reallocate( char* memory, std::size_t size)
		{
			Buffer& buffer = *get( memory);

			buffer.reallocate( size);

			return buffer;
		}

		Buffer& Context::getBuffer( char* memory)
		{
			return *get( memory);
		}


		void Context::clear()
		{
			Context::pool_type empty;
			empty.swap( m_memoryPool);
		}

		Context::pool_type::iterator Context::get( char* memory)
		{
			pool_type::iterator findIter = std::find_if(
				m_memoryPool.begin(),
				m_memoryPool.end(),
				local::FindBuffer( memory));

			if( findIter == m_memoryPool.end())
			{
				throw utility::exception::MemoryNotFound();
			}

			return findIter;
		}

		void Context::deallocate( char* memory)
		{
		   if( memory != 0)
		   {
            pool_type::iterator buffer = get( memory);

            m_memoryPool.erase( buffer);
		   }

		}

	}
}



