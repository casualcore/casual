//!
//! casual
//!

#ifndef SERVICEDEFINITION_H_
#define SERVICEDEFINITION_H_

#include "sf/namevaluepair.h"
#include "sf/platform.h"

#include <algorithm>
#include <string>
#include <vector>

namespace casual
{

   namespace configuration
   {
      namespace build
      {
         namespace server
         {
            namespace service
            {
               struct Default
               {
                  //!
                  //! Can be:
                  //! - 'auto' Join current transaction, or start a new one if there is no current.
                  //! - 'join' Join current transaction if there is one.
                  //! - 'atomic' Always start a new transaction.
                  //! - 'none' Don't start or join any transaction
                  //!
                  //! default is 'auto'
                  //!
                  sf::optional< std::string> transaction;

                  sf::optional< std::size_t> type;

                  CASUAL_CONST_CORRECT_SERIALIZE
                  (
                     archive & CASUAL_MAKE_NVP( transaction);
                     archive & CASUAL_MAKE_NVP( type);
                  )
               };

            } // service

            struct Service : service::Default
            {
               Service();
               Service( std::function< void(Service&)> foreign);

               std::string name;
               sf::optional< std::string> function;

               CASUAL_CONST_CORRECT_SERIALIZE
               (
                  archive & CASUAL_MAKE_NVP( name);
                  archive & CASUAL_MAKE_NVP( function);
                  service::Default::serialize( archive);
               )

            };

            namespace server
            {
               struct Default
               {
                  Default();

                  service::Default service;

                  CASUAL_CONST_CORRECT_SERIALIZE
                  (
                     archive & CASUAL_MAKE_NVP( service);
                  )
               };
            } // server

            struct Server
            {

               //!
               //! Default for all services
               //!
               server::Default server_default;

               std::vector< Service> services;

               CASUAL_CONST_CORRECT_SERIALIZE
               (
                  archive & sf::name::value::pair::make( "default", server_default);
                  archive & CASUAL_MAKE_NVP( services);
               )
            };

            Server get( const std::string& file);

         } // server
      } // build

   } // config


} // casual

#endif // SERVICEDEFINITION_H_
