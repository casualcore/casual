//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/environment/variable.h"

#include <string>

namespace casual
{
   namespace common::environment
   {
         
      //! Parses value for environment variables with format @p ${<variable>}
      //! and tries to find and 'expand' the variable from environment.
      //!
      //! @return possible altered string with regards to environment variables
      //! @note can be used with types that has std::string conversion function, ie std::filesystem::path.
      //! @{
      std::string expand( std::string value);

      //! uses `local` repository first to extract environment values, if not found, the real enviornment is used.
      std::string expand( std::string value, const std::vector< environment::Variable>& local);
      //! @}

   } // common::environment
} // casual