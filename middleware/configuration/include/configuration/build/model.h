//!
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/serialize/macro.h"

#include <string>
#include <vector>
#include <optional>

namespace casual
{
   namespace configuration::build
   {
      namespace model
      {
         struct Resource
         {
            //! key of the resource
            std::string key;

            //! name to correlate configuration
            std::string name;

            // mostly for examples
            std::string note;

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( key);
               CASUAL_SERIALIZE( name);
               CASUAL_SERIALIZE( note);
            )
         };
      } // model

      namespace server
      {
         namespace model
         {
            struct Service
            {
               std::string name;
               std::optional< std::string> function;

               //! Can be:
               //! - 'auto' Join current transaction, or start a new one if there is no current.
               //! - 'join' Join current transaction if there is one.
               //! - 'atomic' Always start a new transaction.
               //! - 'none' Don't start or join any transaction
               //! - 'branch' Branch current transaction, or start a new on if there is no current. Do not use.
               //!
               //! default is 'auto'
               std::optional< std::string> transaction;

               //! Cant be:
               //! - 'discoverable': If the service (and all advertised names to the function) will be discoverable from
               //!                   other domains (without explicit configuration for service names)
               //! - 'undiscoverable'
               //! default is: 'discoverable'
               std::optional< std::string> visibility;

               //! Arbitrary category.
               //! @attention categories starting with '.' is reserved by casual
               std::optional< std::string> category;


               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE( name);
                  CASUAL_SERIALIZE( function);
                  CASUAL_SERIALIZE( transaction);
                  CASUAL_SERIALIZE( visibility);
                  CASUAL_SERIALIZE( category);
                  
               )
            };

            namespace server
            {
               struct Default
               {
                  struct
                  {
                     std::string transaction = "auto";
                     std::optional< std::string> category;
                     std::optional< std::string> visibility;

                     CASUAL_CONST_CORRECT_SERIALIZE(
                        CASUAL_SERIALIZE( transaction);
                        CASUAL_SERIALIZE( category);
                        CASUAL_SERIALIZE( visibility);
                     )

                  } service;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                     CASUAL_SERIALIZE( service);
                  )
               };
            } // server

            struct Server
            {
               //! Default for all services
               server::Default server_default;

               std::vector< build::model::Resource> resources;
               std::vector< Service> services;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE_NAME( server_default, "default");
                  CASUAL_SERIALIZE( resources);
                  CASUAL_SERIALIZE( services);
               )
            };
         } // model

         struct Model
         {
            model::Server server;

            //! normalizes the 'manager', mostly to set default values
            friend Model normalize( Model model);

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( server);
            )
         };
      } // server


      namespace executable
      {
         namespace model
         {
            struct Executable
            {            
               std::vector< build::model::Resource> resources;

               //! name of the function that casual invokes 
               //! after resource setup.
               //! Has to be defined as  int <entrypoint-name>( int argc, const char** argv)
               //!
               //! default: executable_entrypoint
               //!
               std::string entrypoint = "executable_entrypoint";

               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE( resources);
                  CASUAL_SERIALIZE( entrypoint);
               )
            };            
         } // model

         struct Model
         {
            model::Executable executable;

            inline friend Model normalize( Model model) { return model;}

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( executable);
            )
         };

      } // executable


         
   } // configuration::build
} // casual