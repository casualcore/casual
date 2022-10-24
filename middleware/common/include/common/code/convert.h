//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "common/code/tx.h"
#include "common/code/xa.h"
#include "common/code/casual.h"


namespace casual
{
   namespace common::code::convert::to
   {
      code::tx tx( code::xa code);

      code::casual casual( std::errc code);

   } // common::code::convert::to
} // casual

