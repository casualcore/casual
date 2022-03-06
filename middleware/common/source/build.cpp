//!
//! Copyright (c) 2022, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/build.h"

#include "common/string/compose.h"
#include "common/stream.h"

namespace casual
{
   namespace common::build
   {
      Version version() noexcept
      {
         Version result;

         // casual version
#ifdef CASUAL_MAKE_BUILD_VERSION
         result.casual = CASUAL_MAKE_BUILD_VERSION;
#endif 
         // compiler version
#ifdef __clang_version__
         result.compiler = string::compose( "clang: ", __clang_version__);
#elif __GNUC__
         result.compiler = string::compose( "g++: ", __GNUC__, '.', __GNUC_MINOR__, '.', __GNUC_PATCHLEVEL__);
#endif
         // commit hash
#ifdef CASUAL_MAKE_COMMIT_HASH
         result.commit = CASUAL_MAKE_COMMIT_HASH;
#endif

         return result;
      }
   } // common::build
} // casual