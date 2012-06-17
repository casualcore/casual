//!
//! casual_queue.h
//!
//! Created on: Jun 9, 2012
//!     Author: Lazan
//!

#ifndef CASUAL_QUEUE_H_
#define CASUAL_QUEUE_H_

#include "casual_ipc.h"
#include "casual_message.h"
#include "casual_archive.h"
#include "casual_utility_signal.h"


#include <list>

namespace casual
{
	namespace queue
	{
		//!
		//! TODO: rewrite... lift out the cache somehow... The semantics is quite different between
		//! blocking and non-blocking -> different types?
		class Reader
		{
		public:
			typedef ipc::message::Transport transport_type;
			typedef transport_type::message_type_type message_type_type;
			typedef std::size_t Seconds;

			typedef std::list< ipc::message::Transport> cache_type;

			Reader( ipc::receive::Queue& queue);

			//!
			//! Gets the next message type.
			//!
			message_type_type next();

			//!
			//! Tries to read a specific message from the queue.
			//! @attention use next() to determine which message is ready to read.
			//!
			template< typename M>
			void operator () ( M& message)
			{
				message_type_type type = message::type( message);

				archive::input::Binary archive;

				correlate( archive, type);

				archive >> message;
			}

			template< typename M>
			bool fetch( M& message)
			{
				message_type_type type = message::type( message);

				archive::input::Binary archive;

				if( correlateNonBlock( archive, type))
				{
					archive >> message;
					return true;
				}
				return false;
			}

			//!
			//! Tries to read a specific message from the queue.
			//! @attention use next() to determine which message is ready to read.
			//!
			template< typename M>
			void operator () ( M& message, Seconds timeout)
			{
				utility::signal::scoped::Alarm alarm( timeout);

				this->operator ()( message);
			}


			//!
			//! Consumes all transport messages that is present on the ipc-queue, and
			//! stores these to cache.
			//!
			//! @note non blocking
			//!
			bool consume();


		private:

			void correlate( archive::input::Binary& archive, message_type_type type);

			bool correlateNonBlock( archive::input::Binary& archive, message_type_type type);


			void internal_correlate( archive::input::Binary& archive, cache_type::iterator start);


			//!
			//! finds and return the first transport-message of a specified type.
			//!
			cache_type::iterator first( message_type_type type);

			cache_type::iterator fetchIfEmpty( cache_type::iterator start);

			cache_type& messageCache();

			ipc::receive::Queue& m_queue;

		};


		class Writer
		{
		public:
			Writer( ipc::send::Queue& queue) : m_queue( queue) {}

			//!
			//! Sends/Writes a message to the queue. which can result in several
			//! actual ipc-messages.
			//!
			template< typename M>
			void operator () ( M& message)
			{
				utility::Uuid correlation;
				ipc::message::Transport transport;

				transport.m_payload.m_type = message::type( message);
				correlation.get( transport.m_payload.m_header.m_correlation);

				//
				// Serialize the message
				//
				archive::output::Binary archive;
				archive << message;

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

					m_queue( transport);
				}
			}

		private:
			ipc::send::Queue& m_queue;

		};


	}

}


#endif /* CASUAL_QUEUE_H_ */
