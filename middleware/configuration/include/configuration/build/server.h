//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#ifndef SERVICEDEFINITION_H_
#define SERVICEDEFINITION_H_

#include "serviceframework/namevaluepair.h"
#include "serviceframework/platform.h"

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
                  serviceframework::optional< std::string> transaction;

                  //!
                  //! Arbitrary category.
                  //!
                  //! @attention categories starting with '.' is reserved by casual
                  //!
                  serviceframework::optional< std::string> category;

                  CASUAL_CONST_CORRECT_SERIALIZE
                  (
                     archive & CASUAL_MAKE_NVP( transaction);
                     archive & CASUAL_MAKE_NVP( category);
                  )
               };

            } // service

            struct Service : service::Default
            {
               Service();
               Service( std::function< void(Service&)> foreign);

               std::string name;
               serviceframework::optional< std::string> function;

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
                  archive & serviceframework::name::value::pair::make( "default", server_default);
                  archive & CASUAL_MAKE_NVP( services);
               )
            };

            Server get( const std::string& file);

         } // server
      } // build

   } // config


} // casual

#endif // SERVICEDEFINITION_H_
