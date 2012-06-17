//!
//! casual_ipc.cpp
//!
//! Created on: Mar 27, 2012
//!     Author: Lazan
//!

#include "casual_ipc.h"

#include "casual_utility_environment.h"
#include "casual_error.h"
#include "casual_exception.h"
#include "casual_utility_signal.h"
#include "casual_utility_uuid.h"



#include <fstream>

#include <cstdio>
#include <unistd.h>
#include <errno.h>
#include <signal.h>


// temp
#include <iostream>




namespace casual
{
	namespace ipc
	{
		namespace internal
		{

			base_queue::queue_key_type base_queue::getKey() const
			{
				return m_key;
			}


		}

		namespace send
		{
			Queue::Queue(queue_key_type key)
			{
				m_key = key;
				m_id = msgget( m_key, 0220);

				if( m_id  == -1)
				{
					throw casual::exception::QueueFailed( error::stringFromErrno());
				}
			}


			bool Queue::operator () ( message::Transport& message) const
			{
				if( msgsnd( m_id, message.raw(), message.size(), 0) == -1)
				{
					throw casual::exception::QueueSend( error::stringFromErrno());
				}
				return true;
			}



		}

		namespace receive
		{

			Queue::Queue()
				: m_scopedPath( utility::environment::getTemporaryPath() + "/ipc_queue_" + utility::Uuid().getString())
			{
				//
				// Create queue
				//
				std::ofstream ipcQueueFile( m_scopedPath.path().c_str());

				m_key = ftok( m_scopedPath.path().c_str(), 'X');
				m_id = msgget( m_key, 0660 | IPC_CREAT);

 				ipcQueueFile << "key: " << m_key << "\nid:  " << m_id << std::endl;

				if( m_id  == -1)
				{
					throw casual::exception::QueueFailed( error::stringFromErrno());
				}

			}


			Queue::~Queue()
			{
				//
				// Destroy queue
				//
				msgctl( m_id, IPC_RMID, 0);
			}



			bool Queue::receive( message::Transport& message, const long flags) const
			{
				ssize_t result = msgrcv( m_id, message.raw(), message.size(), 0, 0);

				if( result == -1)
				{
					if( errno == EINTR)
					{
						utility::signal::handle();
						return false;
					}
					throw exception::QueueReceive( error::stringFromErrno());
				}

				message.size( result);

				return true;
			}

			/*

			bool Queue::operator () ( message::Transport& message, Seconds timout) const
			{
				//
				// set signal for timout
				//
				alarm( timout);

				bool result = operator () ( message);

				//
				// cancel the alarm
				//
				alarm( 0);

				return result;
			}
			*/

		}



		send::Queue getBrokerQueue()
		{
			static const std::string brokerFile = utility::environment::getBrokerQueueFileName();

			std::ifstream file( brokerFile.c_str());

			send::Queue::queue_key_type key;
			file >> key;

			return send::Queue( key);

		}


	}
}






