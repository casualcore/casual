//!
//! casual_exception.h
//!
//! Created on: Apr 29, 2012
//!     Author: Lazan
//!

#ifndef CASUAL_EXCEPTION_H_
#define CASUAL_EXCEPTION_H_

#include <stdexcept>
#include <string>

#include "casual_utility_platform.h"

namespace casual
{
	namespace exception
	{

		//
		// Serves as a placeholder for later correct exception, whith hopefully a good name...
		//
		struct NotReallySureWhatToNameThisException : public std::exception {};

		struct MemoryNotFound : public std::exception {};


		struct EnvironmentVariableNotFound : public std::runtime_error
		{
			EnvironmentVariableNotFound( const std::string& description)
				: std::runtime_error( description) {}
		};

		struct QueueFailed : public std::runtime_error
		{
			QueueFailed( const std::string& description)
				: std::runtime_error( description) {}
		};

		struct QueueSend : public std::runtime_error
		{
			QueueSend( const std::string& description)
				: std::runtime_error( description) {}
		};

		struct QueueReceive : public std::runtime_error
		{
			QueueReceive( const std::string& description)
				: std::runtime_error( description) {}
		};

		namespace signal
		{
			struct Base : public std::runtime_error
			{
				Base( const std::string& description)
					: std::runtime_error( description) {}

				virtual utility::platform::signal_type getSignal() const = 0;
			};

			template< utility::platform::signal_type signal>
			struct basic_signal : public Base
			{
			   basic_signal( const std::string& description)
			      : Base( description) {}

			   basic_signal()
			      : Base( utility::platform::getSignalDescription( signal)) {}

			   utility::platform::signal_type getSignal() const
			   {
			      return signal;
			   }
			};

			typedef basic_signal< utility::platform::cSignal_Alarm> Timeout;

			typedef basic_signal< utility::platform::cSignal_Alarm> Terminate;


		}

		namespace service
		{
		   struct NoEntry : public std::runtime_error
         {
		      NoEntry( const std::string& description)
               : std::runtime_error( description) {}
         };

		   struct NoMessage : public std::runtime_error
		   {
		      NoMessage()
		         : std::runtime_error( "No messages") {}
		   };

		   struct Timeout : public std::runtime_error
         {
		      Timeout()
               : std::runtime_error( "Timeout occurred") {}
         };

		   struct InvalidDescriptor : public std::runtime_error
         {
		      InvalidDescriptor()
               : std::runtime_error( "Invalid descriptor") {}
         };

		}

	}

}




#endif /* CASUAL_EXCEPTION_H_ */
