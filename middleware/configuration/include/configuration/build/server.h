//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once

#include "configuration/build/resource.h"

#include "common/serialize/macro.h"
#include "common/platform.h"
#include "common/optional.h"

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
                  //! Can be:
                  //! - 'auto' Join current transaction, or start a new one if there is no current.
                  //! - 'join' Join current transaction if there is one.
                  //! - 'atomic' Always start a new transaction.
                  //! - 'branch' Branch current transaction, or start a new on if there is no current.
                  //! - 'none' Don't start or join any transaction
                  //!
                  //! default is 'auto'
                  common::optional< std::string> transaction;

                  //! Arbitrary category.
                  //!
                  //! @attention categories starting with '.' is reserved by casual
                  common::optional< std::string> category;

                  CASUAL_CONST_CORRECT_SERIALIZE
                  (
                     CASUAL_SERIALIZE( transaction);
                     CASUAL_SERIALIZE( category);
                  )
               };

            } // service

            struct Service : service::Default
            {
               std::string name;
               common::optional< std::string> function;

               CASUAL_CONST_CORRECT_SERIALIZE
               (
                  CASUAL_SERIALIZE( name);
                  CASUAL_SERIALIZE( function);
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
                     CASUAL_SERIALIZE( service);
                  )
               };
            } // server

            struct Server
            {
               //! Default for all services
               server::Default server_default;

               std::vector< Resource> resources;
               std::vector< Service> services;

               CASUAL_CONST_CORRECT_SERIALIZE(
               {
                  CASUAL_SERIALIZE_NAME( server_default, "default");
                  CASUAL_SERIALIZE( resources);
                  CASUAL_SERIALIZE( services);
               })
            };

            Server get( const std::string& file);

         } // server
      } // build

   } // config


} // casual


