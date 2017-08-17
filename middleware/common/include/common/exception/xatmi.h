//!
//! casual
//!

#ifndef CASUAL_COMMON_EXCEPTION_XATMI_H_
#define CASUAL_COMMON_EXCEPTION_XATMI_H_

#include "common/code/xatmi.h"
#include "common/exception/common.h"

namespace casual
{
   namespace common
   {
      namespace exception 
      {
         namespace xatmi 
         {

            using exception = common::exception::base_error< code::xatmi>;

            template< code::xatmi error>
            using base = common::exception::basic_error< exception, error>;

            namespace no
            {
               using Message = base< code::xatmi::no_message>;
            } // no

            using Limit = base< code::xatmi::limit>;


            namespace os
            {
               using Error = base< code::xatmi::os>;
            } // os


            using Protocoll = base< code::xatmi::protocol>;

            namespace invalid
            {
               using Argument = base< code::xatmi::argument>;

               using Descriptor = base< code::xatmi::descriptor>;
            } // invalid

            namespace service
            {
               using Error = base< code::xatmi::service_error>;

               using Fail = base< code::xatmi::service_fail>;

               namespace no
               {
                  using Entry = base< code::xatmi::no_entry>;
               } // no


               using Advertised = base< code::xatmi::service_advertised>;
            }


            using System = base< code::xatmi::system>;

            using Timeout = base< code::xatmi::timeout>;

            namespace transaction
            {
               using Support = base< code::xatmi::transaction>;
            } // transaction

            using Signal = base< code::xatmi::signal>;

            namespace buffer
            {
               namespace type
               {
                  using Input = base< code::xatmi::buffer_input>;

                  using Output = base< code::xatmi::buffer_output>;
               } // type
            }

            namespace conversational
            {
               using Event = base< code::xatmi::event>;
            }

            code::xatmi handle();

         } // xatmi 
      } // exception 
   } // common
} // casual



#endif
