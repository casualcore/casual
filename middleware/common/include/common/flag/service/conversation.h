//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "common/flag/xatmi.h"

#include "common/cast.h"

namespace casual
{
   namespace common
   {
      namespace flag
      {
         namespace service
         {
            namespace conversation
            {
               enum class Event : long
               {
                  absent = cast::underlying( xatmi::Event::absent),
                  disconnect = cast::underlying( xatmi::Event::disconnect),
                  send_only = cast::underlying( xatmi::Event::send_only),
                  service_error = cast::underlying( xatmi::Event::service_error),
                  service_fail = cast::underlying( xatmi::Event::service_fail),
                  service_success = cast::underlying( xatmi::Event::service_success),
               };

               using Events = common::Flags< Event>;

               namespace connect
               {
                  enum class Flag : long
                  {
                     no_transaction = cast::underlying( xatmi::Flag::no_transaction),
                     send_only =  cast::underlying( xatmi::Flag::send_only),
                     receive_only =  cast::underlying( xatmi::Flag::receive_only),
                     no_block =  cast::underlying( xatmi::Flag::no_block),
                     no_time =  cast::underlying( xatmi::Flag::no_time),
                     signal_restart =  cast::underlying( xatmi::Flag::signal_restart)
                  };

                  using Flags = common::Flags< connect::Flag>;

               } // connect

               namespace send
               {
                  enum class Flag : long
                  {
                     receive_only =  cast::underlying( xatmi::Flag::receive_only),
                     no_block =  cast::underlying( xatmi::Flag::no_block),
                     no_time =  cast::underlying( xatmi::Flag::no_time),
                     signal_restart =  cast::underlying( xatmi::Flag::signal_restart)
                  };

                  using Flags = common::Flags< send::Flag>;
               } // send

               namespace receive
               {
                  enum class Flag : long
                  {
                     no_change = cast::underlying( xatmi::Flag::no_change),
                     no_block =  cast::underlying( xatmi::Flag::no_block),
                     no_time =  cast::underlying( xatmi::Flag::no_time),
                     signal_restart =  cast::underlying( xatmi::Flag::signal_restart)
                  };

                  using Flags = common::Flags< receive::Flag>;

               } // receive

            } // conversation
         } // service
      } // flag
   } // common
} // casual


