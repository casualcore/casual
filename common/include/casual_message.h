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

#include "casual_exception.h"
#include "casual_buffer_context.h"

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

		struct ServerId
		{
			ServerId() : pid( utility::platform::getProcessId()) {}

			ipc::message::Transport::queue_key_type queue_key;
			utility::platform::pid_type pid;

			template< typename A>
			void serialize( A& archive)
			{
				archive & queue_key;
				archive & pid;
			}
		};

		struct ServerConnect
		{

			enum
			{
				message_type = 2
			};



			std::string serverPath;
			ServerId serverId;
			std::vector< Service> services;

			template< typename A>
			void serialize( A& archive)
			{
				archive & serverPath;
				archive & serverId;
				archive & services;
			}
		};


		struct ServiceRequest
		{
			enum
			{
				message_type = 3
			};

			std::string requested;
			std::string current;
			ServerId server;

			template< typename A>
			void serialize( A& archive)
			{
				archive & requested;
				archive & current;
				archive & server;
			}
		};


		struct ServiceResponse
		{
			enum
			{
				message_type = 4
			};

			std::string requested;
			std::vector< ServerId> server;

			template< typename A>
			void serialize( A& archive)
			{
				archive & requested;
				archive & server;
			}
		};

		struct ServiceCall
		{
			enum
			{
				message_type = 5
			};

			ServiceCall( buffer::Buffer& buffer) : timeout( 0),  m_buffer( buffer) {}

			int callDescriptor;
			std::string service;
			std::size_t timeout;
			ServerId reply;

			buffer::Buffer& buffer()
			{
				return m_buffer;
			}

			template< typename A>
			void serialize( A& archive)
			{
				archive & callDescriptor;
				archive & service;
				archive & timeout;
				archive & reply;
				archive & m_buffer;
			}

		private:
			buffer::Buffer& m_buffer;
		};

		struct ServiceReply
		{
			enum
			{
				message_type = 6
			};

			ServiceReply() : m_buffer( 0) {}

			ServiceReply( buffer::Buffer& buffer) : m_buffer( &buffer) {}

			void setBuffer( buffer::Buffer& buffer)
			{
				m_buffer = &buffer;
			}

			buffer::Buffer& getBuffer()
			{
				return *m_buffer;
			}

			/*
			void releaseBuffer()
			{
				m_buffer = 0;
			}
			*/

			int callDescriptor;
			int returnValue;
			long userReturnCode;

			template< typename A>
			void serialize( A& archive)
			{
				if( m_buffer == 0)
				{
					throw exception::NotReallySureWhatToNameThisException();
				}
				archive & callDescriptor;
				archive & returnValue;
				archive & userReturnCode;
				archive & *m_buffer;
			}

		private:
			buffer::Buffer* m_buffer;


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
