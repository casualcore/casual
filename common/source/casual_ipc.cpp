//!
//! casual_ipc.cpp
//!
//! Created on: Mar 27, 2012
//!     Author: Lazan
//!

#include "casual_ipc.h"


namespace casual
{
	namespace ipc
	{
		Queue::Queue(queue_key_type key)
			: m_id( msgget( key, 0666))
		{

		}

		void Queue::send( message::Transport& message) const
		{

		}



	}
}






