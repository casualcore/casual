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

namespace casual
{
	namespace queue
	{
		class Reader
		{
		public:
			typedef ipc::message::Transport::message_type_type message_type_type;

			Reader( ipc::receive::Queue& queue);


			message_type_type next();


		private:
			ipc::receive::Queue& m_queue;

		};


		class Writer
		{
		public:
			Writer( ipc::send::Queue& queue) : m_queue( queue) {}

			template< typename M>
			void operator () ( M& message)
			{
				utility::Uuid correlation;
				ipc::message::Transport transport;

				transport.m_payload.m_type = message::type( message);
				transport.m_payload.m_header.m_correlation = correlation.get();

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
					transport.m_payload.m_header.m_count = count - index;

					const std::size_t offset = index *  ipc::message::Transport::payload_max_size;
					const std::size_t length =
							archive.get().size() - offset < ipc::message::Transport::payload_max_size ?
									archive.get().size() - offset : ipc::message::Transport::payload_max_size;

					//
					// Copy payload
					//
					std::copy(
						archive.get().begin() + offset,
						archive.get().begin() + length,
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
