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


#include "common/algorithm.h"

#include "serviceframework/archive/create.h"
#include "serviceframework/platform.h"

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

                     //
                     // Create the reader and deserialize configuration
                     //
                     Model http;
                     common::file::Input file{ name};
                     auto reader = serviceframework::archive::create::reader::consumed::from( file.extension(), file);
                     
                     reader >> CASUAL_MAKE_NVP( http);
                     reader.validate();

                     common::log::line( verbose::log, "http: ", http);

                     return current + http;

                  }
               } // <unnamed>
            } // local

            std::ostream& operator << ( std::ostream& out, const Header& value)
            {
               return out << "{ name: " << value.name
                  << ", value: " << value.value
                  << '}';
            }

            std::ostream& operator << ( std::ostream& out, const Service& value)
            {
               out << "{ " << value.name
                  << ", url: " << value.url
                  << ", headers: " << common::range::make( value.headers);
               if( value.discard_transaction)
                  out << ", discard_transaction: " << value.discard_transaction.value();

               return out << '}';
            }

            Default operator + ( Default lhs, Default rhs)
            {
               lhs.service.discard_transaction = rhs.service.discard_transaction;
               common::algorithm::append( rhs.service.headers, lhs.service.headers);
               return lhs;
            }

            std::ostream& operator << ( std::ostream& out, const Default& value)
            {
               return out << "{ discard_transaction: " << value.service.discard_transaction
                  << ", headers: " << common::range::make( value.service.headers)
                 << '}';
            }

            Model operator + ( Model lhs, Model rhs)
            {
               lhs.casual_default = lhs.casual_default + rhs.casual_default;
               common::algorithm::append( rhs.services, lhs.services);
               return lhs;
            }

            std::ostream& operator << ( std::ostream& out, const Model& value)
            {
               return out << "{ default: " << value.casual_default
                  << ", services: " << common::range::make( value.services)
                  << '}';
            }

            Model get( const std::string& file)
            {
               Trace trace{ "http::outbound::configuration::get"};

               return local::get( Model{}, file);
            }

            Model get( const std::vector< std::string>& files)
            {
               Trace trace{ "http::outbound::configuration::get"};

               common::log::line( verbose::log, "files: ", files);

               return common::algorithm::accumulate( files, Model{}, &local::get);
            }

         } // configuration

      } // outbound
   } // http
} // casual




