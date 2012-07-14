//!
//! casual_ipc.h
//!
//! Created on: Mar 27, 2012
//!     Author: Lazan
//!

#ifndef CASUAL_IPC_H_
#define CASUAL_IPC_H_

#include "xatmi.h"

#include "casual_utility_file.h"
#include "casual_utility_uuid.h"
#include "casual_utility_platform.h"

#include <string>

namespace casual
{
	namespace ipc
	{

		namespace message
		{
			struct Transport
			{
				typedef utility::platform::queue_id_type queue_id_type;
				typedef utility::platform::queue_key_type queue_key_type;
				typedef utility::platform::message_type_type message_type_type;
				typedef utility::Uuid::uuid_type correalation_type;

				struct Header
				{
					correalation_type m_correlation;
					std::size_t m_pid;
					long m_count;

				};

				enum
				{
					message_max_size = utility::platform::message_size,
					payload_max_size = utility::platform::message_size - sizeof( Header)
				};

				Transport() : m_size( message_max_size)
				{
					memset( &m_payload, 0, sizeof( Payload));
				}

				struct Payload
				{

					message_type_type m_type;

					Header m_header;

					char m_payload[ payload_max_size];
				} m_payload;

				void* raw() { return &m_payload;}

				std::size_t size() { return m_size; }

				void size( std::size_t size)
				{
					m_size = size;
				}

				std::size_t paylodSize() { return m_size - sizeof( Header);}
				void paylodSize( std::size_t size) { m_size = size +  sizeof( Header);}

			private:

				//Transport( const Transport&);
				//Transport& operator = ( const Transport&);

				std::size_t m_size;

			};

		}


		namespace internal
		{
			class base_queue
			{
			public:
				typedef utility::platform::queue_id_type queue_id_type;
				typedef utility::platform::queue_key_type queue_key_type;


				queue_key_type getKey() const;

			protected:
				queue_key_type m_key;
				queue_id_type m_id;
			};

		}


		namespace send
		{

			class Queue : public internal::base_queue
			{
			public:

			   enum
            {
               cNoBlocking = utility::platform::cIPC_NO_WAIT
            };

				Queue( queue_key_type key);

				bool operator () ( message::Transport& message) const
				{
				   return send( message, 0);
				}

				bool operator () ( message::Transport& message, const long flags) const
            {
               return send( message, flags);
            }

			private:

				bool send( message::Transport& message, const long flags) const;
			};

		}

		namespace receive
		{
			class Queue : public internal::base_queue
			{
			public:

				enum
				{
					cNoBlocking = utility::platform::cIPC_NO_WAIT
				};

				Queue();
				~Queue();

				bool operator () ( message::Transport& message) const
				{
					return receive( message, 0);
				}

				bool operator () ( message::Transport& message, const long flags) const
				{
					// TODO: constraint on flags?
					return receive( message, flags);
				}

			private:
				//!
				//! No value semantics
				//! @{
				Queue( const Queue&);
				Queue& operator = ( const Queue&);
				//! @}


				bool receive( message::Transport& message, const long flags) const;

				utility::file::ScopedPath m_scopedPath;
			};
		}


		send::Queue getBrokerQueue();

	}

}




#endif /* CASUAL_IPC_H_ */
