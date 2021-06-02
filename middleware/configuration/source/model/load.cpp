//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "configuration/model/load.h"
#include "configuration/model/transform.h"
#include "configuration/model.h"
#include "configuration/user.h"
#include "configuration/common.h"

#include "common/serialize/create.h"
#include "common/file.h"


namespace casual
{
   using namespace common;
   namespace configuration
   {
      namespace model
      {
         namespace local
         {
            namespace
            {
               auto load( Model current, const std::filesystem::path& file)
               {
                  Trace trace{ "configuration::user::local::load"};
                  log::line( verbose::log, "file: ", file);

                  user::Domain domain;

                  common::file::Input stream{ file};
                  auto archive = common::serialize::create::reader::consumed::from( stream);
                  archive >> CASUAL_NAMED_VALUE( domain);
                  archive.validate();

                  current += model::transform( std::move( domain));

                  return current;
               }

            } // <unnamed>
         } // local


         configuration::Model load( const std::vector< std::filesystem::path>& files)
         {
            Trace trace{ "configuration::model::load"};
            log::line( verbose::log, "files: ", files);

            auto model = algorithm::accumulate( files, Model{}, &local::load);

            log::line( verbose::log, "model: ", model);

            return model;
         }

      } // model
   } // configuration

} // casual