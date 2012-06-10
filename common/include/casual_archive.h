//!
//! casual_archive.h
//!
//! Created on: Jun 9, 2012
//!     Author: Lazan
//!

#ifndef CASUAL_ARCHIVE_H_
#define CASUAL_ARCHIVE_H_

#include <vector>

namespace casual
{
	namespace archive
	{
		namespace output
		{
			struct Binary
			{
				typedef std::vector< char> buffer_type;

				const buffer_type& get() const
				{
					return m_buffer;
				}


				template< typename T>
				Binary& operator & ( T& value)
				{
					return this->operator << ( value);
				}

				template< typename T>
				Binary& operator << ( T& value)
				{
					write( value);
					return *this;
				}

			private:

				template< typename T>
				void write( T& value)
				{
					value.serialize( *this);
				}

				void write( long& value)
				{
					writePod( value);
				}

				void write( std::size_t& value)
				{
					writePod( value);
				}

				void write( int& value)
				{
					writePod( value);
				}

				template< typename T>
				void writePod( T value)
				{
					Binary::buffer_type::size_type size = m_buffer.size();

					m_buffer.resize( size + sizeof( T));

					memcpy( &m_buffer[ size], &value, sizeof( T));
				}

				template< typename T>
				void write( std::vector< T>& value)
				{
					writePod( value.size());

					typename std::vector< T>::iterator current = value.begin();

					for(; current != value.end(); ++current)
					{
						*this << *current;
					}
				}

				void write( std::string& value)
				{
					writePod( value.size());
					Binary::buffer_type::size_type size = m_buffer.size();

					m_buffer.resize( size + value.size());

					memcpy( &m_buffer[ size], value.c_str(), value.size());
				}

				buffer_type m_buffer;
			};

		} // output

		namespace input
		{

			struct Binary
			{
				typedef std::vector< char> buffer_type;
				typedef std::vector< char>::size_type offest_type;

				Binary() : m_offset( 0) {}

				//!
				//! Only for unittest
				//!
				Binary( const buffer_type& buffer) : m_buffer( buffer), m_offset( 0) {}


				template< std::size_t size>
				void add( char (& buffer)[ size])
				{
					m_buffer.resize( m_buffer.size() + size);

					memcpy( &buffer[ m_buffer.size()], &buffer, size);
				}


				template< typename T>
				Binary& operator & ( T& value)
				{
					return this->operator >>( value);
				}

				template< typename T>
				Binary& operator >> ( T& value)
				{
					read( value);
					return *this;
				}

			private:

				template< typename T>
				void read( T& value)
				{
					value.serialize( *this);
				}

				void read( long& value)
				{
					readPod( value);
				}

				void read( std::size_t& value)
				{
					readPod( value);
				}

				void read( int& value)
				{
					readPod( value);
				}

				template< typename T>
				void read( std::vector< T>& value)
				{
					typename std::vector< T>::size_type size;
					*this >> size;

					value.resize( size);

					typename std::vector< T>::iterator current = value.begin();

					for(; current != value.end(); ++current)
					{
						*this >> *current;
					}
				}

				void read( std::string& value)
				{
					std::string::size_type size;
					*this >> size;

					value.resize( size);

					std::copy(
						m_buffer.begin() + m_offset,
						m_buffer.begin() + m_offset + size,
						value.begin());

					m_offset += size;
				}

				template< typename T>
				void readPod( T& value)
				{
					memcpy( &value, &m_buffer[ m_offset], sizeof( T));
					m_offset += sizeof( T);
				}

				buffer_type m_buffer;
				offest_type m_offset;
			};

		} // output

	} // archive
} // casual



#endif /* CASUAL_ARCHIVE_H_ */
