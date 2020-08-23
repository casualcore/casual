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

                  Model accumulate( const std::vector< std::string>& paths)
                  {
                     auto result = common::algorithm::accumulate( paths, Model{}, &local::get);

                     // normalize environment stuff
                     common::environment::normalize( result);

                     return result;
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

            Model get( const std::string& pattern)
            {
               Trace trace{ "http::outbound::configuration::get"};
               common::log::line( verbose::log, "pattern: ", pattern);

               return local::accumulate( common::file::find( pattern));
            }

            Model get( const std::vector< std::string>& patterns)
            {
               Trace trace{ "http::outbound::configuration::get"};
               common::log::line( verbose::log, "patterns: ", patterns);

               return local::accumulate( common::file::find( patterns));
            }

         } // configuration

      } // outbound
   } // http
} // casual




