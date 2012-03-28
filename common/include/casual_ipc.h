//!
//! casual_ipc.h
//!
//! Created on: Mar 27, 2012
//!     Author: Lazan
//!

#ifndef CASUAL_IPC_H_
#define CASUAL_IPC_H_

#include <string>

namespace casual
{
	namespace ipc
	{

		namespace platform
		{
			//
			// Some ifdefs?
			//
			const size_t message_size = 2048;

			struct queue_traits
			{
				typedef int queue_id_type;
			};

		}

		namespace message
		{

			template< std::size_t size>
			struct basic_message
			{
				long m_type;
				char m_payload[ size];
			};

			typedef basic_message< platform::message_size> Message;

			namespace typed
			{



			}
		}




		template< typename T>
		struct basic_queue
		{
			typedef T traits_type;
			typedef typename traits_type queue_id_type;

			basic_queue( queue_id_type id) : m_id( id) {}

			queue_id_type m_id;

		};

		typedef basic_queue< platform::queue_traits> Queue;

	}

}




#endif /* CASUAL_IPC_H_ */
