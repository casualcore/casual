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
#include <list>
#include <cstddef>
#include <cstdlib>

namespace casual
{
	namespace buffer
	{
		struct Buffer
		{
			Buffer() {}

			Buffer( const std::string& type, const std::string& subtype, std::size_t size)
				: m_type( type), m_subtype( subtype), m_memory( size) {}

			char* raw()
			{
				if( m_memory.empty())
				{
					return 0;
				}
				return &m_memory[ 0];
			}
			std::size_t size() const
			{
				return m_memory.size();
			}

			std::size_t reallocate( std::size_t size)
			{
				if( m_memory.size() < size)
				{
					m_memory.resize( size);
				}
				return m_memory.size();
			}


			template< typename A>
			void marshal( A& archive)
			{
				archive & m_type;
				archive & m_subtype;
				archive & m_memory;

			}

			void clear()
			{
				m_memory.clear();
			}

		private:
			//Buffer( const Buffer&);
			Buffer& operator = ( const Buffer&);

			std::string m_type;
			std::string m_subtype;
			std::vector< char> m_memory;
		};

		class Context
		{
		public:

			typedef std::list< Buffer> pool_type;

			static Context& instance();

			Buffer& allocate( const std::string& type, const std::string& subtype, std::size_t size);

			Buffer& create();

			Buffer& reallocate( char* memory, std::size_t size);

			void deallocate( char* memory);

			Buffer& getBuffer( char* memory);

			void clear();


		private:

			pool_type::iterator get( char* memory);

			Context();
			pool_type m_memoryPool;
		};


		namespace scoped
		{
			struct Deallocator
			{
				Deallocator( Buffer& buffer) : m_memory( buffer.raw())
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


	}



}






#endif /* CASUAL_BUFFER_H_ */
