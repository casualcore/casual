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

#include "common/string.h"
#include "common/signal.h"

#include <stdexcept>
#include <string>
#include <ostream>

#include <xatmi.h>
#include <tx.h>

namespace casual
{
   namespace common
   {
      namespace exception
      {

         struct Base : public std::runtime_error
         {
            Base( const std::string& description)
               : std::runtime_error( description) {}

            friend std::ostream& operator << ( std::ostream& out, const Base& exception)
            {
               return out << exception.what();
            }
         };


         //
         // Serves as a placeholder for later correct exception, with hopefully a good name...
         //
         struct NotReallySureWhatToNameThisException : public Base
         {
            NotReallySureWhatToNameThisException( )
               : Base( "NotReallySureWhatToNameThisException") {}

            NotReallySureWhatToNameThisException( const std::string& information)
               : Base( information) {}
         };

         struct MemoryNotFound : public std::exception {};

         struct FileNotExist : public Base
         {
            FileNotExist( const std::string& file)
               : Base( "file does not exists: " + file) {}
         };


         struct EnvironmentVariableNotFound : public Base
         {
            EnvironmentVariableNotFound( const std::string& description)
               : Base( "environment variable not found: " + description) {}
         };

         namespace invalid
         {
            struct Base : common::exception::Base
            {
               using common::exception::Base::Base;

            };

            struct Argument : Base
            {
               using Base::Base;
            };

         }

         struct QueueFailed : public Base
         {
            using Base::Base;
         };

         struct QueueSend : public Base
         {
            using Base::Base;
         };

         struct QueueReceive : public Base
         {
            using Base::Base;
         };

         namespace signal
         {
            struct Base : public exception::Base
            {
               using common::exception::Base::Base;

               virtual common::platform::signal_type getSignal() const = 0;
            };

            template< common::platform::signal_type signal>
            struct basic_signal : public Base
            {
               enum
               {
                  value = signal
               };
               basic_signal( const std::string& description)
                  : Base( description) {}

               basic_signal()
                  : Base( common::signal::type::string( signal)) {}

               common::platform::signal_type getSignal() const
               {
                  return value;
               }
            };

            typedef basic_signal< common::platform::cSignal_Alarm> Timeout;

            typedef basic_signal< common::platform::cSignal_Terminate> Terminate;

            typedef basic_signal< common::platform::cSignal_UserDefined> User;


            namespace child
            {
               typedef basic_signal< common::platform::cSignal_ChildTerminated> Terminate;
            } // child

         } // signal

         namespace code
         {
            struct Base : public exception::Base
            {
               using common::exception::Base::Base;

               virtual int code() const noexcept = 0;
               virtual int severity() const noexcept = 0;
            };


            template< int value, typename base_type>
            struct basic_exeption : public base_type
            {
               basic_exeption( const std::string& description)
                  : base_type( description) {}

               basic_exeption()
                  : base_type( "No additional information") {}

               int code() const noexcept { return value;}
            };
         } // code

         namespace severity
         {
            template< int value, typename base>
            struct basic_severity : public base
            {
               using base::base;

               int severity() const noexcept { return value;}
            };

            template< typename base>
            using Error = basic_severity< common::platform::cLOG_error, base>;

            template< typename base>
            using Information = basic_severity< common::platform::cLOG_info, base>;

            template< typename base>
            using User = basic_severity< common::platform::cLOG_debug, base>;

         } // severity

         namespace xatmi
         {
            struct Base : public code::Base
            {
               using code::Base::Base;
            };

            namespace severity
            {
               using Error = exception::severity::Error< Base>;
               using Information = exception::severity::Information< Base>;
               using User = exception::severity::User< Base>;
            }



            typedef code::basic_exeption< TPEBLOCK, severity::User> NoMessage;

            typedef code::basic_exeption< TPELIMIT, severity::Information> LimitReached;

            typedef code::basic_exeption< TPEINVAL, severity::User> InvalidArguments;

            typedef code::basic_exeption< TPEOS, severity::Error> OperatingSystemError;

            typedef code::basic_exeption< TPEPROTO, severity::Error> ProtocollError;

            namespace service
            {
               typedef code::basic_exeption< TPEBADDESC, severity::User> InvalidDescriptor;

               typedef code::basic_exeption< TPESVCERR, severity::Error> Error;

               typedef code::basic_exeption< TPESVCFAIL, severity::User> Fail;

               typedef code::basic_exeption< TPENOENT, severity::User> NoEntry;

               typedef code::basic_exeption< TPEMATCH, severity::User> AllreadyAdvertised;
            }

            typedef code::basic_exeption< TPESYSTEM, severity::Error> SystemError;

            typedef code::basic_exeption< TPETIME, severity::User> Timeout;

            typedef code::basic_exeption< TPETRAN, severity::User> TransactionNotSupported;

            typedef code::basic_exeption< TPGOTSIG, severity::Information> Signal;

            namespace buffer
            {

               typedef code::basic_exeption< TPEITYPE, severity::User> TypeNotSupported;

               typedef code::basic_exeption< TPEOTYPE, severity::User> TypeNotExpected;

            }

            namespace conversational
            {
               // TODO: Currently not supported...

              // #define TPEEVENT 22  <-- not supported right now

            }
         } // xatmi

         /*
          * No point using exception for XA - TX stuff, to much return values that needs to be
          * mapped to conform to the standard...
          *
         namespace tx
         {
            struct Base : public code::Base
            {
               using code::Base::Base;
            };

            namespace severity
            {
               using Error = exception::severity::Error< Base>;
               using Information = exception::severity::Information< Base>;
               using User = exception::severity::User< Base>;
            }

            using OutsideTransaction = code::basic_exeption< TX_OUTSIDE, severity::User>;
            using RolledBack = code::basic_exeption< TX_ROLLBACK, severity::Information>;
            using HeuristicallyCommitted = code::basic_exeption< TX_COMMITTED, severity::Information>;

            using Mixed = code::basic_exeption< TX_MIXED, severity::Information>;

            using Hazard = code::basic_exeption< TX_HAZARD, severity::Error>;

            using ProtocollError = code::basic_exeption< TX_PROTOCOL_ERROR, severity::User>;

            using Error = code::basic_exeption< TX_ERROR, severity::Error>;

            using Fail = code::basic_exeption< TX_FAIL, severity::Error>;

            using InvalidArguments = code::basic_exeption< TX_EINVAL, severity::User>;

            namespace no_begin
            {
               using RolledBack = code::basic_exeption< TX_ROLLBACK_NO_BEGIN, severity::User>;
               using Mixed = code::basic_exeption< TX_MIXED_NO_BEGIN, severity::Information>;
               using Haxard = code::basic_exeption< TX_HAZARD_NO_BEGIN, severity::Error>;
               using Committed = code::basic_exeption< TX_COMMITTED_NO_BEGIN, severity::User>;

            } // no_begin
         } // tx
         */

		} // exception
	} // utility
} // casual




#endif /* CASUAL_EXCEPTION_H_ */
