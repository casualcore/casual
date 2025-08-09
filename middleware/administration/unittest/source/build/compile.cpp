//!
//! Copyright (c) 2025, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "administration/unittest/build/compile.h"
#include "administration/unittest/cli/command.h"

#include "common/code/raise.h"
#include "common/code/casual.h"

#include "common/unittest/file.h"

namespace casual
{
   namespace administration::unittest::build
   {
      casual::common::file::scoped::Path compile( std::string_view source_content)
      {
         auto source = common::unittest::file::temporary::content( ".cpp", source_content);

         auto object_file = common::unittest::file::temporary::name( ".o");

         auto capture = administration::unittest::cli::command::execute( "g++ -c ", source, " -o ", object_file, " -O3 -I ${CMAKE_SOURCE_DIR}/middleware/xatmi/include");

         if( ! capture)
            common::code::raise::error( common::code::casual::invalid_argument, "failed to compile source: ", source, " with error: ", capture.standard.error);

         return object_file;
      }
   }
} // casual
