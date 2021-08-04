//! 
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once


namespace casual
{
   namespace common
   {
      namespace build
      {
         using size_type = platform::size::type;

         struct Version
         {
            std::string casual;
            std::string compiler;
            std::string commit;

            struct
            {
               std::vector< size_type> protocols;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE( protocols);
               )
            } gateway;

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( casual);
               CASUAL_SERIALIZE( compiler);
               CASUAL_SERIALIZE( commit);
               CASUAL_SERIALIZE( gateway);
            )

         };


         inline auto version()
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
      }
   }
} // casual
