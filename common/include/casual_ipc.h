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

#include <string>
#include <sys/msg.h>

namespace casual
{
	namespace ipc
	{

		//
		// some platform specific ifdefs?
		//

		//
		// System V start
		//
		namespace platform
		{

			typedef int queue_id_type;
			typedef key_t queue_key_type;


			const std::size_t message_size = 2048;


		}




		namespace message
		{
			struct Transport
			{
				Transport() : m_size( platform::message_size) {}

				struct Payload
				{
					long m_type;
					char m_payload[ platform::message_size];
				} m_payload;

				void* raw() { return &m_payload;}
				std::size_t size() { return m_size; }

			//private:
				std::size_t m_size;

			};

			/*
			struct ServiceRequest
			{
				char serviceName[ XATMI_SERVICE_NAME_LENGTH];
				platform::queue_id_type responseQueue;
			};

			inline void serialize( const ServiceRequest& message, Transport& transport)
			{

			}
			*/

		}



		/*
		struct ServiceCallMessage
		{
			struct
			{

				long flags;
				int cd;
			};

			std::string serviceName;


		};



		namespace traits
		{

			template< std::size_t value>
			struct basic_message_type
			{
				enum
				{
					type = value
				};
			};

			template< typename T>
			struct message_type;

			template<>
			struct message_type< ServiceCallMessage>
				: public basic_message_type< 2> {};
		}
		*/

		namespace internal
		{
			class base_queue
			{
			public:
				typedef platform::queue_id_type queue_id_type;
				typedef platform::queue_key_type queue_key_type;


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
				Queue( queue_key_type key);

				bool operator () ( message::Transport& message) const;
			};

		}

		namespace receive
		{
			class Queue : public internal::base_queue
			{
			public:

				typedef std::size_t Seconds;

				Queue();
				~Queue();

				bool operator () ( message::Transport& message) const;

				bool operator () ( message::Transport& message, Seconds timout) const;

			private:
				//!
				//! No value semantics
				//! @{
				Queue( const Queue&);
				Queue& operator = ( const Queue&);
				//! @}

				utility::file::ScopedPath m_scopedPath;
			};
		}


		send::Queue getBrokerQueue();

	}

}




#endif /* CASUAL_IPC_H_ */
