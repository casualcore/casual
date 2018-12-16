//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "configuration/build/executable.h"

#include "serviceframework/archive/create.h"

namespace casual
{
   namespace configuration
   {
      namespace build
      {
         Executable::Executable() : entrypoint{ "executable_entrypoint"}
         {

         }

         Executable::~Executable() = default;

         namespace executable
         {
            Executable get( const std::string& name)
            {
               Executable executable;

               //
               // Create the reader and deserialize configuration
               //
               common::file::Input file{ name};
               auto reader = serviceframework::archive::create::reader::consumed::from( file.extension(), file);

               reader >> CASUAL_MAKE_NVP( executable);

               reader.validate();


               return executable;
            }

         } // executable

      } // build
   } // configuration
} // casual