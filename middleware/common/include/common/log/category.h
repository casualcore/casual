//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "common/log/stream.h"

namespace casual
{
   namespace common::log::category
   {

      //! Log with category 'parameter'
      extern Stream parameter;

      //! Log with category 'information'
      extern Stream information;

      //! Log with category 'warning'
      //!
      //! @note should be used very sparsely. Either it is an error or it's not...
      extern Stream warning;

      //! Log with category 'error'
      //!
      //! @note always active
      extern Stream error;

      namespace verbose
      {
         //! Log with category 'error.verbose'
         extern Stream error;
      } // verbose

      //! Log with category 'casual.transaction'
      extern Stream transaction;

      //! Log with category 'casual.buffer'
      extern Stream buffer;
      
   } // common::log::category
} // casual
