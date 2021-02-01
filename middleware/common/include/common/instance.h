//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "casual/platform.h"

#include "common/environment/variable.h"
#include "common/serialize/macro.h"

#include <optional>
#include <string>

namespace casual
{
   namespace common::instance
   {
      struct Information
      {
         std::string alias;
         platform::size::type index{};

         CASUAL_CONST_CORRECT_SERIALIZE({
            CASUAL_SERIALIZE( alias);
            CASUAL_SERIALIZE( index);
         })
      };

      //! @returns an environament variable that has the supplied information 
      //! 'serialized' as its value  
      environment::Variable variable( const Information& value);

      //! @return the instance information. If present this has been set
      //! by the parent.
      const std::optional< Information>& information();

      //! @return the alias of the instance, if not present, basename is returned.
      std::string alias();

   } // common::instance
} // casual