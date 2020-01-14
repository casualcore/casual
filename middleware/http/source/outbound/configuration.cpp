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

#include "common/algorithm.h"
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
                  Model get( Model current, const std::string& name)
                  {
                     Trace trace{ "http::outbound::configuration::local::get"};

                     common::log::line( verbose::log, "file: ", name);

                     // Create the reader and deserialize configuration
                     Model http;
                     common::file::Input file{ name};
                     auto reader = common::serialize::create::reader::consumed::from( file.extension(), file);
                     
                     reader >> CASUAL_NAMED_VALUE( http);
                     reader.validate();

                     common::log::line( verbose::log, "http: ", http);

                     return current + http;

                  }
               } // <unnamed>
            } // local



            Default operator + ( Default lhs, Default rhs)
            {
               lhs.service.discard_transaction = rhs.service.discard_transaction;
               common::algorithm::append( rhs.service.headers, lhs.service.headers);
               return lhs;
            }

            Model operator + ( Model lhs, Model rhs)
            {
               lhs.casual_default = lhs.casual_default + rhs.casual_default;
               common::algorithm::append( rhs.services, lhs.services);
               return lhs;
            }

            Model get( const std::string& file)
            {
               Trace trace{ "http::outbound::configuration::get"};

               auto result = local::get( Model{}, file);
               
               // normalize environment stuff
               common::environment::normalize( result);

               return result;
            }

            Model get( const std::vector< std::string>& files)
            {
               Trace trace{ "http::outbound::configuration::get"};

               common::log::line( verbose::log, "files: ", files);

               auto result = common::algorithm::accumulate( files, Model{}, &local::get);

               // normalize environment stuff
               common::environment::normalize( result);

               return result;
            }

         } // configuration

      } // outbound
   } // http
} // casual




