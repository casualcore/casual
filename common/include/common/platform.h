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

// syslog
#include <syslog.h>

// signal
#include <signal.h>

// alarm
#include <unistd.h>


#include <string.h>


//
// std
//
#include <string>

namespace casual
{
	namespace common
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
			typedef char uuid_string_type[ 37];

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



			enum logger_priority
			{
				//! system is unusable
				cLOG_emergency = LOG_EMERG,
				//! action must be taken immediately
				cLOG_alert = LOG_ALERT,
				//! critical conditions
				cLOG_critical = LOG_CRIT,
				//! error conditions
				cLOG_error= LOG_ERR,
				//! warning conditions
				cLOG_warning = LOG_WARNING,
				//! normal, but significant, condition
				cLOG_notice = LOG_NOTICE,
				//! informational message
				cLOG_info = LOG_INFO,
				//! debug-level message
				cLOG_debug = LOG_DEBUG
			};

			typedef int signal_type;

			static const signal_type cSignal_Alarm = SIGALRM;
			static const signal_type cSignal_Terminate = SIGTERM;
			static const signal_type cSignal_Kill = SIGKILL;
			static const signal_type cSignal_Quit = SIGQUIT;
			static const signal_type cSignal_Interupt = SIGINT;

			std::string getSignalDescription( signal_type);

		} // platform
	} // utility
} // casual



#endif /* CASUAL_UTILITY_PLATFORM_H_ */
