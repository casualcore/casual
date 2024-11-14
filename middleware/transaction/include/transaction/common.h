//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once




#include "common/log/stream.h"
#include "common/log.h"


namespace casual
{
   namespace transaction
   {
      extern common::log::Stream& log; // = common::log::category::transaction;


      using Trace = common::Trace;


   } // transaction

} // casual


