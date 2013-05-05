//!
//! casual_exception.h
//!
//! Created on: Apr 29, 2012
//!     Author: Lazan
//!

#ifndef CASUAL_EXCEPTION_H_
#define CASUAL_EXCEPTION_H_



#include "common/platform.h"
#include "common/error.h"

#include <stdexcept>
#include <string>

#include "xatmi.h"

namespace casual
{
   namespace utility
   {
      namespace exception
      {

         //
         // Serves as a placeholder for later correct exception, which hopefully a good name...
         //
         struct NotReallySureWhatToNameThisException : public std::exception {};

         struct MemoryNotFound : public std::exception {};


         struct EnvironmentVariableNotFound : public std::runtime_error
         {
            EnvironmentVariableNotFound( const std::string& description)
               : std::runtime_error( "environment variable not found: " + description) {}
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

            namespace severity
            {
               template< int severity>
               struct Severity : public Base
               {
                  Severity( const std::string& description)
                     : Base( description) {}
               };

               typedef Severity< utility::platform::cLOG_critical> Critical;
               typedef Severity< utility::platform::cLOG_info> Information;
               typedef Severity< utility::platform::cLOG_debug> User;

            }

            template< int xatmi_error, typename base_type>
            struct basic_exeption : public base_type
            {
               basic_exeption( const std::string& description)
                  : base_type( description) {}

               basic_exeption()
                  : base_type( "No additional information") {}

               int code() const throw() { return xatmi_error;}
            };




            typedef basic_exeption< TPEBLOCK, severity::User> NoMessage;

            typedef basic_exeption< TPELIMIT, severity::Information> LimitReached;

            typedef basic_exeption< TPEINVAL, severity::Information> InvalidArguments;

            typedef basic_exeption< TPEOS, severity::Critical> OperatingSystemError;

            typedef basic_exeption< TPEPROTO, severity::Critical> ProtocollError;

            namespace service
            {
               typedef basic_exeption< TPEBADDESC, severity::Information> InvalidDescriptor;

               typedef basic_exeption< TPESVCERR, severity::Critical> Error;

               typedef basic_exeption< TPESVCFAIL, severity::User> Fail;

               typedef basic_exeption< TPENOENT, severity::User> NoEntry;

               typedef basic_exeption< TPEMATCH, severity::User> AllreadyAdvertised;
            }

            typedef basic_exeption< TPESYSTEM, severity::Critical> SystemError;

            typedef basic_exeption< TPETIME, severity::User> Timeout;

            typedef basic_exeption< TPETRAN, severity::User> TransactionNotSupported;

            typedef basic_exeption< TPGOTSIG, severity::Information> Signal;

            namespace buffer
            {

               typedef basic_exeption< TPEITYPE, severity::Information> TypeNotSupported;

               typedef basic_exeption< TPEOTYPE, severity::Information> TypeNotExpected;

            }

            namespace conversational
            {
               // TODO: Currently not supported...

              // #define TPEEVENT 22  <-- not supported right now

            }

         } // xatmi
		} // exception
	} // utility
} // casual




#endif /* CASUAL_EXCEPTION_H_ */
