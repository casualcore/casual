//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

//!
//! casual
//!

#include "http/outbound/configuration.h"
#include "http/common.h"

#include "casual/platform.h"

#include "common/algorithm/container.h"
#include "common/serialize/create.h"
#include "common/file.h"
#include "common/environment/normalize.h"

namespace casual
{
   namespace http
   {
      namespace outbound
      {
         namespace configuration
         {
            namespace local
            {
               namespace
               {
                  Model load( Model current, const std::filesystem::path& name)
                  {
                     Trace trace{ "http::outbound::configuration::local::load"};

                     common::log::line( verbose::log, "file: ", name);

                     // Create the reader and deserialize configuration
                     Model model;
                     common::file::Input file{ name};
                     auto reader = common::serialize::create::reader::consumed::from( file);
                     
                     reader >> CASUAL_NAMED_VALUE_NAME( model, "http");
                     reader.validate();
                     common::environment::normalize( model);

                     common::log::line( verbose::log, "model: ", model);

                     return current + model;
                  }

               } // <unnamed>
            } // local


            Default operator + ( Default lhs, Default rhs)
            {
               lhs.service.discard_transaction = rhs.service.discard_transaction;
               common::algorithm::container::append( rhs.service.headers, lhs.service.headers);
               return lhs;
            }

            Model operator + ( Model lhs, Model rhs)
            {
               lhs.casual_default = lhs.casual_default + rhs.casual_default;
               common::algorithm::container::append( rhs.services, lhs.services);
               return lhs;
            }



            Model load( const std::vector< std::filesystem::path>& paths)
            {  
               return common::algorithm::accumulate( paths, Model{}, &local::load);
            }

            Model load( const std::filesystem::path& path)
            {
               return local::load( {}, path);
            }


            Model load( const std::vector< std::string>& patterns)
            {
               Trace trace{ "http::outbound::configuration::load"};
               common::log::line( verbose::log, "patterns: ", patterns);

               return load( common::file::find( patterns));
            }

         } // configuration

      } // outbound
   } // http
} // casual




