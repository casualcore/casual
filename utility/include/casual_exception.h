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


		struct Signal : public std::runtime_error
		{
			Signal( const std::string& description)
				: std::runtime_error( description) {}
		};

		struct SignalTerminate : public Signal
		{
			SignalTerminate( const std::string& description)
				: Signal( description) {}
		};

	}

}




#endif /* CASUAL_EXCEPTION_H_ */
