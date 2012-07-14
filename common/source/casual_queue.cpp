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


#include <algorithm>

namespace casual
{
	namespace queue
	{
		namespace local
		{

			namespace
			{
				typedef std::list< ipc::message::Transport> cache_type;
				typedef ipc::message::Transport transport_type;
				typedef transport_type::message_type_type message_type_type;

				cache_type& messageCache()
				{
					static cache_type singleton;
					return singleton;
				}



				struct MessageType
				{
					MessageType( message_type_type type) : m_type( type) {}

					bool operator () ( const ipc::message::Transport& message)
					{
						return message.m_payload.m_type == m_type;
					}

				private:
					message_type_type m_type;
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

				struct MessageTypeInCache
				{
					MessageTypeInCache( message_type_type type) : m_type( type) {}

					bool operator () ( const ipc::message::Transport& message)
					{
						return m_type( message) && message.m_payload.m_header.m_count == 0;
					}

				private:
					MessageType m_type;
				};

				template< typename Q>
				void correlate( Q& queue, archive::input::Binary& archive, cache_type::iterator current)
				{
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
						current = fetchIfEmpty( queue, current);

						while( !correlation( *current))
						{
							current = fetchIfEmpty( queue, ++current);
						}

						archive.add( *current);
						count = current->m_payload.m_header.m_count;
						current = messageCache().erase( current);
					}
				}

				template< typename Q>
				cache_type::iterator fetchIfEmpty( Q& queue, cache_type::iterator start)
				{
					if( start == messageCache().end())
					{
						ipc::message::Transport transport;

						queue( transport);

						return messageCache().insert( start, transport);
					}

					return start;
				}

				template< typename Q>
				cache_type::iterator first( Q& queue, message_type_type type)
				{
					cache_type::iterator current = fetchIfEmpty( queue, messageCache().begin());

					//
					// loop while fetched type is not the wanted type.
					//
					while(  current->m_payload.m_type != type)
					{
						current = fetchIfEmpty( queue, ++current);
					}

					return current;
				}



				bool write( archive::output::Binary& archive, message_type_type type, ipc::send::Queue& queue, const long flags)
            {

               utility::Uuid correlation;
               ipc::message::Transport transport;

               transport.m_payload.m_type = type;
               correlation.get( transport.m_payload.m_header.m_correlation);

               //
               // Figure out how many transport-messages we have to use
               //
               std::size_t count = archive.get().size() / ipc::message::Transport::payload_max_size;

               if( archive.get().size() % ipc::message::Transport::payload_max_size != 0)
               {
                  count += 1;
               }

               for( std::size_t index = 0; index < count; ++index)
               {
                  transport.m_payload.m_header.m_count = count - index -1;

                  const std::size_t offset = index *  ipc::message::Transport::payload_max_size;
                  const std::size_t length =
                        archive.get().size() - offset < ipc::message::Transport::payload_max_size ?
                              archive.get().size() - offset : ipc::message::Transport::payload_max_size;

                  //
                  // Copy payload
                  //
                  std::copy(
                     archive.get().begin() + offset,
                     archive.get().begin() + offset + length,
                     transport.m_payload.m_payload);

                  transport.paylodSize( length);

                  if( !queue( transport, flags))
                  {
                     //
                     // We faild to send part of message (non blocking). No need to try send the rest of
                     // the message...
                     //
                     return false;
                  }
               }
               return true;

            }
			} // <unnamed>

		} // local


		namespace blocking
		{

		   Writer::Writer( ipc::send::Queue& queue) : m_queue( queue) {}

		   void Writer::send( archive::output::Binary& archive, message_type_type type)
		   {
		      local::write( archive, type, m_queue, 0);
		   }



			Reader::Reader( ipc::receive::Queue& queue) : m_queue( queue) {}

			message_type_type Reader::next()
			{
				local::cache_type::iterator current = local::fetchIfEmpty( m_queue, local::messageCache().begin());

				return current->m_payload.m_type;
			}



			void Reader::correlate( archive::input::Binary& archive, message_type_type type)
			{
				local::cache_type::iterator current = local::first( m_queue, type);

				local::correlate( m_queue, archive, current);
			}
		} // blocking


		namespace non_blocking
		{
		   Writer::Writer( ipc::send::Queue& queue) : m_queue( queue) {}

         bool Writer::send( archive::output::Binary& archive, message_type_type type)
         {
            return local::write( archive, type, m_queue, ipc::receive::Queue::cNoBlocking);
         }


			Reader::Reader( ipc::receive::Queue& queue) : m_queue( queue) {}

			bool Reader::consume()
			{
				ipc::message::Transport transport;

				bool fetched = false;

				while( m_queue( transport, ipc::receive::Queue::cNoBlocking))
				{
					local::messageCache().insert( local::messageCache().end(), transport);
					fetched = true;
				}
				return fetched;

			}

			bool Reader::correlate( archive::input::Binary& archive, message_type_type type)
			{
				//
				// empty queue first
				//
				consume();

				//
				//	check if we can find a message with the type in cache
				//
				local::cache_type::iterator findIter = std::find_if(
						local::messageCache().begin(),
						local::messageCache().end(),
						local::MessageTypeInCache( type));

				if( findIter != local::messageCache().end())
				{
					//
					// We know the full message lies in cache, use the "blocking" correlate implementation
					// Though, first we have to find the first message in the cache that correspond to
					// the same correlation id (since we searched for the last)
					//
					findIter = std::find_if(
						local::messageCache().begin(),
						local::messageCache().end(),
						local::Correlation( findIter->m_payload.m_header.m_correlation));

					if( findIter != local::messageCache().end())
					{
						local::correlate( m_queue, archive, findIter);
					}
					else
					{
						//
						// Somehow the cache contains inconsistent data...
						//
						throw exception::NotReallySureWhatToNameThisException();
					}

					return true;
				}

				return false;
			}
		}








	} // queue
} // casual


