//!
//! casual
//!

#ifndef CASUAL_COMMON_EXCEPTION_SYSTEM_H_
#define CASUAL_COMMON_EXCEPTION_SYSTEM_H_

#include "common/error/code/system.h"
#include "common/exception/common.h"

namespace casual
{
   namespace common
   {
      namespace exception 
      {
         namespace system 
         {

            using exception = common::exception::base_error< error::code::system>;

            template< error::code::system error>
            using base = common::exception::basic_error< exception, error>;
            

            namespace invalid
            {
               using Argument = system::base< error::code::system::invalid_argument>;

               using File = system::base< error::code::system::no_such_file_or_directory>;

               using Process = system::base< error::code::system::no_such_process>;               

            } // invalid

            namespace communication
            {
               namespace no
               {
                  struct Message : system::exception
                  {
                     using system::exception::exception;
                  };

                  namespace message 
                  {
                     template< error::code::system error>
                     using basic = common::exception::basic_error< Message, error>;

                     using Absent = basic< error::code::system::no_message>;
                     using Resource = basic< error::code::system::resource_unavailable_try_again>;

                  } // message 
               } // no

               struct Unavailable : system::exception
               {
                  using system::exception::exception;
               };

               namespace unavailable
               {
                  template< error::code::system error>
                  using basic = common::exception::basic_error< Unavailable, error>;

                  using Removed = basic< error::code::system::identifier_removed>;
                  using Reset = basic< error::code::system::connection_reset>;
                  using Pipe = basic< error::code::system::broken_pipe>;
                 
                  namespace no 
                  {
                     using Connect = basic< error::code::system::not_connected>;   
                  } // not 
               } // unavailable

               using Refused = system::base< error::code::system::connection_refused>;

               using Protocol = system::base< error::code::system::protocol_error>;

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