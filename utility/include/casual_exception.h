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
#include "xatmi.h"

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

		namespace xatmi
		{

		   struct Base : public std::runtime_error
         {
            Base( const std::string& description)
               : std::runtime_error( description) {}

            virtual int code() const throw() = 0;
         };


		   template< int xatmi_error>
		   struct basic_exeption : public Base
		   {
		      basic_exeption( const std::string& description)
		         : Base( description) {}

		      basic_exeption()
               : Base( "TODO") {}

		      int code() const throw() { return xatmi_error;}
		   };

		   typedef basic_exeption< TPENOENT> NoEntry;

		   typedef basic_exeption< TPEBLOCK> NoMessage;

		   typedef basic_exeption< TPETIME> Timeout;

		   typedef basic_exeption< TPEBADDESC> InvalidDescriptor;


		}

	}

}




#endif /* CASUAL_EXCEPTION_H_ */
