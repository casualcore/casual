//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "queue/code.h"

#include "common/exception/common.h"


namespace casual
{
   namespace queue
   {
      namespace exception
      {
         using base = common::exception::base_error< queue::code>;

         template< queue::code error>
         using basic = common::exception::basic_error< base, error>;

         namespace invalid
         {
            using Argument = basic< queue::code::argument>;
         } // invalid

         namespace no
         {
            using Message = basic< queue::code::no_message>;
            using Queue = basic< queue::code::no_queue>;
         } // no

         using System = basic< queue::code::system>;
         
      } // exception
   } // queue
} // casual