//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "serviceframework/namevaluepair.h"
#include "serviceframework/platform.h"

#include <string>
#include <vector>

namespace casual
{
   namespace configuration
   {
      namespace transaction
      {
         namespace resource
         {
            struct Default
            {
               serviceframework::optional< std::string> key;
               serviceframework::optional< serviceframework::platform::size::type> instances;

               CASUAL_CONST_CORRECT_SERIALIZE
               (
                  archive & CASUAL_MAKE_NVP( key);
                  archive & CASUAL_MAKE_NVP( instances);
               )
            };
         } // resource

         struct Resource : resource::Default
         {
            std::string name;
            std::string note;

            serviceframework::optional< std::string> openinfo;
            serviceframework::optional< std::string> closeinfo;

            CASUAL_CONST_CORRECT_SERIALIZE
            (
               resource::Default::serialize( archive);
               archive & CASUAL_MAKE_NVP( name);
               archive & CASUAL_MAKE_NVP( note);
               archive & CASUAL_MAKE_NVP( openinfo);
               archive & CASUAL_MAKE_NVP( closeinfo);
            )

            friend bool operator == ( const Resource& lhs, const Resource& rhs);
            friend Resource& operator += ( Resource& lhs, const resource::Default& rhs);
         };

         namespace manager
         {
            struct Default
            {
               resource::Default resource;

               CASUAL_CONST_CORRECT_SERIALIZE
               (
                  archive & CASUAL_MAKE_NVP( resource);
               )
            };

         } // manager

         struct Manager
         {
            Manager();

            manager::Default manager_default;

            std::string log;
            std::vector< Resource> resources;

            //! Complement with defaults and validates
            void finalize();

            CASUAL_CONST_CORRECT_SERIALIZE
            (
               archive & serviceframework::name::value::pair::make( "default", manager_default);
               archive & CASUAL_MAKE_NVP( log);
               archive & CASUAL_MAKE_NVP( resources);
            )
         };


      } // transaction
   } // configuration
} // casual


