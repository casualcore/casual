//!
//! casual 
//!

#ifndef CASUAL_MIDDLEWARE_CONFIGURATION_INCLUDE_CONFIG_TRANSACTION_H_
#define CASUAL_MIDDLEWARE_CONFIGURATION_INCLUDE_CONFIG_TRANSACTION_H_

#include "sf/namevaluepair.h"
#include "sf/platform.h"

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
               sf::optional< std::string> key;
               sf::optional< sf::platform::size::type> instances;

               CASUAL_CONST_CORRECT_SERIALIZE
               (
                  archive & CASUAL_MAKE_NVP( key);
                  archive & CASUAL_MAKE_NVP( instances);
               )
            };
         } // resource

         struct Resource : resource::Default
         {
            Resource();
            Resource( std::function< void(Resource&)> foreign);

            std::string name;
            std::string note;

            sf::optional< std::string> openinfo;
            sf::optional< std::string> closeinfo;

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


            //!
            //! Complement with defaults and validates
            //!
            void finalize();

            CASUAL_CONST_CORRECT_SERIALIZE
            (
               archive & sf::name::value::pair::make( "default", manager_default);
               archive & CASUAL_MAKE_NVP( log);
               archive & CASUAL_MAKE_NVP( resources);
            )
         };


      } // transaction
   } // configuration
} // casual

#endif // CASUAL_MIDDLEWARE_CONFIGURATION_INCLUDE_CONFIG_TRANSACTION_H_
