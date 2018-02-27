//!
//! casual
//!

#ifndef CASUAL_COMMON_EXCEPTION_SYSTEM_H_
#define CASUAL_COMMON_EXCEPTION_SYSTEM_H_

#include "common/code/system.h"
#include "common/exception/common.h"

namespace casual
{
   namespace common
   {
      namespace exception 
      {
         namespace system 
         {

            using exception = common::exception::base_error< code::system>;

            template< code::system error>
            using base = common::exception::basic_error< exception, error>;
            

            namespace invalid
            {
               using Argument = system::base< code::system::invalid_argument>;

               using File = system::base< code::system::no_such_file_or_directory>;

               using Process = system::base< code::system::no_such_process>;

            } // invalid

            namespace communication
            {
               struct Error : system::exception
               {
                  using system::exception::exception;
               };

               namespace no
               {
                  struct Message : Error
                  {
                     using Error::Error;
                  };

                  namespace message 
                  {
                     template< code::system error>
                     using basic = common::exception::basic_error< Message, error>;

                     using Absent = basic< code::system::no_message>;
                     using Resource = basic< code::system::resource_unavailable_try_again>;

                  } // message 
               } // no

               struct Unavailable : Error
               {
                  using Error::Error;
               };

               namespace unavailable
               {
                  template< code::system error>
                  using basic = common::exception::basic_error< Unavailable, error>;

                  using Removed = basic< code::system::identifier_removed>;
                  using Reset = basic< code::system::connection_reset>;
                  using Pipe = basic< code::system::broken_pipe>;
                 
                  namespace no 
                  {
                     using Connect = basic< code::system::not_connected>;
                  } // not 
               } // unavailable

               using Refused = common::exception::basic_error< Error, code::system::connection_refused>;

               using Protocol = common::exception::basic_error< Error, code::system::protocol_error>;

            } // communication

            //!
            //! throws a std::system_error based on errno
            //! @{
            void throw_from_errno();
            void throw_from_errno( const char* context);
            void throw_from_errno( const std::string& context);
            //! @}

            void throw_from_code( int code);

         } // system 
      } // exception 
   } // common
} // casual



#endif
