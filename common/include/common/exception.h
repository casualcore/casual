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
#include "common/log.h"

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
         using nip_type = std::tuple< std::string, std::string>;

         template< typename T>
         nip_type make_nip( std::string name, const T& information)
         {
            return std::make_tuple( std::move( name), to_string( information));
         }

         struct base : std::exception
         {


            base( std::string description);

            template< typename... Args>
            base( std::string description, Args&&... information)
               : base( std::move( description), construct( {}, std::forward<Args>( information)...)) {}


            const char* what() const noexcept override;
            const std::string& description() const noexcept;


            friend std::ostream& operator << ( std::ostream& out, const base& exception);

         private:
            using nip_type = std::tuple< std::string, std::string>;

            struct Convert
            {
               nip_type operator() ( nip_type nip) { return nip;}

               template< typename T>
               nip_type operator() ( T value) { return make_nip( "value", value);}
            };

            base( std::string description, std::vector< nip_type> information);

            static std::vector< nip_type> construct( std::vector< nip_type> nips)
            {
               return nips;
            }

            template< typename I, typename... Args>
            static std::vector< nip_type> construct( std::vector< nip_type> nips, I&& infomation, Args&&... args)
            {
               nips.push_back( Convert{}( std::forward< I>( infomation)));
               return construct( std::move( nips), std::forward< Args>( args)...);
            }

            std::string m_description;
         };

         //!
         //! something has gone wrong internally in casual.
         //!
         //! @todo another name?
         //!
         struct Casual : base
         {
            using base::base;
         };




         struct Base : public std::runtime_error
         {

            Base( const std::string& description)
               : std::runtime_error( description) {}

            Base( const std::string& description, const char* file, decltype( __LINE__) line)
               : std::runtime_error( description + " - " + file + ":" + std::to_string( line)) {}

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


         struct Shutdown : base
         {
            using base::base;
         };

         namespace invalid
         {
            struct Base : common::exception::base
            {
               using common::exception::base::base;

            };

            struct Argument : Base
            {
               using Base::Base;
            };

            struct Configuration : Base
            {
               using Base::Base;
            };

            struct Process : Base
            {
               using Base::Base;
            };

            struct File : Base
            {
               using Base::Base;
            };
         }

         namespace limit
         {
            struct Memory : base
            {
               using base::base;
            };

         } // limit

         namespace queue
         {
            struct Unavailable : base
            {
               using base::base;
            };

         } // queue


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

               basic_signal( const std::string& description, const char* file, decltype( __LINE__) line)
                  : Base( description, file, line) {}

               common::platform::signal_type getSignal() const
               {
                  return value;
               }
            };

            typedef basic_signal< common::signal::alarm> Timeout;

            typedef basic_signal< common::signal::terminate> Terminate;

            typedef basic_signal< common::signal::user> User;


            namespace child
            {
               typedef basic_signal< common::signal::child> Terminate;
            } // child

         } // signal

         namespace code
         {
            struct Base : public exception::Base
            {
               using common::exception::Base::Base;

               virtual int code() const noexcept = 0;
               virtual log::category::Type category() const noexcept = 0;
            };


            template< int value, typename base_type>
            struct basic_exeption : public base_type
            {
               enum
               {
                  code_value = value
               };

               basic_exeption( const std::string& description)
                  : base_type( description) {}

               basic_exeption( const std::string& description, const char* file, decltype( __LINE__) line)
                  : base_type( description, file, line) {}

               basic_exeption()
                  : base_type( "No additional information") {}

               int code() const noexcept { return code_value;}
            };
         } // code

         namespace category
         {
            template< log::category::Type value, typename base>
            struct basic_category : public base
            {
               using base::base;

               enum
               {
                  category_value = static_cast< long>( value)
               };

               log::category::Type category() const noexcept { return value;}
            };


            template< typename base>
            using Error = basic_category< log::category::Type::error, base>;

            template< typename base>
            using Warning = basic_category< log::category::Type::warning, base>;

            template< typename base>
            using Information = basic_category< log::category::Type::information, base>;

            template< typename base>
            using User = basic_category< log::category::Type::debug, base>;

         } // category

         namespace xatmi
         {
            struct Base : public code::Base
            {
               using code::Base::Base;
            };

            namespace category
            {
               using Error = exception::category::Error< Base>;
               using Warning = exception::category::Warning< Base>;
               using Information = exception::category::Information< Base>;
               using User = exception::category::User< Base>;
            }



            typedef code::basic_exeption< TPEBLOCK, category::User> NoMessage;

            typedef code::basic_exeption< TPELIMIT, category::Information> LimitReached;

            typedef code::basic_exeption< TPEINVAL, category::User> InvalidArguments;

            typedef code::basic_exeption< TPEOS, category::Error> OperatingSystemError;

            typedef code::basic_exeption< TPEPROTO, category::Error> ProtocollError;

            namespace service
            {
               typedef code::basic_exeption< TPEBADDESC, category::User> InvalidDescriptor;

               typedef code::basic_exeption< TPESVCERR, category::Error> Error;

               typedef code::basic_exeption< TPESVCFAIL, category::User> Fail;

               typedef code::basic_exeption< TPENOENT, category::User> NoEntry;

               typedef code::basic_exeption< TPEMATCH, category::User> AllreadyAdvertised;
            }

            typedef code::basic_exeption< TPESYSTEM, category::Error> SystemError;

            typedef code::basic_exeption< TPETIME, category::User> Timeout;

            typedef code::basic_exeption< TPETRAN, category::User> TransactionNotSupported;

            typedef code::basic_exeption< TPGOTSIG, category::Information> Signal;

            namespace buffer
            {

               typedef code::basic_exeption< TPEITYPE, category::User> TypeNotSupported;

               typedef code::basic_exeption< TPEOTYPE, category::User> TypeNotExpected;

            }

            namespace conversational
            {
               // TODO: Currently not supported...

              // #define TPEEVENT 22  <-- not supported right now

            }
         } // xatmi

         namespace tx
         {
            struct base : public common::exception::base
            {
               using common::exception::base::base;
            };

            struct Fail : base
            {
               using base::base;
            };

            struct Protocoll : base
            {
               using base::base;
            };

            struct Argument : base
            {
               using base::base;
            };

            struct Outside : base
            {
               using base::base;
            };

            struct Error : base
            {
               using base::base;
            };


            namespace no
            {
               struct Begin : base
               {
                  using base::base;
               };

               struct Support : base
               {
                  using base::base;
               };
            } // no
         }


		} // exception
	} // common
} // casual


#define CASUAL_NIP( information) \
   casual::common::exception::make_nip( #information, information)




#endif /* CASUAL_EXCEPTION_H_ */
