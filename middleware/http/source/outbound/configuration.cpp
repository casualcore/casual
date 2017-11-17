//!
//! casual
//!

#include "http/outbound/configuration.h"

#include "http/common.h"


#include "common/algorithm.h"

#include "sf/archive/maker.h"
#include "sf/platform.h"

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
                  Model get( Model current, const std::string& file)
                  {
                     Trace trace{ "http::outbound::configuration::local::get"};

                     verbose::log << "file: " << file << '\n';

                     //
                     // Create the reader and deserialize configuration
                     //
                     Model http;
                     auto reader = sf::archive::reader::from::file( file);
                     reader >> CASUAL_MAKE_NVP( http);

                     verbose::log << "http: " << http << '\n';

                     return current + http;

                  }
               } // <unnamed>
            } // local

            Default operator + ( Default lhs, Default rhs)
            {
               common::algorithm::append( lhs.headers, rhs.headers);
               return lhs;
            }

            Model operator + ( Model lhs, Model rhs)
            {
               lhs.casual_default = lhs.casual_default + rhs.casual_default;
               common::algorithm::append( rhs.services, lhs.services);
               return lhs;
            }

            std::ostream& operator << ( std::ostream& out, const Model& value)
            {
               return out;
            }

            Model get( const std::string& file)
            {
               Trace trace{ "http::outbound::configuration::get"};

               return local::get( Model{}, file);
            }

            Model get( const std::vector< std::string>& files)
            {
               Trace trace{ "http::outbound::configuration::get"};

               verbose::log << "files: " << common::range::make( files) << '\n';

               return common::algorithm::accumulate( files, Model{}, &local::get);
            }

         } // configuration

      } // outbound
   } // http
} // casual




