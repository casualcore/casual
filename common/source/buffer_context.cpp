//!
//! casual_buffer.cpp
//!
//! Created on: Apr 29, 2012
//!     Author: Lazan
//!

#include "common/buffer_context.h"

#include "utility/exception.h"
#include "utility/logger.h"

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

			m_memoryPool.emplace_back( type, subtype, size);

			utility::logger::debug << "allocates type: " << type << " subtype: " << subtype << "adress: " << static_cast< void*>( m_memoryPool.back().raw()) << " size: " << size;

			return m_memoryPool.back();
		}


		Buffer& Context::create()
		{
			m_memoryPool.emplace_back();
			return m_memoryPool.back();
		}



		Buffer& Context::reallocate( char* memory, std::size_t size)
		{
			Buffer& buffer = *get( memory);

			 utility::logger::debug << "reallocates: " << static_cast< void*>( memory) << " size: " << size;

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
			auto findIter = std::find_if(
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
		   utility::logger::debug << "deallocates: " << static_cast< void*>( memory);

		   if( memory != 0)
		   {
            pool_type::iterator buffer = get( memory);

            m_memoryPool.erase( buffer);
		   }

		}

	}
}



