//!
//! casual_ipc.h
//!
//! Created on: Mar 27, 2012
//!     Author: Lazan
//!

#ifndef CASUAL_IPC_H_
#define CASUAL_IPC_H_

#include "xatmi.h"

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

			enum
			{
				message_size = 2048
			};

		}




		namespace message
		{
			struct Transport
			{
				long m_type;
				char m_payload[ platform::message_size];
			};

			struct ServiceRequest
			{
				char serviceName[ XATMI_SERVICE_NAME_LENGTH];
				platform::queue_id_type responseQueue;
			};

			void serialize( const ServiceRequest& message, Transport& transport)
			{

			}


		}




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




		struct Queue
		{
			typedef platform::queue_id_type queue_id_type;
			typedef platform::queue_key_type queue_key_type;

			Queue( queue_key_type key);

			void send( message::Transport& message) const;

			void receive( message::Transport& message) const;

		private:
			queue_id_type m_id;

		};

	}

}




#endif /* CASUAL_IPC_H_ */
