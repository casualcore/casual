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
			};

			struct Timeout : public Base
			{
				Timeout( const std::string& description)
					: Base( description) {}
			};

			struct Terminate : public Base
			{
				Terminate( const std::string& description)
					: Base( description) {}
			};


		}

		namespace service
		{
		   struct NoEntry : public std::runtime_error
         {
		      NoEntry( const std::string& description)
               : std::runtime_error( description) {}
         };

		}

	}

}




#endif /* CASUAL_EXCEPTION_H_ */
