//!
//! casual
//!

#ifndef CASUAL_COMMON_EXCEPTION_XATMI_H_
#define CASUAL_COMMON_EXCEPTION_XATMI_H_

#include "common/error/code/xatmi.h"
#include "common/exception/common.h"

namespace casual
{
   namespace common
   {
      namespace exception 
      {
         namespace xatmi 
         {

            using exception = common::exception::base_error< error::code::xatmi>;

            template< error::code::xatmi error>
            using base = common::exception::basic_error< exception, error>;

            namespace no
            {
               using Message = base< error::code::xatmi::no_message>;
            } // no

            using Limit = base< error::code::xatmi::limit>;


            namespace os
            {
               using Error = base< error::code::xatmi::os>;
            } // os


            using Protocoll = base< error::code::xatmi::protocol>;

            namespace invalid
            {
               using Argument = base< error::code::xatmi::argument>;

               using Descriptor = base< error::code::xatmi::descriptor>;
            } // invalid

            namespace service
            {
               using Error = base< error::code::xatmi::service_error>;

               using Fail = base< error::code::xatmi::service_fail>;

               namespace no
               {
                  using Entry = base< error::code::xatmi::no_entry>;
               } // no


               using Advertised = base< error::code::xatmi::service_advertised>;
            }


            using System = base< error::code::xatmi::system>;

            using Timeout = base< error::code::xatmi::timeout>;

            namespace transaction
            {
               using Support = base< error::code::xatmi::transaction>;
            } // transaction

            using Signal = base< error::code::xatmi::signal>;

            namespace buffer
            {
               namespace type
               {
                  using Input = base< error::code::xatmi::buffer_input>;

                  using Output = base< error::code::xatmi::buffer_output>;
               } // type
            }

            namespace conversational
            {
               using Event = base< error::code::xatmi::event>;
            }
         } // xatmi 
      } // exception 
   } // common
} // casual



#endif