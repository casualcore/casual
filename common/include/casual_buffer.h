//!
//! casual_buffer.h
//!
//! Created on: Mar 28, 2012
//!     Author: Lazan
//!

#ifndef CASUAL_BUFFER_H_
#define CASUAL_BUFFER_H_

#include <string>
#include <vector>
#include <cstddef>

namespace casual
{
	namespace buffer
	{
		struct Buffer
		{


			Buffer() : m_memory( 0), m_size( 0) {}

			Buffer( const std::string& type, const std::string& subtype)
				: m_type( type), m_subtype( subtype), m_size( 0), m_memory( 0) {}

			std::string m_type;
			std::string m_subtype;
			std::size_t m_size;
			void* m_memory;
		};

		class Holder
		{
		public:

			typedef std::vector< Buffer> pool_type;

			static Holder& instance();

			Buffer& allocate( const std::string& type, const std::string& subtype, std::size_t size);

			Buffer& reallocate( void* memory, std::size_t size);

			void deallocate( void* memory);


		private:

			pool_type::iterator get( void* memory);

			Holder();
			pool_type m_memoryPool;
		};



	}



}






#endif /* CASUAL_BUFFER_H_ */
