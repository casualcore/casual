//!
//! casual_queue.cpp
//!
//! Created on: Jun 10, 2012
//!     Author: Lazan
//!

#include "casual_queue.h"

#include "casual_ipc.h"


// temp
#include <iostream>

namespace casual
{
	namespace queue
	{
		namespace local
		{
			namespace
			{
				Reader::cache_type& messageCache()
				{
					static Reader::cache_type singleton;
					return singleton;
				}

				struct MessageType
				{
					MessageType( Reader::message_type_type type) : m_type( type) {}

					bool operator () ( const ipc::message::Transport& message)
					{
						return message.m_payload.m_type == m_type;
					}

				private:
					Reader::message_type_type m_type;
				};

				struct Correlation
				{
					Correlation( const utility::Uuid& uuid) : m_uuid( uuid) {}

					bool operator () ( const ipc::message::Transport& message)
					{
						return message.m_payload.m_header.m_correlation == m_uuid;
					}

				private:
					utility::Uuid m_uuid;
				};

			}

		}

		Reader::Reader( ipc::receive::Queue& queue) : m_queue( queue) {}

		Reader::message_type_type Reader::next()
		{
			cache_type::iterator current = fetchIfEmpty( messageCache().begin());

			const Reader::message_type_type type = current->m_payload.m_type;

			return current->m_payload.m_type;
		}

		void Reader::correlate( archive::input::Binary& archive, message_type_type type)
		{
			cache_type::iterator current = first( type);

			archive.add( *current);

			local::Correlation correlation( current->m_payload.m_header.m_correlation);
			int count = current->m_payload.m_header.m_count;

			//
			// Erase the consumed transport message from cache
			//
			current = messageCache().erase( current);

			//
			// Fetch transport messages until "count" is zero, ie there are no correlated
			// messages left.
			//
			while( count != 0)
			{
				//
				// Make sure the iterator does point to a transport message
				//
				current = fetchIfEmpty( current);

				while( !correlation( *current))
				{
					current = fetchIfEmpty( ++current);
				}

				archive.add( *current);
				count = current->m_payload.m_header.m_count;
				current = messageCache().erase( current);
			}
		}

		Reader::cache_type::iterator Reader::first( message_type_type type)
		{
			cache_type::iterator current = fetchIfEmpty( messageCache().begin());

			//
			// loop while fetched type is not the wanted type.
			//
			while(  current->m_payload.m_type != type)
			{
				current = fetchIfEmpty( ++current);
			}

			return current;
		}

		Reader::cache_type::iterator Reader::fetchIfEmpty( cache_type::iterator start)
		{
			if( start == messageCache().end())
			{
				ipc::message::Transport transport;

				m_queue( transport);

				return messageCache().insert( start, transport);
			}

			return start;
		}


		Reader::cache_type& Reader::messageCache()
		{
			return local::messageCache();
		}

	} // queue
} // casual


