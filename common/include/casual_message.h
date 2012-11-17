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
		   typedef int Seconds;

		   Service() : timeout( 0) {}

		   explicit Service( const std::string& name_) : name( name_), timeout( 0) {}

			std::string name;
			Seconds timeout;

			template< typename A>
			void marshal( A& archive)
			{
				archive & name;
				archive & timeout;
			}
		};

		//!
		//! Represents id for a server.
		//!
		struct ServerId
		{
		   typedef utility::platform::pid_type pid_type;
		   typedef ipc::message::Transport::queue_key_type queue_key_type;

			ServerId() : pid( utility::platform::getProcessId()) {}

			queue_key_type queue_key;
			pid_type pid;

			template< typename A>
			void marshal( A& archive)
			{
				archive & queue_key;
				archive & pid;
			}
		};


		struct ServerDisconnect
      {
         enum
         {
            message_type = 3
         };

         ServerId serverId;

         template< typename A>
         void marshal( A& archive)
         {
            archive & serverId;
         }
      };

		struct ServiceAdvertise
      {
         enum
         {
            message_type = 4
         };

         std::string serverPath;
         ServerId serverId;
         std::vector< Service> services;

         template< typename A>
         void marshal( A& archive)
         {
            archive & serverPath;
            archive & serverId;
            archive & services;
         }
      };

		struct ServiceUnadvertise
      {
         enum
         {
            message_type = 5
         };

         ServerId serverId;
         std::vector< Service> services;

         template< typename A>
         void marshal( A& archive)
         {
            archive & serverId;
            archive & services;
         }
      };


		//!
		//! Represent "service-name-lookup" request.
		//! TODO: need a better name?
		//!
		struct ServiceRequest
		{
			enum
			{
				message_type = 10
			};

			std::string requested;
			std::string current;
			ServerId server;

			template< typename A>
			void marshal( A& archive)
			{
				archive & requested;
				archive & current;
				archive & server;
			}
		};


		//!
      //! Represent "service-name-lookup" response.
      //! TODO: need a better name?
      //!
		struct ServiceResponse
		{

			enum
			{
				message_type = 11
			};

			Service service;

			std::vector< ServerId> server;

			template< typename A>
			void marshal( A& archive)
			{
				archive & service;
				archive & server;
			}
		};

		//!
		//! Represents a service call. via tp(a)call
		//!
		struct ServiceCall
		{
			enum
			{
				message_type = 20
			};

			ServiceCall( buffer::Buffer& buffer) : callDescriptor( 0),  m_buffer( buffer) {}

			int callDescriptor;
			Service service;
			ServerId reply;

			buffer::Buffer& buffer()
			{
				return m_buffer;
			}

			const buffer::Buffer& buffer() const
         {
            return m_buffer;
         }

			template< typename A>
			void marshal( A& archive)
			{
				archive & callDescriptor;
				archive & service;
				archive & reply;
				archive & m_buffer;
			}

		private:
			buffer::Buffer& m_buffer;
		};

		//!
		//! Represent service reply.
		//!
		struct ServiceReply
		{
			enum
			{
				message_type = 21
			};

			ServiceReply() : callDescriptor( 0), returnValue( 0), userReturnCode( 0), m_buffer( 0) {}

			ServiceReply( buffer::Buffer& buffer) : callDescriptor( 0), returnValue( 0), userReturnCode( 0), m_buffer( &buffer) {}

			void setBuffer( buffer::Buffer& buffer)
			{
				m_buffer = &buffer;
			}

			buffer::Buffer& getBuffer()
			{
				return *m_buffer;
			}


			int callDescriptor;
			int returnValue;
			long userReturnCode;

			template< typename A>
			void marshal( A& archive)
			{
				if( m_buffer == 0)
				{
					throw exception::xatmi::SystemError( "Not a valid buffer for ServiceReply");
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
		//!
		//!
		struct ServiceACK
      {
         enum
         {
            message_type = 22
         };

         typedef long Microseconds;

         std::string service;
         ServerId server;
         Microseconds time;

         template< typename A>
         void marshal( A& archive)
         {
            archive & service;
            archive & server;
            archive & time;
         }
      };

		//!
		//! Used to advertise the monitorserver
		//!
		struct MonitorAdvertise
		{
			enum
			{
				message_type = 30
			};

			ServerId serverId;

			template<typename A>
			void marshal(A& archive)
			{
				archive & serverId;
			}
		};

		//!
		//! Used to unadvertise the monitorserver
		//!
		struct MonitorUnadvertise
		{
			enum
			{
				message_type = 31
			};

			ServerId serverId;

			template<typename A>
			void marshal(A& archive)
			{
				archive & serverId;
			}
		};

		//!
		//! Call to monitorserver with statistics
		//!
		struct MonitorCall
        {
			enum
			{
				message_type = 32
			};

			std::string parentService;
			std::string service;
			std::string callChainId;
			std::string transactionId;
			std::string startTime;
			std::string endTime;

			template< typename A>
			void marshal( A& archive)
			{
				archive & parentService;
				archive & service;
				archive & callChainId;
				archive & transactionId;
				archive & startTime;
				archive & endTime;
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


	} // message
} // casual



#endif /* CASUAL_IPC_MESSAGES_H_ */
