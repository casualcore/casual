//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "configuration/user/load.h"


#include "common/serialize/create.h"
#include "common/file.h"


namespace casual
{
   using namespace common;
   namespace configuration
   {
      namespace user
      {
         namespace local
         {
            namespace
            {
               auto load( Domain current, const std::string& file)
               {
                  Trace trace{ "configuration::model::local::load"};
                  log::line( verbose::log, "file: ", file);

                  user::Domain domain;

                  common::file::Input stream{ file};
                  auto archive = common::serialize::create::reader::consumed::from( stream.extension(), stream);
                  archive >> CASUAL_NAMED_VALUE( domain);
                  archive.validate();
                  domain.normalize();

                  current += std::move( domain);

                  return current;
               }

            } // <unnamed>
         } // local



         Domain load( const std::vector< std::string>& files)
         {
            Trace trace{ "configuration::user::load"};
            log::line( verbose::log, "files: ", files);

            auto domain = algorithm::accumulate( files, Domain{}, &local::load);

            log::line( verbose::log, "domain: ", domain);

            return domain;
         }

      } // user
   } // configuration

} // casual