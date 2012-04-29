//!
//! casual_buffer.cpp
//!
//! Created on: Apr 29, 2012
//!     Author: Lazan
//!

#include "casual_buffer.h"


#include <stdexcept>
#include "casual_exception.h"

namespace casual
{
	namespace buffer
	{

		namespace local
		{
			struct FindBuffer
			{
				FindBuffer( void* toFind) : m_toFind( toFind) {}

				bool operator () ( const Buffer& buffer)
				{
					return m_toFind == buffer.m_memory;
				}

				void* m_toFind;
			};


		}

		Holder::Holder()
		{

		}

		Holder& Holder::instance()
		{
			static Holder singleton;
			return singleton;
		}

		Buffer& Holder::allocate(const std::string& type, const std::string& subtype, std::size_t size)
		{

			Buffer buffer( type, subtype);
			buffer.m_size = size >= 1024 ? size : 1024;
			// TODO validate

			buffer.m_memory = malloc( buffer.m_size);

			if( buffer.m_memory == 0)
			{
				throw std::bad_alloc();
			}

			m_memoryPool.push_back( buffer);
			return m_memoryPool.back();
		}



		Buffer& Holder::reallocate( void* memory, std::size_t size)
		{
			Buffer& buffer = *get( memory);

			if( buffer.m_size < size)
			{
				buffer.m_memory = ::realloc( buffer.m_memory, size);
			}

			return buffer;
		}

		Holder::pool_type::iterator Holder::get( void* memory)
		{
			std::vector< Buffer>::iterator findIter = std::find_if(
				m_memoryPool.begin(),
				m_memoryPool.end(),
				local::FindBuffer( memory));

			if( findIter == m_memoryPool.end())
			{
				throw exception::MemoryNotFound();
			}

			return findIter;
		}

		void Holder::deallocate( void* memory)
		{
			pool_type::iterator buffer = get( memory);

			free( buffer->m_memory);

			m_memoryPool.erase( buffer);

		}

	}
}



