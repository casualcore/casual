//!
//! casual_utility_platform.h
//!
//! Created on: Jun 14, 2012
//!     Author: Lazan
//!

#ifndef CASUAL_UTILITY_PLATFORM_H_
#define CASUAL_UTILITY_PLATFORM_H_


// ipc
#include <sys/msg.h>
#include <sys/ipc.h>

// size_t
#include <cstddef>

//uuid
#include <uuid/uuid.h>

// longjump
#include <setjmp.h>

// time
#include <time.h>

#include <string.h>

namespace casual
{
	namespace utility
	{
		namespace platform
		{
			//
			// Some os-specific if-defs?
			//

			//
			// System V start
			//

			//
			// ipc
			//
			typedef int queue_id_type;
			typedef key_t queue_key_type;
			typedef long message_type_type;


			const std::size_t message_size = 2048;

			//
			// uuid
			//
			typedef uuid_t uuid_type;

			typedef pid_t pid_type;

			//
			// long jump
			//
			typedef jmp_buf long_jump_buffer_type;

			//
			// time
			//
			typedef time_t seconds_type;


			enum ipc_flags
			{
				cIPC_NO_WAIT = IPC_NOWAIT
			};


			pid_type getProcessId();








		} // platform
	} // utility
} // casual



#endif /* CASUAL_UTILITY_PLATFORM_H_ */
