//!
//! casual_ipc_messages.h
//!
//! Created on: Apr 25, 2012
//!     Author: Lazan
//!

#ifndef CASUAL_MESSAGES_H_
#define CASUAL_MESSAGES_H_

#include "casual_ipc.h"


#include "casual_utility_platform.h"

#include <vector>



namespace casual
{
	namespace message
	{
		struct Service
		{
			std::string name;

			template< typename A>
			void serialize( A& archive)
			{
				archive & name;
			}
		};

		struct ServerConnect
		{

			enum
			{
				message_type = 2
			};

			ServerConnect() : pid( utility::platform::getProcessId()) {}

			std::string serverPath;
			ipc::message::Transport::queue_key_type queue_key;
			utility::platform::pid_type pid;
			std::vector< Service> services;


			template< typename A>
			void serialize( A& archive)
			{
				archive & serverPath;
				archive & queue_key;
				archive & pid;
				archive & services;
			}
		};




		//!
		//! Deduce witch type of message it is.
		//!
		template< typename M>
		ipc::message::Transport::message_type_type type( const M& message)
		{
			return M::message_type;
		}


	}

}



#endif /* CASUAL_IPC_MESSAGES_H_ */
